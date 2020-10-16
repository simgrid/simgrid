/* extracted from mpig_myreduce.c with
   :3,$s/MPL/MPI/g     and  :%s/\\\\$/ \\/   */

/* Copyright: Rolf Rabenseifner, 1997
 *            Computing Center University of Stuttgart
 *            rabenseifner@rus.uni-stuttgart.de
 *
 * The usage of this software is free,
 * but this header must not be removed.
 */

#include "../colls_private.hpp"
#include <cstdio>
#include <cstdlib>

#define REDUCE_NEW_ALWAYS 1

#ifdef CRAY
#      define SCR_LNG_OPTIM(bytelng)  128 + ((bytelng+127)/256) * 256;
                                 /* =  16 + multiple of 32 doubles*/

#define REDUCE_LIMITS /*  values are lower limits for count arg.  */                                                   \
                      /*  routine =    reduce                allreduce             */                                  \
                      /*  size    =       2,   3,2**n,other     2,   3,2**n,other  */                                  \
  static int Lsh[2][4] = {{896, 1728, 576, 736}, {448, 1280, 512, 512}};                                               \
  static int Lin[2][4] = {{896, 1728, 576, 736}, {448, 1280, 512, 512}};                                               \
  static int Llg[2][4] = {{896, 1728, 576, 736}, {448, 1280, 512, 512}};                                               \
  static int Lfp[2][4] = {{896, 1728, 576, 736}, {448, 1280, 512, 512}};                                               \
  static int Ldb[2][4] = {{896, 1728, 576, 736}, {448, 1280, 512, 512}};                                               \
  static int Lby[2][4] = {{896, 1728, 576, 736}, {448, 1280, 512, 512}};
#endif

#ifdef REDUCE_NEW_ALWAYS
# undef  REDUCE_LIMITS
#define REDUCE_LIMITS /*  values are lower limits for count arg.  */                                                   \
                      /*  routine =    reduce                allreduce             */                                  \
                      /*  size    =       2,   3,2**n,other     2,   3,2**n,other  */                                  \
  static int Lsh[2][4] = {{1, 1, 1, 1}, {1, 1, 1, 1}};                                                                 \
  static int Lin[2][4] = {{1, 1, 1, 1}, {1, 1, 1, 1}};                                                                 \
  static int Llg[2][4] = {{1, 1, 1, 1}, {1, 1, 1, 1}};                                                                 \
  static int Lfp[2][4] = {{1, 1, 1, 1}, {1, 1, 1, 1}};                                                                 \
  static int Ldb[2][4] = {{1, 1, 1, 1}, {1, 1, 1, 1}};                                                                 \
  static int Lby[2][4] = {{1, 1, 1, 1}, {1, 1, 1, 1}};
#endif

