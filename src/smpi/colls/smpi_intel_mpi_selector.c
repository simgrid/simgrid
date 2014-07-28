/* selector for collective algorithms based on openmpi's default coll_tuned_decision_fixed selector */

/* Copyright (c) 2009-2010, 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "colls_private.h"


// This selector is based on information gathered on the Stampede cluster, with Intel MPI 4.1.3.049, and from the intel reference manual. The data was gathered launching one process/node. With other settings, selection will be different (more SMP aware algorithms, for instance)


#define INTEL_MAX_NB_THRESHOLDS  32

typedef struct {
  int max_size;
  int algo;
} intel_tuning_table_element_element;

typedef struct {
  int max_num_proc;
  int num_elems;
  intel_tuning_table_element_element elems[INTEL_MAX_NB_THRESHOLDS];
} intel_tuning_table_element;

/*
I_MPI_ADJUST_ALLREDUCE

MPI_Allreduce

1 - Recursive doubling algorithm
2 - Rabenseifner's algorithm
3 - Reduce + Bcast algorithm
4 - Topology aware Reduce + Bcast algorithm
5 - Binomial gather + scatter algorithm
6 - Topology aware binominal gather + scatter algorithm
7 - Shumilin's ring algorithm 
8 - Ring algorithm


//as Shumilin's ring algorithm is unknown, default to ring'
*/


int (*intel_allreduce_functions_table[])(void *sendbuf,
      void *recvbuf,
      int count,
      MPI_Datatype datatype,
      MPI_Op op, MPI_Comm comm) ={
      smpi_coll_tuned_allreduce_rdb,
      smpi_coll_tuned_allreduce_rab1,
      smpi_coll_tuned_allreduce_redbcast,
      smpi_coll_tuned_allreduce_redbcast,
      smpi_coll_tuned_allreduce_smp_binomial,
      smpi_coll_tuned_allreduce_smp_binomial,
      smpi_coll_tuned_allreduce_ompi_ring_segmented,
      smpi_coll_tuned_allreduce_ompi_ring_segmented
};

intel_tuning_table_element intel_allreduce_table[] =
{
  { 2,9,{
    {6,7},
    {85,1},
    {192,7},
    {853,1},
    {1279,7},
    {16684,1},
    {34279,8},
    {1681224,3},
    {2147483647,7}
  }
  },
  { 4, 8,{
    {16,7},
    {47,1},
    {2062,7},
    {16699,1},
    {33627,7},
    {70732,8},
    {1300705,3},
    {2147483647,8}
  }
  },
  {8,8,{
    {118,1},
    {146,4},
    {16760,1},
    {36364,6},
    {136239,8},
    {315710,7},
    {3220366,3},
    {2147483647,8}
    }
  },
  {16,7,{
    {934,1},
    {1160,6},
    {15505,1},
    {52730,2},
    {300705,8},
    {563680,7},
    {2147483647,3}
    }
  },
  {2147483647,11,{
    {5,6},
    {11,4},
    {182,1},
    {700,6},
    {1450,4},
    {11146,1},
    {25539,6},
    {37634,4},
    {93784,6},
    {817658,2},
    {2147483647,3}
  }
  }
};



/*I_MPI_ADJUST_ALLTOALL 

MPI_Alltoall 

1. Bruck's algorithm 
2. Isend/Irecv + waitall algorithm 
3. Pair wise exchange algorithm 
4. Plum's algorithm

*/


