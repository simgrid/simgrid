/* selector for collective algorithms based on mpich decision logic */

/* Copyright (c) 2009-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "colls_private.hpp"

#include <memory>

/* This is the default implementation of allreduce. The algorithm is:

   Algorithm: MPI_Allreduce

   For the heterogeneous case, we call MPI_Reduce followed by MPI_Bcast
   in order to meet the requirement that all processes must have the
   same result. For the homogeneous case, we use the following algorithms.


   For long messages and for builtin ops and if count >= pof2 (where
   pof2 is the nearest power-of-two less than or equal to the number
   of processes), we use Rabenseifner's algorithm (see
   http://www.hlrs.de/mpi/myreduce.html).
   This algorithm implements the allreduce in two steps: first a
   reduce-scatter, followed by an allgather. A recursive-halving
   algorithm (beginning with processes that are distance 1 apart) is
   used for the reduce-scatter, and a recursive doubling
   algorithm is used for the allgather. The non-power-of-two case is
   handled by dropping to the nearest lower power-of-two: the first
   few even-numbered processes send their data to their right neighbors
   (rank+1), and the reduce-scatter and allgather happen among the remaining
   power-of-two processes. At the end, the first few even-numbered
   processes get the result from their right neighbors.

   For the power-of-two case, the cost for the reduce-scatter is
   lgp.alpha + n.((p-1)/p).beta + n.((p-1)/p).gamma. The cost for the
   allgather lgp.alpha + n.((p-1)/p).beta. Therefore, the
   total cost is:
   Cost = 2.lgp.alpha + 2.n.((p-1)/p).beta + n.((p-1)/p).gamma

   For the non-power-of-two case,
   Cost = (2.floor(lgp)+2).alpha + (2.((p-1)/p) + 2).n.beta + n.(1+(p-1)/p).gamma


   For short messages, for user-defined ops, and for count < pof2
   we use a recursive doubling algorithm (similar to the one in
   MPI_Allgather). We use this algorithm in the case of user-defined ops
   because in this case derived datatypes are allowed, and the user
   could pass basic datatypes on one process and derived on another as
   long as the type maps are the same. Breaking up derived datatypes
   to do the reduce-scatter is tricky.

   Cost = lgp.alpha + n.lgp.beta + n.lgp.gamma

   Possible improvements:

   End Algorithm: MPI_Allreduce
*/
namespace simgrid::smpi {
int allreduce__mpich(const void *sbuf, void *rbuf, int count,
                        MPI_Datatype dtype, MPI_Op op, MPI_Comm comm)
{
    size_t dsize, block_dsize;

    dsize = dtype->size();
    block_dsize = dsize * count;

    /* Single-node communicators use the shared-memory algorithm (MPICH's
       release-gather branch on CH4/SHM); everything else follows the
       MSG_SIZE/BUILTIN_OP tree of MPICH's allreduce-intra-auto. */
    if (comm->get_leaders_comm() == MPI_COMM_NULL)
      comm->init_smp();
    if (comm->get_leaders_comm() != MPI_COMM_NULL && comm->get_leaders_comm()->size() == 1)
      return allreduce__smp_rsag(sbuf, rbuf, count, dtype, op, comm);

    if (block_dsize < 8)
      return allreduce__rdb(sbuf, rbuf, count, dtype, op, comm);

    if (op != MPI_OP_NULL && not op->is_predefined())
      return allreduce__rdb(sbuf, rbuf, count, dtype, op, comm);

    return allreduce__rab_rdb(sbuf, rbuf, count, dtype, op, comm);
}


/* This is the default implementation of alltoall. The algorithm is:

   Algorithm: MPI_Alltoall

   We use four algorithms for alltoall. For short messages and
   (comm_size >= 8), we use the algorithm by Jehoshua Bruck et al,
   IEEE TPDS, Nov. 1997. It is a store-and-forward algorithm that
   takes lgp steps. Because of the extra communication, the bandwidth
   requirement is (n/2).lgp.beta.

   Cost = lgp.alpha + (n/2).lgp.beta

   where n is the total amount of data a process needs to send to all
   other processes.

   For medium size messages and (short messages for comm_size < 8), we
   use an algorithm that posts all irecvs and isends and then does a
   waitall. We scatter the order of sources and destinations among the
   processes, so that all processes don't try to send/recv to/from the
   same process at the same time.

   *** Modification: We post only a small number of isends and irecvs
   at a time and wait on them as suggested by Tony Ladd. ***
   *** See comments below about an additional modification that
   we may want to consider ***

   For long messages and power-of-two number of processes, we use a
   pairwise exchange algorithm, which takes p-1 steps. We
   calculate the pairs by using an exclusive-or algorithm:
           for (i=1; i<comm_size; i++)
               dest = rank ^ i;
   This algorithm doesn't work if the number of processes is not a power of
   two. For a non-power-of-two number of processes, we use an
   algorithm in which, in step i, each process  receives from (rank-i)
   and sends to (rank+i).

   Cost = (p-1).alpha + n.beta

   where n is the total amount of data a process needs to send to all
   other processes.

   Possible improvements:

   End Algorithm: MPI_Alltoall
*/

int alltoall__mpich(const void *sbuf, int scount,
                    MPI_Datatype sdtype,
                    void* rbuf, int rcount,
                    MPI_Datatype rdtype,
                    MPI_Comm comm)
{
    int communicator_size;
    size_t dsize, block_dsize;
    communicator_size = comm->size();

    dsize = sdtype->size();
    block_dsize = dsize * scount;

    /* Selection mirrors MPICH's alltoall-intra-auto tuning table.
       MPI_IN_PLACE is normalized into a real send buffer by the PMPI
       wrapper, so the inplace branch has no equivalent here. */
    if (communicator_size < 8) {
        if (block_dsize <= 32768)
            return alltoall__scattered(sbuf, scount, sdtype, rbuf, rcount, rdtype, comm);
        return alltoall__pairwise(sbuf, scount, sdtype, rbuf, rcount, rdtype, comm);
    }

    if (block_dsize <= 256)
        return alltoall__bruck(sbuf, scount, sdtype, rbuf, rcount, rdtype, comm);
    if (block_dsize <= 32768)
        return alltoall__scattered(sbuf, scount, sdtype, rbuf, rcount, rdtype, comm);
    return alltoall__pairwise(sbuf, scount, sdtype, rbuf, rcount, rdtype, comm);
}

int alltoallv__mpich(const void *sbuf, const int *scounts, const int *sdisps,
                     MPI_Datatype sdtype,
                     void *rbuf, const int *rcounts, const int *rdisps,
                     MPI_Datatype rdtype,
                     MPI_Comm  comm
                     )
{
    /* MPICH's alltoallv-intra-auto always picks the scattered algorithm;
       MPI_IN_PLACE is normalized into a real send buffer by the PMPI wrapper. */
    return alltoallv__scattered(sbuf, scounts, sdisps, sdtype,
                                rbuf, rcounts, rdisps, rdtype,
                                comm);
}


int barrier__mpich(MPI_Comm  comm)
{
    return barrier__k_dissemination2(comm);
}

/* This is the default implementation of broadcast. The algorithm is:

   Algorithm: MPI_Bcast

   For short messages, we use a binomial tree algorithm.
   Cost = lgp.alpha + n.lgp.beta

   For long messages, we do a scatter followed by an allgather.
   We first scatter the buffer using a binomial tree algorithm. This costs
   lgp.alpha + n.((p-1)/p).beta
   If the datatype is contiguous and the communicator is homogeneous,
   we treat the data as bytes and divide (scatter) it among processes
   by using ceiling division. For the noncontiguous or heterogeneous
   cases, we first pack the data into a temporary buffer by using
   MPI_Pack, scatter it as bytes, and unpack it after the allgather.

   For the allgather, we use a recursive doubling algorithm for
   medium-size messages and power-of-two number of processes. This
   takes lgp steps. In each step pairs of processes exchange all the
   data they have (we take care of non-power-of-two situations). This
   costs approximately lgp.alpha + n.((p-1)/p).beta. (Approximately
   because it may be slightly more in the non-power-of-two case, but
   it's still a logarithmic algorithm.) Therefore, for long messages
   Total Cost = 2.lgp.alpha + 2.n.((p-1)/p).beta

   Note that this algorithm has twice the latency as the tree algorithm
   we use for short messages, but requires lower bandwidth: 2.n.beta
   versus n.lgp.beta. Therefore, for long messages and when lgp > 2,
   this algorithm will perform better.

   For long messages and for medium-size messages and non-power-of-two
   processes, we use a ring algorithm for the allgather, which
   takes p-1 steps, because it performs better than recursive doubling.
   Total Cost = (lgp+p-1).alpha + 2.n.((p-1)/p).beta

   Possible improvements:
   For clusters of SMPs, we may want to do something differently to
   take advantage of shared memory on each node.

   End Algorithm: MPI_Bcast
*/


int bcast__mpich(void *buff, int count,
                 MPI_Datatype datatype, int root,
                 MPI_Comm  comm
                                          )
{
    /* Tuning table of MPICH's bcast-intra-auto. */
    const size_t small_message_size = 12288;
    const size_t intermediate_message_size = 524288;

    const int communicator_size = comm->size();
    const size_t message_size = datatype->size() * static_cast<size_t>(count);

    if (communicator_size < 8)
        return bcast__binomial_tree(buff, count, datatype, root, comm);

    const bool pof2 = (communicator_size & (communicator_size - 1)) == 0;

    if (message_size <= small_message_size)
        return bcast__binomial_tree(buff, count, datatype, root, comm);

    if (pof2 && message_size <= intermediate_message_size)
        return bcast__scatter_rdb_allgather(buff, count, datatype, root, comm);

    return bcast__scatter_LR_allgather(buff, count, datatype, root, comm);
}

/* This is the default implementation of reduce. The algorithm is:

   Algorithm: MPI_Reduce

   For long messages and for builtin ops and if count >= pof2 (where
   pof2 is the nearest power-of-two less than or equal to the number
   of processes), we use Rabenseifner's algorithm (see
   http://www.hlrs.de/organization/par/services/models/mpi/myreduce.html ).
   This algorithm implements the reduce in two steps: first a
   reduce-scatter, followed by a gather to the root. A
   recursive-halving algorithm (beginning with processes that are
   distance 1 apart) is used for the reduce-scatter, and a binomial tree
   algorithm is used for the gather. The non-power-of-two case is
   handled by dropping to the nearest lower power-of-two: the first
   few odd-numbered processes send their data to their left neighbors
   (rank-1), and the reduce-scatter happens among the remaining
   power-of-two processes. If the root is one of the excluded
   processes, then after the reduce-scatter, rank 0 sends its result to
   the root and exits; the root now acts as rank 0 in the binomial tree
   algorithm for gather.

   For the power-of-two case, the cost for the reduce-scatter is
   lgp.alpha + n.((p-1)/p).beta + n.((p-1)/p).gamma. The cost for the
   gather to root is lgp.alpha + n.((p-1)/p).beta. Therefore, the
   total cost is:
   Cost = 2.lgp.alpha + 2.n.((p-1)/p).beta + n.((p-1)/p).gamma

   For the non-power-of-two case, assuming the root is not one of the
   odd-numbered processes that get excluded in the reduce-scatter,
   Cost = (2.floor(lgp)+1).alpha + (2.((p-1)/p) + 1).n.beta +
           n.(1+(p-1)/p).gamma


   For short messages, user-defined ops, and count < pof2, we use a
   binomial tree algorithm for both short and long messages.

   Cost = lgp.alpha + n.lgp.beta + n.lgp.gamma


   We use the binomial tree algorithm in the case of user-defined ops
   because in this case derived datatypes are allowed, and the user
   could pass basic datatypes on one process and derived on another as
   long as the type maps are the same. Breaking up derived datatypes
   to do the reduce-scatter is tricky.

   FIXME: Per the MPI-2.1 standard this case is not possible.  We
   should be able to use the reduce-scatter/gather approach as long as
   count >= pof2.  [goodell@ 2009-01-21]

   Possible improvements:

   End Algorithm: MPI_Reduce
*/


int reduce__mpich(const void *sendbuf, void *recvbuf,
                                            int count, MPI_Datatype  datatype,
                                            MPI_Op   op, int root,
                                            MPI_Comm   comm
                                            )
{
    int communicator_size=0;
    size_t message_size, dsize;

    if (not comm->is_smp_comm()) {
      if(comm->get_leaders_comm()==MPI_COMM_NULL){
        comm->init_smp();
      }
      if (op->is_commutative() == 1)
        return reduce__mvapich2_two_level(sendbuf, recvbuf, count, datatype, op, root, comm);
    }

    communicator_size = comm->size();

    /* need data size for decision function */
    dsize=datatype->size();
    message_size = dsize * count;   /* needed for decision */

    int pof2 = 1;
    while (pof2 <= communicator_size) pof2 <<= 1;
    pof2 >>= 1;

    if ((count < pof2) || (message_size < 2048) || (op != MPI_OP_NULL && not op->is_commutative())) {
      return reduce__binomial(sendbuf, recvbuf, count, datatype, op, root, comm);
    }
    return reduce__scatter_gather(sendbuf, recvbuf, count, datatype, op, root, comm);
}



/* This is the default implementation of reduce_scatter. The algorithm is:

   Algorithm: MPI_Reduce_scatter

   If the operation is commutative, for short and medium-size
   messages, we use a recursive-halving
   algorithm in which the first p/2 processes send the second n/2 data
   to their counterparts in the other half and receive the first n/2
   data from them. This procedure continues recursively, halving the
   data communicated at each step, for a total of lgp steps. If the
   number of processes is not a power-of-two, we convert it to the
   nearest lower power-of-two by having the first few even-numbered
   processes send their data to the neighboring odd-numbered process
   at (rank+1). Those odd-numbered processes compute the result for
   their left neighbor as well in the recursive halving algorithm, and
   then at  the end send the result back to the processes that didn't
   participate.
   Therefore, if p is a power-of-two,
   Cost = lgp.alpha + n.((p-1)/p).beta + n.((p-1)/p).gamma
   If p is not a power-of-two,
   Cost = (floor(lgp)+2).alpha + n.(1+(p-1+n)/p).beta + n.(1+(p-1)/p).gamma
   The above cost in the non power-of-two case is approximate because
   there is some imbalance in the amount of work each process does
   because some processes do the work of their neighbors as well.

   For commutative operations and very long messages we use
   we use a pairwise exchange algorithm similar to
   the one used in MPI_Alltoall. At step i, each process sends n/p
   amount of data to (rank+i) and receives n/p amount of data from
   (rank-i).
   Cost = (p-1).alpha + n.((p-1)/p).beta + n.((p-1)/p).gamma


   If the operation is not commutative, we do the following:

   We use a recursive doubling algorithm, which
   takes lgp steps. At step 1, processes exchange (n-n/p) amount of
   data; at step 2, (n-2n/p) amount of data; at step 3, (n-4n/p)
   amount of data, and so forth.

   Cost = lgp.alpha + n.(lgp-(p-1)/p).beta + n.(lgp-(p-1)/p).gamma

   Possible improvements:

   End Algorithm: MPI_Reduce_scatter
*/


int reduce_scatter__mpich(const void *sbuf, void *rbuf,
                          const int *rcounts,
                          MPI_Datatype dtype,
                          MPI_Op  op,
                          MPI_Comm  comm
                          )
{
    int comm_size, i;
    size_t total_message_size;

    if(sbuf==rbuf)sbuf=MPI_IN_PLACE; //restore MPI_IN_PLACE as these algorithms handle it

    XBT_DEBUG("Coll_reduce_scatter_mpich::reduce");

    comm_size = comm->size();
    // We need data size for decision function
    total_message_size = 0;
    for (i = 0; i < comm_size; i++) {
        total_message_size += rcounts[i];
    }

    if( (op==MPI_OP_NULL || op->is_commutative()) &&  total_message_size > 524288) {
        return reduce_scatter__mpich_pair(sbuf, rbuf, rcounts, dtype, op, comm);
    } else if ((op != MPI_OP_NULL && not op->is_commutative())) {
      bool is_block_regular = true;
      for (i = 0; i < (comm_size - 1); ++i) {
        if (rcounts[i] != rcounts[i + 1]) {
          is_block_regular = false;
          break;
        }
      }

      /* slightly retask pof2 to mean pof2 equal or greater, not always greater as it is above */
      int pof2 = 1;
      while (pof2 < comm_size)
        pof2 <<= 1;

      if (pof2 == comm_size && is_block_regular) {
        /* noncommutative, pof2 size, and block regular */
        return reduce_scatter__mpich_noncomm(sbuf, rbuf, rcounts, dtype, op, comm);
      }

      return reduce_scatter__mpich_rdb(sbuf, rbuf, rcounts, dtype, op, comm);
    }else{
       return reduce_scatter__mpich_rdb(sbuf, rbuf, rcounts, dtype, op, comm);
    }
}


/* This is the default implementation of allgather. The algorithm is:

   Algorithm: MPI_Allgather

   For short messages and non-power-of-two no. of processes, we use
   the algorithm from the Jehoshua Bruck et al IEEE TPDS Nov 97
   paper. It is a variant of the disemmination algorithm for
   barrier. It takes ceiling(lg p) steps.

   Cost = lgp.alpha + n.((p-1)/p).beta
   where n is total size of data gathered on each process.

   For short or medium-size messages and power-of-two no. of
   processes, we use the recursive doubling algorithm.

   Cost = lgp.alpha + n.((p-1)/p).beta

   TODO: On TCP, we may want to use recursive doubling instead of the Bruck
   algorithm in all cases because of the pairwise-exchange property of
   recursive doubling (see Benson et al paper in Euro PVM/MPI
   2003).

   It is interesting to note that either of the above algorithms for
   MPI_Allgather has the same cost as the tree algorithm for MPI_Gather!

   For long messages or medium-size messages and non-power-of-two
   no. of processes, we use a ring algorithm. In the first step, each
   process i sends its contribution to process i+1 and receives
   the contribution from process i-1 (with wrap-around). From the
   second step onwards, each process i forwards to process i+1 the
   data it received from process i-1 in the previous step. This takes
   a total of p-1 steps.

   Cost = (p-1).alpha + n.((p-1)/p).beta

   We use this algorithm instead of recursive doubling for long
   messages because we find that this communication pattern (nearest
   neighbor) performs twice as fast as recursive doubling for long
   messages (on Myrinet and IBM SP).

   Possible improvements:

   End Algorithm: MPI_Allgather
*/

int allgather__mpich(const void *sbuf, int scount,
                     MPI_Datatype sdtype,
                     void* rbuf, int rcount,
                     MPI_Datatype rdtype,
                     MPI_Comm  comm
                     )
{
    int communicator_size, pow2_size;
    size_t dsize, total_dsize;

    communicator_size = comm->size();

    /* Per-rank message size, as used by the MPICH selection tables */
    dsize=sdtype->size();
    total_dsize = dsize * scount;

    for (pow2_size  = 1; pow2_size < communicator_size; pow2_size <<=1);

    /* Selection mirrors MPICH's allgather-intra-auto tuning table:
       power-of-two communicators pick recursive doubling for small messages
       and ring otherwise, with thresholds depending on the communicator size;
       non-power-of-two communicators pick Brucks for small messages. */
    if (pow2_size == communicator_size) {
        if (communicator_size <= 8) {
            if (total_dsize < 65536)
                return allgather__rdb(sbuf, scount, sdtype, rbuf, rcount, rdtype, comm);
        } else if (communicator_size <= 64) {
            if (total_dsize < 8192)
                return allgather__rdb(sbuf, scount, sdtype, rbuf, rcount, rdtype, comm);
        } else if (total_dsize < 1024) {
            return allgather__rdb(sbuf, scount, sdtype, rbuf, rcount, rdtype, comm);
        }
        return allgather__ring(sbuf, scount, sdtype, rbuf, rcount, rdtype, comm);
    }

    if (communicator_size <= 8) {
        if (total_dsize < 10240)
            return allgather__bruck(sbuf, scount, sdtype, rbuf, rcount, rdtype, comm);
    } else if (communicator_size <= 64) {
        if (total_dsize < 1280)
            return allgather__bruck(sbuf, scount, sdtype, rbuf, rcount, rdtype, comm);
    } else if (total_dsize < 160) {
        return allgather__bruck(sbuf, scount, sdtype, rbuf, rcount, rdtype, comm);
    }
    return allgather__ring(sbuf, scount, sdtype, rbuf, rcount, rdtype, comm);
}


/* This is the default implementation of allgatherv. The algorithm is:

   Algorithm: MPI_Allgatherv

   For short messages and non-power-of-two no. of processes, we use
   the algorithm from the Jehoshua Bruck et al IEEE TPDS Nov 97
   paper. It is a variant of the disemmination algorithm for
   barrier. It takes ceiling(lg p) steps.

   Cost = lgp.alpha + n.((p-1)/p).beta
   where n is total size of data gathered on each process.

   For short or medium-size messages and power-of-two no. of
   processes, we use the recursive doubling algorithm.

   Cost = lgp.alpha + n.((p-1)/p).beta

   TODO: On TCP, we may want to use recursive doubling instead of the Bruck
   algorithm in all cases because of the pairwise-exchange property of
   recursive doubling (see Benson et al paper in Euro PVM/MPI
   2003).

   For long messages or medium-size messages and non-power-of-two
   no. of processes, we use a ring algorithm. In the first step, each
   process i sends its contribution to process i+1 and receives
   the contribution from process i-1 (with wrap-around). From the
   second step onwards, each process i forwards to process i+1 the
   data it received from process i-1 in the previous step. This takes
   a total of p-1 steps.

   Cost = (p-1).alpha + n.((p-1)/p).beta

   Possible improvements:

   End Algorithm: MPI_Allgatherv
*/
int allgatherv__mpich(const void *sbuf, int scount,
                      MPI_Datatype sdtype,
                      void* rbuf, const int *rcounts,
                      const int *rdispls,
                      MPI_Datatype rdtype,
                      MPI_Comm  comm
                      )
{
    int communicator_size, pow2_size,i;
    size_t total_count = 0;
    size_t msg_size;

    communicator_size = comm->size();

    for (i=0; i<communicator_size; i++)
        total_count += rcounts[i];
    if (total_count == 0)
      return MPI_SUCCESS;

    /* MPICH thresholds msg_size as the mean receive count, in bytes */
    msg_size = ((total_count + communicator_size - 1) / communicator_size) * rdtype->size();

    for (pow2_size  = 1; pow2_size < communicator_size; pow2_size <<=1);

    /* Selection mirrors MPICH's allgatherv-intra-auto tuning table */
    if (pow2_size == communicator_size) {
        if (communicator_size <= 8) {
            if (msg_size < 65536)
                return allgatherv__mpich_rdb(sbuf, scount, sdtype, rbuf, rcounts, rdispls, rdtype, comm);
        } else if (communicator_size <= 64) {
            if (msg_size < 8192)
                return allgatherv__mpich_rdb(sbuf, scount, sdtype, rbuf, rcounts, rdispls, rdtype, comm);
        } else if (msg_size < 1024) {
            return allgatherv__mpich_rdb(sbuf, scount, sdtype, rbuf, rcounts, rdispls, rdtype, comm);
        }
        return allgatherv__mpich_ring(sbuf, scount, sdtype, rbuf, rcounts, rdispls, rdtype, comm);
    }

    if (communicator_size <= 8) {
        if (msg_size < 10240)
            return allgatherv__ompi_bruck(sbuf, scount, sdtype, rbuf, rcounts, rdispls, rdtype, comm);
    } else if (communicator_size <= 64) {
        if (msg_size < 1280)
            return allgatherv__ompi_bruck(sbuf, scount, sdtype, rbuf, rcounts, rdispls, rdtype, comm);
    } else if (msg_size < 160) {
        return allgatherv__ompi_bruck(sbuf, scount, sdtype, rbuf, rcounts, rdispls, rdtype, comm);
    }
    return allgatherv__mpich_ring(sbuf, scount, sdtype, rbuf, rcounts, rdispls, rdtype, comm);
}

/* This is the default implementation of gather. The algorithm is:

   Algorithm: MPI_Gather

   We use a binomial tree algorithm for both short and long
   messages. At nodes other than leaf nodes we need to allocate a
   temporary buffer to store the incoming message. If the root is not
   rank 0, for very small messages, we pack it into a temporary
   contiguous buffer and reorder it to be placed in the right
   order. For small (but not very small) messages, we use a derived
   datatype to unpack the incoming data into non-contiguous buffers in
   the right order. In the heterogeneous case we first pack the
   buffers by using MPI_Pack and then do the gather.

   Cost = lgp.alpha + n.((p-1)/p).beta
   where n is the total size of the data gathered at the root.

   Possible improvements:

   End Algorithm: MPI_Gather
*/

int gather__mpich(const void *sbuf, int scount,
                  MPI_Datatype sdtype,
                  void* rbuf, int rcount,
                  MPI_Datatype rdtype,
                  int root,
                  MPI_Comm  comm
                  )
{
    return gather__ompi_binomial(sbuf, scount, sdtype,
                                 rbuf, rcount, rdtype,
                                 root, comm);
}

/* This is the default implementation of scatter. The algorithm is:

   Algorithm: MPI_Scatter

   We use a binomial tree algorithm for both short and
   long messages. At nodes other than leaf nodes we need to allocate
   a temporary buffer to store the incoming message. If the root is
   not rank 0, we reorder the sendbuf in order of relative ranks by
   copying it into a temporary buffer, so that all the sends from the
   root are contiguous and in the right order. In the heterogeneous
   case, we first pack the buffer by using MPI_Pack and then do the
   scatter.

   Cost = lgp.alpha + n.((p-1)/p).beta
   where n is the total size of the data to be scattered from the root.

   Possible improvements:

   End Algorithm: MPI_Scatter
*/


int scatter__mpich(const void *sbuf, int scount,
                   MPI_Datatype sdtype,
                   void* rbuf, int rcount,
                   MPI_Datatype rdtype,
                   int root, MPI_Comm  comm
                   )
{
  std::unique_ptr<unsigned char[]> tmp_buf;
  if(comm->rank()!=root){
    tmp_buf = std::make_unique<unsigned char[]>(rcount * rdtype->get_extent());
    sbuf   = tmp_buf.get();
    scount = rcount;
    sdtype = rdtype;
  }
  return scatter__ompi_binomial(sbuf, scount, sdtype, rbuf, rcount, rdtype, root, comm);
}
} // namespace simgrid::smpi