/* Fast reduce and allreduce algorithm for longer buffers and predefined
   operations.

   This algorithm is explained with the example of 13 nodes.
   The nodes are numbered   0, 1, 2, ... 12.
   The sendbuf content is   a, b, c, ...  m.
   The buffer array is notated with ABCDEFGH, this means that
   e.g. 'C' is the third 1/8 of the buffer.

   The algorithm computes

   { [(a+b)+(c+d)] + [(e+f)+(g+h)] }  +  { [(i+j)+k] + [l+m] }

   This is equivalent to the mostly used binary tree algorithm
   (e.g. used in mpich).

   size := number of nodes in the communicator.
   2**n := the power of 2 that is next smaller or equal to the size.
   r    := size - 2**n

   Exa.: size=13 ==> n=3, r=5  (i.e. size == 13 == 2**n+r ==  2**3 + 5)

   The algorithm needs for the execution of one colls::reduce

   - for r==0
     exec_time = n*(L1+L2)     + buf_lng * (1-1/2**n) * (T1 + T2 + O/d)

   - for r>0
     exec_time = (n+1)*(L1+L2) + buf_lng               * T1
                               + buf_lng*(1+1/2-1/2**n)*     (T2 + O/d)

   with L1 = latency of a message transfer e.g. by send+recv
        L2 = latency of a message exchange e.g. by sendrecv
        T1 = additional time to transfer 1 byte (= 1/bandwidth)
        T2 = additional time to exchange 1 byte in both directions.
        O  = time for one operation
        d  = size of the datatype

        On a MPP system it is expected that T1==T2.

   In Comparison with the binary tree algorithm that needs:

   - for r==0
     exec_time_bin_tree =  n * L1   +  buf_lng * n * (T1 + O/d)

   - for r>0
     exec_time_bin_tree = (n+1)*L1  +  buf_lng*(n+1)*(T1 + O/d)

   the new algorithm is faster if (assuming T1=T2)

    for n>2:
     for r==0:  buf_lng  >  L2 *   n   / [ (n-2) * T1 + (n-1) * O/d ]
     for r>0:   buf_lng  >  L2 * (n+1) / [ (n-1.5)*T1 + (n-0.5)*O/d ]

    for size = 5, 6, and 7:
                buf_lng  >  L2 / [0.25 * T1  +  0.58 * O/d ]

    for size = 4:
                buf_lng  >  L2 / [0.25 * T1  +  0.62 * O/d ]
    for size = 2 and 3:
                buf_lng  >  L2 / [  0.5 * O/d ]

   and for O/d >> T1 we can summarize:

    The new algorithm is faster for about   buf_lng > 2 * L2 * d / O

   Example L1 = L2 = 50 us,
           bandwidth=300MB/s, i.e. T1 = T2 = 3.3 ns/byte
           and 10 MFLOP/s,    i.e. O  = 100 ns/FLOP
           for double,        i.e. d  =   8 byte/FLOP

           ==> the new algorithm is faster for about buf_lng>8000 bytes
               i.e count > 1000 doubles !
Step 1)

   compute n and r

Step 2)

   if  myrank < 2*r

     split the buffer into ABCD and EFGH

     even myrank: send    buffer EFGH to   myrank+1 (buf_lng/2 exchange)
                  receive buffer ABCD from myrank+1
                  compute op for ABCD                 (count/2 ops)
                  receive result EFGH               (buf_lng/2 transfer)
     odd  myrank: send    buffer ABCD to   myrank+1
                  receive buffer EFGH from myrank+1
                  compute op for EFGH
                  send    result EFGH

   Result: node:     0    2    4    6    8  10  11  12
           value:  a+b  c+d  e+f  g+h  i+j   k   l   m

Step 3)

   The following algorithm uses only the nodes with

   (myrank is even  &&  myrank < 2*r) || (myrank >= 2*r)

Step 4)

   define NEWRANK(old) := (old < 2*r ? old/2 : old-r)
   define OLDRANK(new) := (new < r   ? new*2 : new+r)

   Result:
       old ranks:    0    2    4    6    8  10  11  12
       new ranks:    0    1    2    3    4   5   6   7
           value:  a+b  c+d  e+f  g+h  i+j   k   l   m

Step 5.1)

   Split the buffer (ABCDEFGH) in the middle,
   the lower half (ABCD) is computed on even (new) ranks,
   the opper half (EFGH) is computed on odd  (new) ranks.

   exchange: ABCD from 1 to 0, from 3 to 2, from 5 to 4 and from 7 to 6
             EFGH from 0 to 1, from 2 to 3, from 4 to 5 and from 6 to 7
                                               (i.e. buf_lng/2 exchange)
   compute op in each node on its half:        (i.e.   count/2 ops)

   Result: node 0: (a+b)+(c+d)   for  ABCD
           node 1: (a+b)+(c+d)   for  EFGH
           node 2: (e+f)+(g+h)   for  ABCD
           node 3: (e+f)+(g+h)   for  EFGH
           node 4: (i+j)+ k      for  ABCD
           node 5: (i+j)+ k      for  EFGH
           node 6:    l + m      for  ABCD
           node 7:    l + m      for  EFGH

Step 5.2)

   Same with double distance and oncemore the half of the buffer.
                                               (i.e. buf_lng/4 exchange)
                                               (i.e.   count/4 ops)

   Result: node 0: [(a+b)+(c+d)] + [(e+f)+(g+h)]  for  AB
           node 1: [(a+b)+(c+d)] + [(e+f)+(g+h)]  for  EF
           node 2: [(a+b)+(c+d)] + [(e+f)+(g+h)]  for  CD
           node 3: [(a+b)+(c+d)] + [(e+f)+(g+h)]  for  GH
           node 4: [(i+j)+ k   ] + [   l + m   ]  for  AB
           node 5: [(i+j)+ k   ] + [   l + m   ]  for  EF
           node 6: [(i+j)+ k   ] + [   l + m   ]  for  CD
           node 7: [(i+j)+ k   ] + [   l + m   ]  for  GH

...
Step 5.n)

   Same with double distance and oncemore the half of the buffer.
                                            (i.e. buf_lng/2**n exchange)
                                            (i.e.   count/2**n ops)

   Result:
     0: { [(a+b)+(c+d)] + [(e+f)+(g+h)] } + { [(i+j)+k] + [l+m] } for A
     1: { [(a+b)+(c+d)] + [(e+f)+(g+h)] } + { [(i+j)+k] + [l+m] } for E
     2: { [(a+b)+(c+d)] + [(e+f)+(g+h)] } + { [(i+j)+k] + [l+m] } for C
     3: { [(a+b)+(c+d)] + [(e+f)+(g+h)] } + { [(i+j)+k] + [l+m] } for G
     4: { [(a+b)+(c+d)] + [(e+f)+(g+h)] } + { [(i+j)+k] + [l+m] } for B
     5: { [(a+b)+(c+d)] + [(e+f)+(g+h)] } + { [(i+j)+k] + [l+m] } for F
     6: { [(a+b)+(c+d)] + [(e+f)+(g+h)] } + { [(i+j)+k] + [l+m] } for D
     7: { [(a+b)+(c+d)] + [(e+f)+(g+h)] } + { [(i+j)+k] + [l+m] } for H


For colls::allreduce:
------------------

Step 6.1)

   Exchange on the last distance (2*n) the last result
                                            (i.e. buf_lng/2**n exchange)

Step 6.2)

   Same with half the distance and double of the result
                                        (i.e. buf_lng/2**(n-1) exchange)

...
Step 6.n)

   Same with distance 1 and double of the result (== half of the
   original buffer)                            (i.e. buf_lng/2 exchange)

   Result after 6.1     6.2         6.n

    on node 0:   AB    ABCD    ABCDEFGH
    on node 1:   EF    EFGH    ABCDEFGH
    on node 2:   CD    ABCD    ABCDEFGH
    on node 3:   GH    EFGH    ABCDEFGH
    on node 4:   AB    ABCD    ABCDEFGH
    on node 5:   EF    EFGH    ABCDEFGH
    on node 6:   CD    ABCD    ABCDEFGH
    on node 7:   GH    EFGH    ABCDEFGH

Step 7)

   If r > 0
     transfer the result from the even nodes with old rank < 2*r
                           to the odd  nodes with old rank < 2*r
                                                 (i.e. buf_lng transfer)
   Result:
     { [(a+b)+(c+d)] + [(e+f)+(g+h)] } + { [(i+j)+k] + [l+m] }
     for ABCDEFGH
     on all nodes 0..12


For colls::reduce:
---------------

Step 6.0)

   If root node not in the list of the nodes with newranks
   (see steps 3+4) then

     send last result from node 0 to the root    (buf_lng/2**n transfer)
     and replace the role of node 0 by the root node.

Step 6.1)

   Send on the last distance (2**(n-1)) the last result
   from node with bit '2**(n-1)' in the 'new rank' unequal to that of
   root's new rank  to the node with same '2**(n-1)' bit.
                                            (i.e. buf_lng/2**n transfer)

Step 6.2)

   Same with half the distance and double of the result
   and bit '2**(n-2)'
                                        (i.e. buf_lng/2**(n-1) transfer)

...
Step 6.n)

   Same with distance 1 and double of the result (== half of the
   original buffer) and bit '2**0'             (i.e. buf_lng/2 transfer)

   Example: roots old rank: 10
            roots new rank:  5

   Results:          6.1               6.2                 6.n
                action result     action result       action result

    on node 0:  send A
    on node 1:  send E
    on node 2:  send C
    on node 3:  send G
    on node 4:  recv A => AB     recv CD => ABCD   send ABCD
    on node 5:  recv E => EF     recv GH => EFGH   recv ABCD => ABCDEFGH
    on node 6:  recv C => CD     send CD
    on node 7:  recv G => GH     send GH

Benchmark results on CRAY T3E
-----------------------------

   uname -a: (sn6307 hwwt3e 1.6.1.51 unicosmk CRAY T3E)
   MPI:      /opt/ctl/mpt/1.1.0.3
   datatype: MPI_DOUBLE
   Ldb[][] = {{ 896,1728, 576, 736},{ 448,1280, 512, 512}}
   env: export MPI_BUFFER_MAX=4099
   compiled with: cc -c -O3 -h restrict=f

   old = binary tree protocol of the vendor
   new = the new protocol and its implementation
   mixed = 'new' is used if count > limit(datatype, communicator size)

   REDUCE:
                                          communicator size
   measurement count prot. unit    2    3    4    6    8   12   16   24
   --------------------------------------------------------------------
   latency         1 mixed  us  20.7 35.1 35.6 49.1 49.2 61.8 62.4 74.8
                       old  us  19.0 32.9 33.8 47.1 47.2 61.7 62.1 73.2
   --------------------------------------------------------------------
   bandwidth    128k mixed MB/s 75.4 34.9 49.1 27.3 41.1 24.6 38.0 23.7
   (=buf_lng/time)     old MB/s 28.8 16.2 16.3 11.6 11.6  8.8  8.8  7.2
   ration = mixed/old            2.6  2.1  3.0  2.4  3.5  2.8  4.3  3.3
   --------------------------------------------------------------------
   limit                doubles  896 1536  576  736  576  736  576  736
   bandwidth   limit mixed MB/s 35.9 20.5 18.6 12.3 13.5  9.2 10.0  8.6
                       old MB/s 35.9 20.3 17.8 13.0 12.5  9.6  9.2  7.8
   ratio = mixed/old            1.00 1.01 1.04 0.95 1.08 0.96 1.09 1.10

                                          communicator size
   measurement count prot. unit   32   48   64   96  128  192  256
   ---------------------------------------------------------------
   latency         1 mixed  us  77.8 88.5 90.6  102                 1)
                       old  us  78.6 87.2 90.1 99.7  108  119  120
   ---------------------------------------------------------------
   bandwidth    128k mixed MB/s 35.1 23.3 34.1 22.8 34.4 22.4 33.9
   (=buf_lng/time)     old MB/s  6.0  6.0  6.0  5.2  5.2  4.6  4.6
   ration = mixed/old            5.8  3.9  5.7  4.4  6.6  4.8  7.4  5)
   ---------------------------------------------------------------
   limit                doubles  576  736  576  736  576  736  576  2)
   bandwidth   limit mixed MB/s  9.7  7.5  8.4  6.5  6.9  5.5  5.1  3)
                       old MB/s  7.7  6.4  6.4  5.7  5.5  4.9  4.7  3)
   ratio = mixed/old            1.26 1.17 1.31 1.14 1.26 1.12 1.08  4)

   ALLREDUCE:
                                          communicator size
   measurement count prot. unit    2    3    4    6    8   12   16   24
   --------------------------------------------------------------------
   latency         1 mixed  us  28.2 51.0 44.5 74.4 59.9  102 74.2  133
                       old  us  26.9 48.3 42.4 69.8 57.6 96.0 75.7  126
   --------------------------------------------------------------------
   bandwidth    128k mixed MB/s 74.0 29.4 42.4 23.3 35.0 20.9 32.8 19.7
   (=buf_lng/time)     old MB/s 20.9 14.4 13.2  9.7  9.6  7.3  7.4  5.8
   ration = mixed/old            3.5  2.0  3.2  2.4  3.6  2.9  4.4  3.4
   --------------------------------------------------------------------
   limit                doubles  448 1280  512  512  512  512  512  512
   bandwidth   limit mixed MB/s 26.4 15.1 16.2  8.2 12.4  7.2 10.8  5.7
                       old MB/s 26.1 14.9 15.0  9.1 10.5  6.7  7.3  5.3
   ratio = mixed/old            1.01 1.01 1.08 0.90 1.18 1.07 1.48 1.08

                                          communicator size
   measurement count prot. unit   32   48   64   96  128  192  256
   ---------------------------------------------------------------
   latency         1 mixed  us  90.3  162  109  190
                       old  us  92.7  152  104  179  122  225  135
   ---------------------------------------------------------------
   bandwidth    128k mixed MB/s 31.1 19.7 30.1 19.2 29.8 18.7 29.0
   (=buf_lng/time)     old MB/s  5.9  4.8  5.0  4.1  4.4  3.4  3.8
   ration = mixed/old            5.3  4.1  6.0  4.7  6.8  5.5  7.7
   ---------------------------------------------------------------
   limit                doubles  512  512  512  512  512  512  512
   bandwidth   limit mixed MB/s  6.6  5.6  5.7  3.5  4.4  3.2  3.8
                       old MB/s  6.3  4.2  5.4  3.6  4.4  3.1
   ratio = mixed/old            1.05 1.33 1.06 0.97 1.00 1.03

   Footnotes:
   1) This line shows that the overhead to decide which protocol
      should be used can be ignored.
   2) This line shows the limit for the count argument.
      If count < limit then the vendor protocol is used,
      otherwise the new protocol is used (see variable Ldb).
   3) These lines show the bandwidth (= buffer length / execution time)
      for both protocols.
   4) This line shows that the limit is chosen well if the ratio is
      between 0.95 (losing 5% for buffer length near and >=limit)
      and 1.10 (not gaining 10% for buffer length near and <limit).
   5) This line shows that the new protocol is 2..7 times faster
      for long counts.

*/