intel_tuning_table_element intel_alltoall_table[] =
{
    { 2,1,
        {
        {2147483647,3}
        }
    },
    { 4,2,
        {
        {0,4},
        {2147483647,2}
        }
    },
    {8,1,
        {
        {2147483647,2}
        }
    },
    {16,5,
        {
        {0,3},
        {84645,2},
        {167570,3},
        {413152,4},
        {2147483647,2}
        }
    },
    {32,6,
        {
        {61,1},
        {164,2},
        {696,1},
        {143254,2},
        {387024,3},
        {2147483647,2}
        },
    },
    {64,4,
        {
        {523,1},
        {146088,2},
        {488989,4},
        {2147483647,2}
        }
    },
    {2147483647,3,
        {
        {270,1},
        {628,4},
        {2147483647,2}
        }
    }
};
int (*intel_alltoall_functions_table[])(void *sbuf, int scount, 
                                             MPI_Datatype sdtype,
                                             void* rbuf, int rcount, 
                                             MPI_Datatype rdtype, 
                                             MPI_Comm comm) ={
      smpi_coll_tuned_alltoall_bruck,
      smpi_coll_tuned_alltoall_mvapich2_scatter_dest,
      smpi_coll_tuned_alltoall_pair,
      smpi_coll_tuned_alltoall_pair//Plum is proprietary ? (and super efficient)
};

/*I_MPI_ADJUST_BARRIER 

MPI_Barrier 

1. Dissemination algorithm 
2. Recursive doubling algorithm 
3. Topology aware dissemination algorithm 
4. Topology aware recursive doubling algorithm 
5. Binominal gather + scatter algorithm 
6. Topology aware binominal gather + scatter algorithm 

*/
static int intel_barrier_gather_scatter(MPI_Comm comm){
    //our default barrier performs a antibcast/bcast
    smpi_mpi_barrier(comm);
    return MPI_SUCCESS;
}

int (*intel_barrier_functions_table[])(MPI_Comm comm) ={
      smpi_coll_tuned_barrier_ompi_basic_linear,
      smpi_coll_tuned_barrier_ompi_recursivedoubling,
      smpi_coll_tuned_barrier_ompi_basic_linear,
      smpi_coll_tuned_barrier_ompi_recursivedoubling,
      intel_barrier_gather_scatter,
      intel_barrier_gather_scatter
};

intel_tuning_table_element intel_barrier_table[] =
{
    {2,1,
        {
        {2147483647,2}
        }
    },
    {4,1,
        {
        {2147483647,6}
        }
    },
    {8,1,
        {
        {2147483647,1}
        }
    },
    {64,1,
        {
        {2147483647,2}
        }
    },
    {2147483647,1,
        {
        {2147483647,6}
        }
    }
};


/*I_MPI_ADJUST_BCAST 

MPI_Bcast 

1. Binomial algorithm 
2. Recursive doubling algorithm 
3. Ring algorithm 
4. Topology aware binomial algorithm 
5. Topology aware recursive doubling algorithm 
6. Topology aware ring algorithm 
7. Shumilin's bcast algorithm 
*/

int (*intel_bcast_functions_table[])(void *buff, int count,
                                          MPI_Datatype datatype, int root,
                                          MPI_Comm  comm) ={
      smpi_coll_tuned_bcast_binomial_tree,
      //smpi_coll_tuned_bcast_scatter_rdb_allgather,
      smpi_coll_tuned_bcast_NTSL,
      smpi_coll_tuned_bcast_NTSL,
      smpi_coll_tuned_bcast_SMP_binomial,
      //smpi_coll_tuned_bcast_scatter_rdb_allgather,
            smpi_coll_tuned_bcast_NTSL,
      smpi_coll_tuned_bcast_SMP_linear,
      smpi_coll_tuned_bcast_mvapich2,//we don't know shumilin's algo'
};

intel_tuning_table_element intel_bcast_table[] =
{
    {2,9,
        {
        {1,2},
        {402,7},
        {682,5},
        {1433,4},
        {5734,7},
        {21845,1},
        {95963,6},
        {409897,5},
        {2147483647,1}
        }
    },
    {4,1,
        {
        {2147483647,7}
        }
    },
    {8,11,
        {
        {3,6},
        {4,7},
        {25,6},
        {256,1},
        {682,6},
        {1264,1},
        {2234,6},
        {6655,5},
        {16336,1},
        {3998434,7},
        {2147483647,6}
        }
    },
    {2147483647,1,
        {
        {2147483647,7}
        }
    }
};