#ifdef REDUCE_LIMITS

#ifdef USE_Irecv
#define MPI_I_Sendrecv(sb, sc, sd, dest, st, rb, rc, rd, source, rt, comm, stat)                                       \
  {                                                                                                                    \
    MPI_Request req;                                                                                                   \
    req = Request::irecv(rb, rc, rd, source, rt, comm);                                                                \
    Request::send(sb, sc, sd, dest, st, comm);                                                                         \
    Request::wait(&req, stat);                                                                                         \
  }
#else
#ifdef USE_Isend
#define MPI_I_Sendrecv(sb, sc, sd, dest, st, rb, rc, rd, source, rt, comm, stat)                                       \
  {                                                                                                                    \
    MPI_Request req;                                                                                                   \
    req = mpi_mpi_isend(sb, sc, sd, dest, st, comm);                                                                   \
    Request::recv(rb, rc, rd, source, rt, comm, stat);                                                                 \
    Request::wait(&req, stat);                                                                                         \
  }
#else
#define MPI_I_Sendrecv(sb, sc, sd, dest, st, rb, rc, rd, source, rt, comm, stat)                                       \
  Request::sendrecv(sb, sc, sd, dest, st, rb, rc, rd, source, rt, comm, stat)
#endif
#endif

enum MPIM_Datatype {
  MPIM_SHORT,
  MPIM_INT,
  MPIM_LONG,
  MPIM_UNSIGNED_SHORT,
  MPIM_UNSIGNED,
  MPIM_UNSIGNED_LONG,
  MPIM_UNSIGNED_LONG_LONG,
  MPIM_FLOAT,
  MPIM_DOUBLE,
  MPIM_BYTE
};

enum MPIM_Op {
  MPIM_MAX,
  MPIM_MIN,
  MPIM_SUM,
  MPIM_PROD,
  MPIM_LAND,
  MPIM_BAND,
  MPIM_LOR,
  MPIM_BOR,
  MPIM_LXOR,
  MPIM_BXOR
};
#define MPI_I_DO_OP_C_INTEGER(MPI_I_do_op_TYPE, TYPE)                                                                  \
  static void MPI_I_do_op_TYPE(TYPE* b1, TYPE* b2, TYPE* rslt, int cnt, MPIM_Op op)                                    \
  {                                                                                                                    \
    int i;                                                                                                             \
    switch (op) {                                                                                                      \
      case MPIM_MAX:                                                                                                   \
        for (i = 0; i < cnt; i++)                                                                                      \
          rslt[i] = (b1[i] > b2[i] ? b1[i] : b2[i]);                                                                   \
        break;                                                                                                         \
      case MPIM_MIN:                                                                                                   \
        for (i = 0; i < cnt; i++)                                                                                      \
          rslt[i] = (b1[i] < b2[i] ? b1[i] : b2[i]);                                                                   \
        break;                                                                                                         \
      case MPIM_SUM:                                                                                                   \
        for (i = 0; i < cnt; i++)                                                                                      \
          rslt[i] = b1[i] + b2[i];                                                                                     \
        break;                                                                                                         \
      case MPIM_PROD:                                                                                                  \
        for (i = 0; i < cnt; i++)                                                                                      \
          rslt[i] = b1[i] * b2[i];                                                                                     \
        break;                                                                                                         \
      case MPIM_LAND:                                                                                                  \
        for (i = 0; i < cnt; i++)                                                                                      \
          rslt[i] = b1[i] && b2[i];                                                                                    \
        break;                                                                                                         \
      case MPIM_LOR:                                                                                                   \
        for (i = 0; i < cnt; i++)                                                                                      \
          rslt[i] = b1[i] || b2[i];                                                                                    \
        break;                                                                                                         \
      case MPIM_LXOR:                                                                                                  \
        for (i = 0; i < cnt; i++)                                                                                      \
          rslt[i] = b1[i] != b2[i];                                                                                    \
        break;                                                                                                         \
      case MPIM_BAND:                                                                                                  \
        for (i = 0; i < cnt; i++)                                                                                      \
          rslt[i] = b1[i] & b2[i];                                                                                     \
        break;                                                                                                         \
      case MPIM_BOR:                                                                                                   \
        for (i = 0; i < cnt; i++)                                                                                      \
          rslt[i] = b1[i] | b2[i];                                                                                     \
        break;                                                                                                         \
      case MPIM_BXOR:                                                                                                  \
        for (i = 0; i < cnt; i++)                                                                                      \
          rslt[i] = b1[i] ^ b2[i];                                                                                     \
        break;                                                                                                         \
      default:                                                                                                         \
        break;                                                                                                         \
    }                                                                                                                  \
  }