/*I_MPI_ADJUST_REDUCE 

MPI_Reduce 

1. Shumilin's algorithm 
2. Binomial algorithm 
3. Topology aware Shumilin's algorithm 
4. Topology aware binomial algorithm 
5. Rabenseifner's algorithm 
6. Topology aware Rabenseifner's algorithm

*/

int (*intel_reduce_functions_table[])(void *sendbuf, void *recvbuf,
                                            int count, MPI_Datatype  datatype,
                                            MPI_Op   op, int root,
                                            MPI_Comm   comm) ={
      smpi_coll_tuned_reduce_mvapich2,
      smpi_coll_tuned_reduce_binomial,
      smpi_coll_tuned_reduce_mvapich2,
      smpi_coll_tuned_reduce_binomial,
      smpi_coll_tuned_reduce_rab,
      smpi_coll_tuned_reduce_rab
};

intel_tuning_table_element intel_reduce_table[] =
{
    {2147483647,1,
        {
        {2147483647,1}
        }
    }
};

/* I_MPI_ADJUST_REDUCE_SCATTER 

MPI_Reduce_scatter 

1. Recursive having algorithm 
2. Pair wise exchange algorithm 
3. Recursive doubling algorithm 
4. Reduce + Scatterv algorithm 
5. Topology aware Reduce + Scatterv algorithm 

*/
static  int intel_reduce_scatter_reduce_scatterv(void *sbuf, void *rbuf,
                                                    int *rcounts,
                                                    MPI_Datatype dtype,
                                                    MPI_Op  op,
                                                    MPI_Comm  comm)
{
  smpi_mpi_reduce_scatter(sbuf, rbuf, rcounts,dtype, op,comm);
  return MPI_SUCCESS;
}

static  int  intel_reduce_scatter_recursivehalving(void *sbuf, void *rbuf,
                                                    int *rcounts,
                                                    MPI_Datatype dtype,
                                                    MPI_Op  op,
                                                    MPI_Comm  comm)
{
  if(smpi_op_is_commute(op))
    return smpi_coll_tuned_reduce_scatter_ompi_basic_recursivehalving(sbuf, rbuf, rcounts,dtype, op,comm);
  else
    return smpi_coll_tuned_reduce_scatter_mvapich2(sbuf, rbuf, rcounts,dtype, op,comm);
}

int (*intel_reduce_scatter_functions_table[])( void *sbuf, void *rbuf,
                                                    int *rcounts,
                                                    MPI_Datatype dtype,
                                                    MPI_Op  op,
                                                    MPI_Comm  comm
                                                    ) ={
      intel_reduce_scatter_recursivehalving,
      smpi_coll_tuned_reduce_scatter_mpich_pair,
      smpi_coll_tuned_reduce_scatter_mpich_rdb,
      intel_reduce_scatter_reduce_scatterv,
      intel_reduce_scatter_reduce_scatterv
};

intel_tuning_table_element intel_reduce_scatter_table[] =
{
    {2,5,
    {
        {5,4},
        {522429,2},
        {1375877,5},
        {2932736,2},
        {2147483647,5}
        }
    },
    {4,9,
        {
        {4,4},
        {15,1},
        {120,3},
        {651,1},
        {12188,3},
        {33890,1},
        {572117,2},
        {1410202,5},
        {2147483647,2}
        }
    },
    {8,7,
        {
        {4,4},
        {2263,1},
        {25007,3},
        {34861,1},
        {169625,2},
        {2734000,4},
        {2147483647,2}
        }
    },
    {16,5,
        {
        {4,4},
        {14228,1},
        {46084,3},
        {522139,2},
        {2147483647,5}
        }
    },
    {32,5,
        {
        {4,4},
        {27516,1},
        {61693,3},
        {2483469,2},
        {2147483647,5}
        }
    },
    {64,4,
        {
        {0,3},
        {4,4},
        {100396,1},
        {2147483647,2}
        }
    },
    {2147483647,6,
        {
        {0,3},
        {4,4},
        {186926,1},
        {278259,3},
        {1500100,2},
        {2147483647,5}
        }
    }
};