#define MPI_I_DO_OP_FP(MPI_I_do_op_TYPE, TYPE)                                                                         \
  static void MPI_I_do_op_TYPE(TYPE* b1, TYPE* b2, TYPE* rslt, int cnt, MPIM_Op op)                                    \
  {                                                                                                                    \
    int i;                                                                                                             \
    switch (op) {                                                                                                      \
      case MPIM_MAX:                                                                                                   \
        for (i = 0; i < cnt; i++)                                                                                      \
          rslt[i] = (b1[i] > b2[i] ? b1[i] : b2[i]);                                                                   \
        break;                                                                                                         \
      case MPIM_MIN:                                                                                                   \
        for (i = 0; i < cnt; i++)                                                                                      \
          rslt[i] = (b1[i] < b2[i] ? b1[i] : b2[i]);                                                                   \
        break;                                                                                                         \
      case MPIM_SUM:                                                                                                   \
        for (i = 0; i < cnt; i++)                                                                                      \
          rslt[i] = b1[i] + b2[i];                                                                                     \
        break;                                                                                                         \
      case MPIM_PROD:                                                                                                  \
        for (i = 0; i < cnt; i++)                                                                                      \
          rslt[i] = b1[i] * b2[i];                                                                                     \
        break;                                                                                                         \
      default:                                                                                                         \
        break;                                                                                                         \
    }                                                                                                                  \
  }

#define MPI_I_DO_OP_BYTE(MPI_I_do_op_TYPE, TYPE)                                                                       \
  static void MPI_I_do_op_TYPE(TYPE* b1, TYPE* b2, TYPE* rslt, int cnt, MPIM_Op op)                                    \
  {                                                                                                                    \
    int i;                                                                                                             \
    switch (op) {                                                                                                      \
      case MPIM_BAND:                                                                                                  \
        for (i = 0; i < cnt; i++)                                                                                      \
          rslt[i] = b1[i] & b2[i];                                                                                     \
        break;                                                                                                         \
      case MPIM_BOR:                                                                                                   \
        for (i = 0; i < cnt; i++)                                                                                      \
          rslt[i] = b1[i] | b2[i];                                                                                     \
        break;                                                                                                         \
      case MPIM_BXOR:                                                                                                  \
        for (i = 0; i < cnt; i++)                                                                                      \
          rslt[i] = b1[i] ^ b2[i];                                                                                     \
        break;                                                                                                         \
      default:                                                                                                         \
        break;                                                                                                         \
    }                                                                                                                  \
  }

MPI_I_DO_OP_C_INTEGER(MPI_I_do_op_short, short)
MPI_I_DO_OP_C_INTEGER(MPI_I_do_op_int, int)
MPI_I_DO_OP_C_INTEGER(MPI_I_do_op_long, long)
MPI_I_DO_OP_C_INTEGER(MPI_I_do_op_ushort, unsigned short)
MPI_I_DO_OP_C_INTEGER(MPI_I_do_op_uint, unsigned int)
MPI_I_DO_OP_C_INTEGER(MPI_I_do_op_ulong, unsigned long)
MPI_I_DO_OP_C_INTEGER(MPI_I_do_op_ulonglong, unsigned long long)
MPI_I_DO_OP_FP(MPI_I_do_op_float, float)
MPI_I_DO_OP_FP(MPI_I_do_op_double, double)
MPI_I_DO_OP_BYTE(MPI_I_do_op_byte, char)

#define MPI_I_DO_OP_CALL(MPI_I_do_op_TYPE, TYPE)                                                                       \
  MPI_I_do_op_TYPE((TYPE*)b1, (TYPE*)b2, (TYPE*)rslt, cnt, op);                                                        \
  break;

static void MPI_I_do_op(void* b1, void* b2, void* rslt, int cnt, MPIM_Datatype datatype, MPIM_Op op)
{
  switch (datatype) {
    case MPIM_SHORT:
      MPI_I_DO_OP_CALL(MPI_I_do_op_short, short)
    case MPIM_INT:
      MPI_I_DO_OP_CALL(MPI_I_do_op_int, int)
    case MPIM_LONG:
      MPI_I_DO_OP_CALL(MPI_I_do_op_long, long)
    case MPIM_UNSIGNED_SHORT:
      MPI_I_DO_OP_CALL(MPI_I_do_op_ushort, unsigned short)
    case MPIM_UNSIGNED:
      MPI_I_DO_OP_CALL(MPI_I_do_op_uint, unsigned int)
    case MPIM_UNSIGNED_LONG:
      MPI_I_DO_OP_CALL(MPI_I_do_op_ulong, unsigned long)
    case MPIM_UNSIGNED_LONG_LONG:
      MPI_I_DO_OP_CALL(MPI_I_do_op_ulonglong, unsigned long long)
    case MPIM_FLOAT:
      MPI_I_DO_OP_CALL(MPI_I_do_op_float, float)
    case MPIM_DOUBLE:
      MPI_I_DO_OP_CALL(MPI_I_do_op_double, double)
    case MPIM_BYTE:
      MPI_I_DO_OP_CALL(MPI_I_do_op_byte, char)
  }
}

REDUCE_LIMITS
namespace simgrid {
namespace smpi {
static int MPI_I_anyReduce(const void* Sendbuf, void* Recvbuf, int count, MPI_Datatype mpi_datatype, MPI_Op mpi_op,
                           int root, MPI_Comm comm, bool is_all)
{
  char *scr1buf, *scr2buf, *scr3buf, *xxx, *sendbuf, *recvbuf;
  int myrank, size, x_base, x_size, computed, idx;
  int x_start, x_count = 0, r, n, mynewrank, newroot, partner;
  int start_even[20], start_odd[20], count_even[20], count_odd[20];
  MPI_Aint typelng;
  MPI_Status status;
  size_t scrlng;
  int new_prot;
  MPIM_Datatype datatype = MPIM_INT; MPIM_Op op = MPIM_MAX;

  if     (mpi_datatype==MPI_SHORT         ) datatype=MPIM_SHORT;
  else if(mpi_datatype==MPI_INT           ) datatype=MPIM_INT;
  else if(mpi_datatype==MPI_LONG          ) datatype=MPIM_LONG;
  else if(mpi_datatype==MPI_UNSIGNED_SHORT) datatype=MPIM_UNSIGNED_SHORT;
  else if(mpi_datatype==MPI_UNSIGNED      ) datatype=MPIM_UNSIGNED;
  else if(mpi_datatype==MPI_UNSIGNED_LONG ) datatype=MPIM_UNSIGNED_LONG;
  else if(mpi_datatype==MPI_UNSIGNED_LONG_LONG ) datatype=MPIM_UNSIGNED_LONG_LONG;
  else if(mpi_datatype==MPI_FLOAT         ) datatype=MPIM_FLOAT;
  else if(mpi_datatype==MPI_DOUBLE        ) datatype=MPIM_DOUBLE;
  else if(mpi_datatype==MPI_BYTE          ) datatype=MPIM_BYTE;
  else
    throw std::invalid_argument("reduce rab algorithm can't be used with this datatype!");

  if     (mpi_op==MPI_MAX     ) op=MPIM_MAX;
  else if(mpi_op==MPI_MIN     ) op=MPIM_MIN;
  else if(mpi_op==MPI_SUM     ) op=MPIM_SUM;
  else if(mpi_op==MPI_PROD    ) op=MPIM_PROD;
  else if(mpi_op==MPI_LAND    ) op=MPIM_LAND;
  else if(mpi_op==MPI_BAND    ) op=MPIM_BAND;
  else if(mpi_op==MPI_LOR     ) op=MPIM_LOR;
  else if(mpi_op==MPI_BOR     ) op=MPIM_BOR;
  else if(mpi_op==MPI_LXOR    ) op=MPIM_LXOR;
  else if(mpi_op==MPI_BXOR    ) op=MPIM_BXOR;

  new_prot = 0;
  MPI_Comm_size(comm, &size);
  if (size > 1) /*otherwise no balancing_protocol*/
  { int ss;
    if      (size==2) ss=0;
    else if (size==3) ss=1;
    else { int s = size; while (!(s & 1)) s = s >> 1;
           if (s==1) /* size == power of 2 */ ss = 2;
           else      /* size != power of 2 */ ss = 3; }
    switch(op) {
     case MPIM_MAX:   case MPIM_MIN: case MPIM_SUM:  case MPIM_PROD:
     case MPIM_LAND:  case MPIM_LOR: case MPIM_LXOR:
     case MPIM_BAND:  case MPIM_BOR: case MPIM_BXOR:
      switch(datatype) {
        case MPIM_SHORT:  case MPIM_UNSIGNED_SHORT:
         new_prot = count >= Lsh[is_all][ss]; break;
        case MPIM_INT:    case MPIM_UNSIGNED:
         new_prot = count >= Lin[is_all][ss]; break;
        case MPIM_LONG:   case MPIM_UNSIGNED_LONG: case MPIM_UNSIGNED_LONG_LONG:
         new_prot = count >= Llg[is_all][ss]; break;
     default:
        break;
    } default:
        break;}
    switch(op) {
     case MPIM_MAX:  case MPIM_MIN: case MPIM_SUM: case MPIM_PROD:
      switch(datatype) {
        case MPIM_FLOAT:
         new_prot = count >= Lfp[is_all][ss]; break;
        case MPIM_DOUBLE:
         new_prot = count >= Ldb[is_all][ss]; break;
              default:
        break;
    } default:
        break;}
    switch(op) {
     case MPIM_BAND:  case MPIM_BOR: case MPIM_BXOR:
      switch(datatype) {
        case MPIM_BYTE:
         new_prot = count >= Lby[is_all][ss]; break;
              default:
        break;
    } default:
        break;}
#   ifdef DEBUG
    { char *ss_str[]={"two","three","power of 2","no power of 2"};
      printf("MPI_(All)Reduce: is_all=%1d, size=%1d=%s, new_prot=%1d\n",
             is_all, size, ss_str[ss], new_prot); fflush(stdout);
    }
#   endif
  }

  if (new_prot)
  {
    sendbuf = (char*) Sendbuf;
    recvbuf = (char*) Recvbuf;
    MPI_Comm_rank(comm, &myrank);
    MPI_Type_extent(mpi_datatype, &typelng);
    scrlng  = typelng * count;
#ifdef NO_CACHE_OPTIMIZATION
    scr1buf = new char[scrlng];
    scr2buf = new char[scrlng];
    scr3buf = new char[scrlng];
#else
#  ifdef SCR_LNG_OPTIM
    scrlng = SCR_LNG_OPTIM(scrlng);
#  endif
    scr2buf = new char[3 * scrlng]; /* To test cache problems.     */
    scr1buf = scr2buf + 1*scrlng; /* scr1buf and scr3buf must not*/
    scr3buf = scr2buf + 2*scrlng; /* be used for malloc because  */
                                  /* they are interchanged below.*/
#endif
    computed = 0;
    if (is_all) root = myrank; /* for correct recvbuf handling */

  /*...step 1 */

#   ifdef DEBUG
     printf("[%2d] step 1 begin\n",myrank); fflush(stdout);
#   endif
    n = 0; x_size = 1;
    while (2*x_size <= size) { n++; x_size = x_size * 2; }
    /* x_size == 2**n */
    r = size - x_size;

  /*...step 2 */

#   ifdef DEBUG
    printf("[%2d] step 2 begin n=%d, r=%d\n",myrank,n,r);fflush(stdout);
#   endif
    if (myrank < 2*r)
    {
      if ((myrank % 2) == 0 /*even*/)
      {
        MPI_I_Sendrecv(sendbuf + (count/2)*typelng,
                       count - count/2, mpi_datatype, myrank+1, 1220,
                       scr2buf, count/2,mpi_datatype, myrank+1, 1221,
                       comm, &status);
        MPI_I_do_op(sendbuf, scr2buf, scr1buf,
                    count/2, datatype, op);
        Request::recv(scr1buf + (count/2)*typelng, count - count/2,
                 mpi_datatype, myrank+1, 1223, comm, &status);
        computed = 1;
#       ifdef DEBUG
        { int i; printf("[%2d] after step 2: val=",
                        myrank);
          for (i=0; i<count; i++)
           printf(" %5.0lf",((double*)scr1buf)[i] );
          printf("\n"); fflush(stdout);
        }
#       endif
      }
      else /*odd*/
      {
        MPI_I_Sendrecv(sendbuf, count/2,mpi_datatype, myrank-1, 1221,
                       scr2buf + (count/2)*typelng,
                       count - count/2, mpi_datatype, myrank-1, 1220,
                       comm, &status);
        MPI_I_do_op(scr2buf + (count/2)*typelng,
                    sendbuf + (count/2)*typelng,
                    scr1buf + (count/2)*typelng,
                    count - count/2, datatype, op);
        Request::send(scr1buf + (count/2)*typelng, count - count/2,
                 mpi_datatype, myrank-1, 1223, comm);
      }
    }

  /*...step 3+4 */

#   ifdef DEBUG
     printf("[%2d] step 3+4 begin\n",myrank); fflush(stdout);
#   endif
    if ((myrank >= 2*r) || ((myrank%2 == 0)  &&  (myrank < 2*r)))
         mynewrank = (myrank < 2*r ? myrank/2 : myrank-r);
    else mynewrank = -1;

    if (mynewrank >= 0)
    { /* begin -- only for nodes with new rank */

#     define OLDRANK(new)   ((new) < r ? (new)*2 : (new)+r)

  /*...step 5 */

      x_start = 0;
      x_count = count;
      for (idx=0, x_base=1; idx<n; idx++, x_base=x_base*2)
      {
        start_even[idx] = x_start;
        count_even[idx] = x_count / 2;
        start_odd [idx] = x_start + count_even[idx];
        count_odd [idx] = x_count - count_even[idx];
        if (((mynewrank/x_base) % 2) == 0 /*even*/)
        {
#         ifdef DEBUG
            printf("[%2d](%2d) step 5.%d begin even c=%1d\n",
                   myrank,mynewrank,idx+1,computed); fflush(stdout);
#         endif
          x_start = start_even[idx];
          x_count = count_even[idx];
          MPI_I_Sendrecv((computed ? scr1buf : sendbuf)
                         + start_odd[idx]*typelng, count_odd[idx],
                         mpi_datatype, OLDRANK(mynewrank+x_base), 1231,
                         scr2buf + x_start*typelng, x_count,
                         mpi_datatype, OLDRANK(mynewrank+x_base), 1232,
                         comm, &status);
          MPI_I_do_op((computed?scr1buf:sendbuf) + x_start*typelng,
                      scr2buf                    + x_start*typelng,
                      ((root==myrank) && (idx==(n-1))
                        ? recvbuf + x_start*typelng
                        : scr3buf + x_start*typelng),
                      x_count, datatype, op);
        }
        else /*odd*/
        {
#         ifdef DEBUG
            printf("[%2d](%2d) step 5.%d begin  odd c=%1d\n",
                   myrank,mynewrank,idx+1,computed); fflush(stdout);
#         endif
          x_start = start_odd[idx];
          x_count = count_odd[idx];
          MPI_I_Sendrecv((computed ? scr1buf : sendbuf)
                         +start_even[idx]*typelng, count_even[idx],
                         mpi_datatype, OLDRANK(mynewrank-x_base), 1232,
                         scr2buf + x_start*typelng, x_count,
                         mpi_datatype, OLDRANK(mynewrank-x_base), 1231,
                         comm, &status);
          MPI_I_do_op(scr2buf                    + x_start*typelng,
                      (computed?scr1buf:sendbuf) + x_start*typelng,
                      ((root==myrank) && (idx==(n-1))
                        ? recvbuf + x_start*typelng
                        : scr3buf + x_start*typelng),
                      x_count, datatype, op);
        }
        xxx = scr3buf; scr3buf = scr1buf; scr1buf = xxx;
        computed = 1;
#       ifdef DEBUG
        { int i; printf("[%2d](%2d) after step 5.%d   end: start=%2d  count=%2d  val=",
                        myrank,mynewrank,idx+1,x_start,x_count);
          for (i=0; i<x_count; i++)
           printf(" %5.0lf",((double*)((root==myrank)&&(idx==(n-1))
                             ? recvbuf + x_start*typelng
                             : scr1buf + x_start*typelng))[i] );
          printf("\n"); fflush(stdout);
        }
#       endif
      } /*for*/

#     undef OLDRANK


    } /* end -- only for nodes with new rank */

    if (is_all)
    {
      /*...steps 6.1 to 6.n */

      if (mynewrank >= 0)
      { /* begin -- only for nodes with new rank */

#       define OLDRANK(new)   ((new) < r ? (new)*2 : (new)+r)

        for(idx=n-1, x_base=x_size/2; idx>=0; idx--, x_base=x_base/2)
        {
#         ifdef DEBUG
            printf("[%2d](%2d) step 6.%d begin\n",myrank,mynewrank,n-idx); fflush(stdout);
#         endif
          if (((mynewrank/x_base) % 2) == 0 /*even*/)
          {
            MPI_I_Sendrecv(recvbuf + start_even[idx]*typelng,
                                     count_even[idx],
                           mpi_datatype, OLDRANK(mynewrank+x_base),1241,
                           recvbuf + start_odd[idx]*typelng,
                                     count_odd[idx],
                           mpi_datatype, OLDRANK(mynewrank+x_base),1242,
                           comm, &status);
#           ifdef DEBUG
              x_start = start_odd[idx];
              x_count = count_odd[idx];
#           endif
          }
          else /*odd*/
          {
            MPI_I_Sendrecv(recvbuf + start_odd[idx]*typelng,
                                     count_odd[idx],
                           mpi_datatype, OLDRANK(mynewrank-x_base),1242,
                           recvbuf + start_even[idx]*typelng,
                                     count_even[idx],
                           mpi_datatype, OLDRANK(mynewrank-x_base),1241,
                           comm, &status);
#           ifdef DEBUG
              x_start = start_even[idx];
              x_count = count_even[idx];
#           endif
          }
#         ifdef DEBUG
          { int i; printf("[%2d](%2d) after step 6.%d   end: start=%2d  count=%2d  val=",
                          myrank,mynewrank,n-idx,x_start,x_count);
            for (i=0; i<x_count; i++)
             printf(" %5.0lf",((double*)(root==myrank
                               ? recvbuf + x_start*typelng
                               : scr1buf + x_start*typelng))[i] );
            printf("\n"); fflush(stdout);
          }
#         endif
        } /*for*/

#       undef OLDRANK

      } /* end -- only for nodes with new rank */

      /*...step 7 */

      if (myrank < 2*r)
      {
#       ifdef DEBUG
          printf("[%2d] step 7 begin\n",myrank); fflush(stdout);
#       endif
        if (myrank%2 == 0 /*even*/)
          Request::send(recvbuf, count, mpi_datatype, myrank+1, 1253, comm);
        else /*odd*/
          Request::recv(recvbuf, count, mpi_datatype, myrank-1, 1253, comm, &status);
      }

    }
    else /* not is_all, i.e. Reduce */
    {

    /*...step 6.0 */

      if ((root < 2*r) && (root%2 == 1))
      {
#       ifdef DEBUG
          printf("[%2d] step 6.0 begin\n",myrank); fflush(stdout);
#       endif
        if (myrank == 0) /* then mynewrank==0, x_start==0
                                 x_count == count/x_size  */
        {
          Request::send(scr1buf,x_count,mpi_datatype,root,1241,comm);
          mynewrank = -1;
        }

        if (myrank == root)
        {
          mynewrank = 0;
          x_start = 0;
          x_count = count;
          for (idx=0, x_base=1; idx<n; idx++, x_base=x_base*2)
          {
            start_even[idx] = x_start;
            count_even[idx] = x_count / 2;
            start_odd [idx] = x_start + count_even[idx];
            count_odd [idx] = x_count - count_even[idx];
            /* code for always even in each bit of mynewrank: */
            x_start = start_even[idx];
            x_count = count_even[idx];
          }
          Request::recv(recvbuf,x_count,mpi_datatype,0,1241,comm,&status);
        }
        newroot = 0;
      }
      else
      {
        newroot = (root < 2*r ? root/2 : root-r);
      }

    /*...steps 6.1 to 6.n */

      if (mynewrank >= 0)
      { /* begin -- only for nodes with new rank */

#define OLDRANK(new) ((new) == newroot ? root : ((new) < r ? (new) * 2 : (new) + r))

        for(idx=n-1, x_base=x_size/2; idx>=0; idx--, x_base=x_base/2)
        {
#         ifdef DEBUG
            printf("[%2d](%2d) step 6.%d begin\n",myrank,mynewrank,n-idx); fflush(stdout);
#         endif
          if ((mynewrank & x_base) != (newroot & x_base))
          {
            if (((mynewrank/x_base) % 2) == 0 /*even*/)
            { x_start = start_even[idx]; x_count = count_even[idx];
              partner = mynewrank+x_base; }
            else
            { x_start = start_odd[idx]; x_count = count_odd[idx];
              partner = mynewrank-x_base; }
            Request::send(scr1buf + x_start*typelng, x_count, mpi_datatype,
                     OLDRANK(partner), 1244, comm);
          }
          else /*odd*/
          {
            if (((mynewrank/x_base) % 2) == 0 /*even*/)
            { x_start = start_odd[idx]; x_count = count_odd[idx];
              partner = mynewrank+x_base; }
            else
            { x_start = start_even[idx]; x_count = count_even[idx];
              partner = mynewrank-x_base; }
            Request::recv((myrank==root ? recvbuf : scr1buf)
                     + x_start*typelng, x_count, mpi_datatype,
                     OLDRANK(partner), 1244, comm, &status);
#           ifdef DEBUG
            { int i; printf("[%2d](%2d) after step 6.%d   end: start=%2d  count=%2d  val=",
                            myrank,mynewrank,n-idx,x_start,x_count);
              for (i=0; i<x_count; i++)
               printf(" %5.0lf",((double*)(root==myrank
                                 ? recvbuf + x_start*typelng
                                 : scr1buf + x_start*typelng))[i] );
              printf("\n"); fflush(stdout);
            }
#           endif
          }
        } /*for*/

#       undef OLDRANK

      } /* end -- only for nodes with new rank */
    }

#   ifdef NO_CACHE_TESTING
    delete[] scr1buf;
    delete[] scr2buf;
    delete[] scr3buf;
#   else
    delete[] scr2buf;             /* scr1buf and scr3buf are part of scr2buf */
#   endif
    return(MPI_SUCCESS);
  } /* new_prot */
  /*otherwise:*/
  if (is_all)
    return (colls::allreduce(Sendbuf, Recvbuf, count, mpi_datatype, mpi_op, comm));
  else
    return (colls::reduce(Sendbuf, Recvbuf, count, mpi_datatype, mpi_op, root, comm));
}
#endif /*REDUCE_LIMITS*/

int reduce__rab(const void* Sendbuf, void* Recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root,
                MPI_Comm comm)
{
  return MPI_I_anyReduce(Sendbuf, Recvbuf, count, datatype, op, root, comm, false);
}

int allreduce__rab(const void* Sendbuf, void* Recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  return MPI_I_anyReduce(Sendbuf, Recvbuf, count, datatype, op, -1, comm, true);
}
}
}