/* I_MPI_ADJUST_ALLGATHER 

MPI_Allgather 

1. Recursive doubling algorithm 
2. Bruck's algorithm 
3. Ring algorithm 
4. Topology aware Gatherv + Bcast algorithm 

*/

int (*intel_allgather_functions_table[])(void *sbuf, int scount, 
                                              MPI_Datatype sdtype,
                                              void* rbuf, int rcount, 
                                              MPI_Datatype rdtype, 
                                              MPI_Comm  comm
                                                    ) ={
      smpi_coll_tuned_allgather_rdb,
      smpi_coll_tuned_allgather_bruck,
      smpi_coll_tuned_allgather_ring,
      smpi_coll_tuned_allgather_GB
};

intel_tuning_table_element intel_allgather_table[] =
{
    {4,11,
        {
        {1,4},
        {384,1},
        {1533,4},
        {3296,1},
        {10763,4},
        {31816,3},
        {193343,4},
        {405857,3},
        {597626,4},
        {1844323,3},
        {2147483647,4}
        }
    },
    {8,10,
        {
        {12,4},
        {46,1},
        {205,4},
        {3422,2},
        {4200,4},
        {8748,1},
        {24080,3},
        {33244,4},
        {371159,1},
        {2147483647,3}
        }
    },
    {16, 8,
        {
        {3,4},
        {53,1},
        {100,4},
        {170,1},
        {6077,4},
        {127644,1},
        {143741,4},
        {2147483647,3}
        }
    },
    {2147483647,10,
        {
        {184,1},
        {320,4},
        {759,1},
        {1219,4},
        {2633,1},
        {8259,4},
        {123678,1},
        {160801,4},
        {284341,1},
        {2147483647,4}
        }
    }
};

/* I_MPI_ADJUST_ALLGATHERV 

MPI_Allgatherv 

1. Recursive doubling algorithm 
2. Bruck's algorithm 
3. Ring algorithm 
4. Topology aware Gatherv + Bcast algorithm 

*/

int (*intel_allgatherv_functions_table[])(void *sbuf, int scount, 
                                               MPI_Datatype sdtype,
                                               void* rbuf, int *rcounts, 
                                               int *rdispls,
                                               MPI_Datatype rdtype, 
                                               MPI_Comm  comm
                                                    ) ={
      smpi_coll_tuned_allgatherv_mpich_rdb,
      smpi_coll_tuned_allgatherv_ompi_bruck,
      smpi_coll_tuned_allgatherv_ring,
      smpi_coll_tuned_allgatherv_GB
};

intel_tuning_table_element intel_allgatherv_table[] =
{
    {2,3,
        {
        {259668,3},
        {635750,4},
        {2147483647,3}
        }
    },
    {4,7,
        {
        {1,1},
        {5,4},
        {46,1},
        {2590,2},
        {1177259,3},
        {2767234,4},
        {2147483647,3}
        }
    },
    {8, 6,
        {
        {99,2},
        {143,1},
        {4646,2},
        {63522,3},
        {2187806,4},
        {2147483647,3}
        }
    },
    {2147483647,7,
        {
        {1,1},
        {5,4},
        {46,1},
        {2590,2},
        {1177259,3},
        {2767234,4},
        {2147483647,3}
        }
    }
};


/* I_MPI_ADJUST_GATHER

MPI_Gather

1. Binomial algorithm 
2. Topology aware binomial algorithm 
3. Shumilin's algorithm

*/

int (*intel_gather_functions_table[])(void *sbuf, int scount, 
                                           MPI_Datatype sdtype,
                                           void* rbuf, int rcount, 
                                           MPI_Datatype rdtype, 
                                           int root,
                                           MPI_Comm  comm
                                                    ) ={
      smpi_coll_tuned_gather_ompi_binomial,
      smpi_coll_tuned_gather_ompi_binomial,
      smpi_coll_tuned_gather_mvapich2
};

intel_tuning_table_element intel_gather_table[] =
{
    {8,3,
        {
        {17561,3},
        {44791,2},
        {2147483647,3}
        }
    },
    {16,7,
        {
        {16932,3},
        {84425,2},
        {158363,3},
        {702801,2},
        {1341444,3},
        {2413569,2},
        {2147483647,3}
        }
    },
    {2147483647,4,
        {
        {47187,3},
        {349696,2},
        {2147483647,3},
        {2147483647,1}
        }
    }
};


/* I_MPI_ADJUST_SCATTER 

MPI_Scatter 

1. Binomial algorithm 
2. Topology aware binomial algorithm 
3. Shumilin's algorithm 

*/

int (*intel_scatter_functions_table[])(void *sbuf, int scount, 
                                            MPI_Datatype sdtype,
                                            void* rbuf, int rcount, 
                                            MPI_Datatype rdtype, 
                                            int root, MPI_Comm  comm
                                                    ) ={
      smpi_coll_tuned_scatter_ompi_binomial,
      smpi_coll_tuned_scatter_ompi_binomial,
      smpi_coll_tuned_scatter_mvapich2
};

intel_tuning_table_element intel_scatter_table[] =
{
    {2,2,
        {
        {16391,1},
        {2147483647,3}
        }
    },
    {4,6,
        {
        {16723,3},
        {153541,2},
        {425631,3},
        {794142,2},
        {1257027,3},
        {2147483647,2}
        }
    },
    {8,7,
        {
        {2633,3},
        {6144,2},
        {14043,3},
        {24576,2},
        {107995,3},
        {1752729,2},
        {2147483647,3}
        }
    },
    {16,7,
        {
        {2043,3},
        {2252,2},
        {17749,3},
        {106020,2},
        {628654,3},
        {3751354,2},
        {2147483647,3}
        }
    },
    {2147483647,4,
        {
        {65907,3},
        {245132,2},
        {1042439,3},
        {2147483647,2},
        {2147483647,1}
        }
    }
};



/* I_MPI_ADJUST_ALLTOALLV 

MPI_Alltoallv 

1. Isend/Irecv + waitall algorithm 
2. Plum's algorithm 

*/

int (*intel_alltoallv_functions_table[])(void *sbuf, int *scounts, int *sdisps,
                                              MPI_Datatype sdtype,
                                              void *rbuf, int *rcounts, int *rdisps,
                                              MPI_Datatype rdtype,
                                              MPI_Comm  comm
                                                    ) ={
      smpi_coll_tuned_alltoallv_ompi_basic_linear,
      smpi_coll_tuned_alltoallv_bruck
};

intel_tuning_table_element intel_alltoallv_table[] =
{
    {2147483647,1,
        {
        {2147483647,1}
        }
    }
};


//These are collected from table 3.5-2 of the Intel MPI Reference Manual 

    
#define SIZECOMP_reduce_scatter\
    int total_message_size = 0;\
    for (i = 0; i < comm_size; i++) { \
        total_message_size += rcounts[i];\
    }\
    size_t block_dsize = total_message_size*smpi_datatype_size(dtype);\
    
#define SIZECOMP_allreduce\
  size_t block_dsize =rcount * smpi_datatype_size(dtype);
  
#define SIZECOMP_alltoall\
  size_t block_dsize =send_count * smpi_datatype_size(send_type);

#define SIZECOMP_bcast\
  size_t block_dsize =count * smpi_datatype_size(datatype);

#define SIZECOMP_reduce\
  size_t block_dsize =count * smpi_datatype_size(datatype);

#define SIZECOMP_barrier\
  size_t block_dsize = 1;

#define SIZECOMP_allgather\
  size_t block_dsize =recv_count * smpi_datatype_size(recv_type);

#define SIZECOMP_allgatherv\
    int total_message_size = 0;\
    for (i = 0; i < comm_size; i++) { \
        total_message_size += recv_count[i];\
    }\
    size_t block_dsize = total_message_size*smpi_datatype_size(recv_type);
    
#define SIZECOMP_gather\
  int rank = smpi_comm_rank(comm);\
  size_t block_dsize = (send_buff == MPI_IN_PLACE || rank ==root) ?\
                recv_count * smpi_datatype_size(recv_type) :\
                send_count * smpi_datatype_size(send_type);

#define SIZECOMP_scatter\
  int rank = smpi_comm_rank(comm);\
  size_t block_dsize = (sendbuf == MPI_IN_PLACE || rank !=root ) ?\
                recvcount * smpi_datatype_size(recvtype) :\
                sendcount * smpi_datatype_size(sendtype);

#define SIZECOMP_alltoallv\
  size_t block_dsize = 1;
  
#define IMPI_COLL_SELECT(cat, ret, args, args2)\
ret smpi_coll_tuned_ ## cat ## _impi (COLL_UNPAREN args)\
{\
    int comm_size = smpi_comm_size(comm);\
    int i =0;\
    SIZECOMP_ ## cat\
    i=0;\
    int j =0;\
    while(comm_size>=intel_ ## cat ## _table[i].max_num_proc\
        && i < INTEL_MAX_NB_THRESHOLDS)\
      i++;\
    while(block_dsize >=intel_ ## cat ## _table[i].elems[j].max_size\
         && j< intel_ ## cat ## _table[i].num_elems)\
      j++;\
    return (intel_ ## cat ## _functions_table[intel_ ## cat ## _table[i].elems[j].algo-1]\
    args2);\
}

COLL_APPLY(IMPI_COLL_SELECT, COLL_ALLGATHERV_SIG, (send_buff, send_count, send_type, recv_buff, recv_count, recv_disps, recv_type, comm));
COLL_APPLY(IMPI_COLL_SELECT, COLL_ALLREDUCE_SIG, (sbuf, rbuf, rcount, dtype, op, comm));
COLL_APPLY(IMPI_COLL_SELECT, COLL_GATHER_SIG, (send_buff, send_count, send_type, recv_buff, recv_count, recv_type, root, comm));
COLL_APPLY(IMPI_COLL_SELECT, COLL_ALLGATHER_SIG, (send_buff,send_count,send_type,recv_buff,recv_count,recv_type,comm));
COLL_APPLY(IMPI_COLL_SELECT, COLL_ALLTOALL_SIG,(send_buff, send_count, send_type, recv_buff, recv_count, recv_type,comm));
COLL_APPLY(IMPI_COLL_SELECT, COLL_ALLTOALLV_SIG, (send_buff, send_counts, send_disps, send_type, recv_buff, recv_counts, recv_disps, recv_type, comm));
COLL_APPLY(IMPI_COLL_SELECT, COLL_BCAST_SIG , (buf, count, datatype, root, comm));
COLL_APPLY(IMPI_COLL_SELECT, COLL_REDUCE_SIG,(buf,rbuf, count, datatype, op, root, comm));
COLL_APPLY(IMPI_COLL_SELECT, COLL_REDUCE_SCATTER_SIG ,(sbuf,rbuf, rcounts,dtype,op,comm));
COLL_APPLY(IMPI_COLL_SELECT, COLL_SCATTER_SIG ,(sendbuf, sendcount, sendtype,recvbuf, recvcount, recvtype,root, comm));
COLL_APPLY(IMPI_COLL_SELECT, COLL_BARRIER_SIG,(comm));

