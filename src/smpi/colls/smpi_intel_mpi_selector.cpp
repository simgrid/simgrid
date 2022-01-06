/* selector for collective algorithms based on openmpi's default coll_tuned_decision_fixed selector */

/* Copyright (c) 2009-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "colls_private.hpp"

// This selector is based on information gathered on the Stampede cluster, with Intel MPI 4.1.3.049, and from the intel reference manual. The data was gathered launching runs with 1,2,4,8,16 processes per node.

#define INTEL_MAX_NB_THRESHOLDS  32
#define INTEL_MAX_NB_NUMPROCS  12
#define INTEL_MAX_NB_PPN  5  /* 1 2 4 8 16 ppn */

struct intel_tuning_table_size_element {
  unsigned int max_size;
  int algo;
};

struct intel_tuning_table_numproc_element {
  int max_num_proc;
  int num_elems;
  intel_tuning_table_size_element elems[INTEL_MAX_NB_THRESHOLDS];
};

struct intel_tuning_table_element {
  int ppn;
  intel_tuning_table_numproc_element elems[INTEL_MAX_NB_NUMPROCS];
};

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

  as Shumilin's ring algorithm is unknown, default to ring'
*/

namespace simgrid{
namespace smpi{

int (*intel_allreduce_functions_table[])(const void *sendbuf,
      void *recvbuf,
      int count,
      MPI_Datatype datatype,
      MPI_Op op, MPI_Comm comm) ={
      allreduce__rdb,
      allreduce__rab1,
      allreduce__redbcast,
      allreduce__mvapich2_two_level,
      allreduce__smp_binomial,
      allreduce__mvapich2_two_level,
      allreduce__ompi_ring_segmented,
      allreduce__ompi_ring_segmented
};

intel_tuning_table_element intel_allreduce_table[] =
{
  {1,{
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
  }
  },
  {2,{
    { 4,6,{
      {2084,7},
      {15216,1},
      {99715,7},
      {168666,3},
      {363889,2},
      {2147483647,7}
    }
    },
    { 8,6,{
      {14978,1},
      {66879,2},
      {179296,8},
      {304801,3},
      {704509,7},
      {2147483647,2}
    }
    },
    { 16,6,{
      {16405,1},
      {81784,2},
      {346385,8},
      {807546,7},
      {1259854,2},
      {2147483647,3}
    }
    },
    { 32,4,{
      {8913,1},
      {103578,2},
      {615876,8},
      {2147483647,2}
    }
    },
    { 64,7,{
      {1000,1},
      {2249,2},
      {6029,1},
      {325357,2},
      {1470976,8},
      {2556670,7},
      {2147483647,3}
    }
    },
    { 128,5,{
      {664,1},
      {754706,2},
      {1663862,4},
      {3269097,2},
      {2147483647,7}
    }
    },
    { 2147483647,3,{
      {789,1},
      {2247589,2},
      {2147483647,8}
    }
    }
  }
  },
  {4,{
    { 4,4,{
      {5738,1},
      {197433,2},
      {593742,7},
      {2147483647,2}
    }
    },
    { 8,7,{
      {5655,1},
      {75166,2},
      {177639,8},
      {988014,3},
      {1643869,2},
      {2494859,8},
      {2147483647,2}
    }
    },
    { 16,7,{
      {587,1},
      {3941,2},
      {9003,1},
      {101469,2},
      {355768,8},
      {3341814,3},
      {2147483647,8}
    }
    },
    { 32,4,{
      {795,1},
      {146567,2},
      {732118,8},
      {2147483647,3}
    }
    },
    { 64,4,{
      {528,1},
      {221277,2},
      {1440737,8},
      {2147483647,3}
    }
    },
    { 128,4,{
      {481,1},
      {593833,2},
      {2962021,8},
      {2147483647,7}
    }
    },
    { 256,2,{
      {584,1},
      {2147483647,2}
    }
    },
    { 2147483647,3,{
      {604,1},
      {2997006,2},
      {2147483647,8}
    }
    }
  }
  },
  {8,{
    { 8,6,{
      {2560,1},
      {114230,6},
      {288510,8},
      {664038,2},
      {1339913,6},
      {2147483647,4}
    }
    },
    { 16,5,{
      {497,1},
      {54201,2},
      {356217,8},
      {3413609,3},
      {2147483647,8}
    }
    },
    { 32,5,{
      {377,1},
      {109745,2},
      {716514,8},
      {3976768,3},
      {2147483647,8}
    }
    },
    { 64,6,{
      {109,1},
      {649,5},
      {266080,2},
      {1493331,8},
      {2541403,7},
      {2147483647,3}
    }
    },
    { 128,4,{
      {7,1},
      {751,5},
      {408808,2},
      {2147483647,8}
    }
    },
    { 256,3,{
      {828,5},
      {909676,2},
      {2147483647,8}
    }
    },
    { 512,5,{
      {847,5},
      {1007066,2},
      {1068775,4},
      {2803389,2},
      {2147483647,8}
    }
    },
    { 2147483647,3,{
      {1974,5},
      {4007876,2},
      {2147483647,8}
    }
    }
  }
  },
  {16,{
    { 16,12,{
      {409,1},
      {768,6},
      {1365,4},
      {3071,6},
      {11299,2},
      {21746,6},
      {55629,2},
      {86065,4},
      {153867,2},
      {590560,6},
      {1448760,2},
      {2147483647,8},
    }
    },
    { 32,8,{
      {6,1},
      {24,5},
      {86,1},
      {875,5},
      {74528,2},
      {813050,8},
      {1725981,7},
      {2147483647,8},
    }
    },
    { 64,6,{
      {1018,5},
      {1217,6},
      {2370,5},
      {160654,2},
      {1885487,8},
      {2147483647,3},
    }
    },
    { 128,4,{
      {2291,5},
      {434465,2},
      {3525103,8},
      {2147483647,7},
    }
    },
    { 256,3,{
      {2189,5},
      {713154,2},
      {2147483647,8},
    }
    },
    { 512,3,{
      {2140,5},
      {1235056,2},
      {2147483647,8},
    }
    },
    { 2147483647,3,{
      {2153,5},
      {2629855,2},
      {2147483647,8},
    }
    }
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
  {1,{
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
  }
  },
  {2, {
    { 4,4,{
      {1,2},
      {75,3},
      {131072,2},
      {2147483647,2}
    }
    },
    { 8,3,{
      {709,1},
      {131072,2},
      {2147483647,2}
    }
    },
    { 16,4,{
      {40048,2},
      {131072,3},
      {155927,3},
      {2147483647,4}
    }
    },
    { 32,7,{
      {105,1},
      {130,2},
      {1030,1},
      {58893,2},
      {131072,2},
      {271838,3},
      {2147483647,2}
    }
    },
    { 2147483647,8,{
      {521,1},
      {2032,4},
      {2412,2},
      {4112,4},
      {61620,2},
      {131072,3},
      {427408,3},
      {2147483647,4}
    }
    }
  }
  },
  {4,{
    { 8,3,{
      {512,1},
      {32768,2},
      {2147483647,2}
    }
    },
    { 64,8,{
      {7,1},
      {199,4},
      {764,1},
      {6409,4},
      {20026,2},
      {32768,3},
      {221643,4},
      {2147483647,3}
    }
    },
    { 2147483647,7,{
      {262,1},
      {7592,4},
      {22871,2},
      {32768,3},
      {47538,3},
      {101559,4},
      {2147483647,3}
    }
    }
  }
  },
  {8,{
    { 16,6,{
      {973,1},
      {5126,4},
      {16898,2},
      {32768,4},
      {65456,4},
      {2147483647,2}
    }
    },
    { 32,7,{
      {874,1},
      {6727,4},
      {17912,2},
      {32768,3},
      {41513,3},
      {199604,4},
      {2147483647,3}
    }
    },
    { 64,8,{
      {5,1},
      {114,4},
      {552,1},
      {8130,4},
      {32768,3},
      {34486,3},
      {160113,4},
      {2147483647,3}
    }
    },
    { 128,6,{
      {270,1},
      {3679,4},
      {32768,3},
      {64367,3},
      {146595,4},
      {2147483647,3}
    }
    },
    { 2147483647,4,{
      {133,1},
      {4017,4},
      {32768,3},
      {76351,4},
      {2147483647,3}
    }
    }
  }
  },
  {16,{
    { 32,7,{
      {963,1},
      {1818,4},
      {20007,2},
      {32768,4},
      {54296,4},
      {169735,3},
      {2147483647,2}
    }
    },
    { 64,11,{
      {17,1},
      {42,4},
      {592,1},
      {2015,4},
      {2753,2},
      {6496,3},
      {20402,4},
      {32768,3},
      {36246,3},
      {93229,4},
      {2147483647,3}
    }
    },
    { 128,9,{
      {18,1},
      {40,4},
      {287,1},
      {1308,4},
      {6842,1},
      {32768,3},
      {36986,3},
      {129081,4},
      {2147483647,3}
    }
    },
    { 256,7,{
      {135,1},
      {1538,4},
      {3267,1},
      {4132,3},
      {31469,4},
      {32768,3},
      {2147483647,3}
    }
    },
    { 2147483647,8,{
      {66,1},
      {1637,4},
      {2626,1},
      {4842,4},
      {32768,3},
      {33963,3},
      {72978,4},
      {2147483647,3}
    }
    }
  }
  }
};
int (*intel_alltoall_functions_table[])(const void *sbuf, int scount,
                                             MPI_Datatype sdtype,
                                             void* rbuf, int rcount,
                                             MPI_Datatype rdtype,
                                             MPI_Comm comm) ={
      alltoall__bruck,
      alltoall__mvapich2_scatter_dest,
      alltoall__pair,
      alltoall__mvapich2//Plum is proprietary ? (and super efficient)
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
  // our default barrier performs an antibcast/bcast
  barrier__default(comm);
  return MPI_SUCCESS;
}

int (*intel_barrier_functions_table[])(MPI_Comm comm) ={
      barrier__ompi_basic_linear,
      barrier__ompi_recursivedoubling,
      barrier__ompi_basic_linear,
      barrier__ompi_recursivedoubling,
      intel_barrier_gather_scatter,
      intel_barrier_gather_scatter
};

intel_tuning_table_element intel_barrier_table[] =
{
  {1,{
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
  }
  },
  {2,{
    { 2,1,{
      {2147483647,1}
    }
    },
    { 4,1,{
      {2147483647,3}
    }
    },
    { 8,1,{
      {2147483647,5}
    }
    },
    { 32,1,{
      {2147483647,2}
    }
    },
    { 128,1,{
      {2147483647,3}
    }
    },
    { 2147483647,1,{
      {2147483647,4}
    }
    }
  }
  },
  {4,{
    { 4,1,{
      {2147483647,2}
    }
    },
    { 8,1,{
      {2147483647,5}
    }
    },
    { 32,1,{
      {2147483647,2}
    }
    },
    { 2147483647,1,{
      {2147483647,4}
    }
    }
  }
  },
  {8,{
    { 8,1,{
      {2147483647,1}
    }
    },
    { 32,1,{
      {2147483647,2}
    }
    },
    { 2147483647,1,{
      {2147483647,4}
    }
    }
  }
  },
  {16,{
    { 4,1,{
      {2147483647,2}
    }
    },
    { 8,1,{
      {2147483647,5}
    }
    },
    { 32,1,{
      {2147483647,2}
    }
    },
    { 2147483647,1,{
      {2147483647,4}
    }
    }
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
      bcast__binomial_tree,
      //bcast__scatter_rdb_allgather,
      bcast__NTSL,
      bcast__NTSL,
      bcast__SMP_binomial,
      //bcast__scatter_rdb_allgather,
      bcast__NTSL,
      bcast__SMP_linear,
      bcast__mvapich2,//we don't know shumilin's algo'
};

intel_tuning_table_element intel_bcast_table[] =
{
  {1,{
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
  }
  },
  {2,{
    { 4,6,{
      {806,4},
      {18093,7},
      {51366,6},
      {182526,4},
      {618390,1},
      {2147483647,7}
    }
    },
    { 8,6,{
      {24,1},
      {74,4},
      {18137,1},
      {614661,7},
      {1284626,1},
      {2147483647,2}
    }
    },
    { 16,4,{
      {1,1},
      {158,7},
      {16955,1},
      {2147483647,7}
    }
    },
    { 32,3,{
      {242,7},
      {10345,1},
      {2147483647,7}
    }
    },
    { 2147483647,4,{
      {1,1},
      {737,7},
      {5340,1},
      {2147483647,7}
    }
    }
  }
  },
  {4,{
    { 8,4,{
      {256,4},
      {17181,1},
      {1048576,7},
      {2147483647,7}
    }
    },
    { 2147483647,1,{
      {2147483647,7}
    }
    }
  }
  },
  {8,{
    { 16,5,{
      {3,1},
      {318,7},
      {1505,1},
      {1048576,7},
      {2147483647,7}
    }
    },
    { 32,3,{
      {422,7},
      {851,1},
      {2147483647,7}
    }
    },
    { 64,3,{
      {468,7},
      {699,1},
      {2147483647,7}
    }
    },
    { 2147483647,1,{
      {2147483647,7}
    }
    }
  }
  },
  {16,{
    { 8,4,{
      {256,4},
      {17181,1},
      {1048576,7},
      {2147483647,7}
    }
    },
    { 2147483647,1,{
      {2147483647,7}
    }
    }
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

int (*intel_reduce_functions_table[])(const void *sendbuf, void *recvbuf,
                                            int count, MPI_Datatype  datatype,
                                            MPI_Op   op, int root,
                                            MPI_Comm   comm) ={
      reduce__mvapich2,
      reduce__binomial,
      reduce__mvapich2,
      reduce__mvapich2_two_level,
      reduce__rab,
      reduce__rab
};

intel_tuning_table_element intel_reduce_table[] =
{
  {1,{
    {2147483647,1,
      {
      {2147483647,1}
      }
    }
  }
  },
  {2,{
    { 2,1,{
      {2147483647,1}
    }
    },
    { 4,2,{
      {10541,3},
      {2147483647,1}
    }
    },
    { 2147483647,1,{
      {2147483647,1}
    }
    }
  }
  },
  {4,{
    { 256,1,{
      {2147483647,1}
    }
    },
    { 2147483647,2,{
      {45,3},
      {2147483647,1}
    }
    }
  }
  },
  {8,{
    { 512,1,{
      {2147483647,1}
    }
    },
    { 2147483647,3,{
      {5,1},
      {11882,3},
      {2147483647,1}
    }
    }
  }
  },
  {16,{
    { 256,1,{
      {2147483647,1}
    }
    },
    { 2147483647,2,{
      {45,3},
      {2147483647,1}
    }
    }
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
static  int intel_reduce_scatter_reduce_scatterv(const void *sbuf, void *rbuf,
                                                    const int *rcounts,
                                                    MPI_Datatype dtype,
                                                    MPI_Op  op,
                                                    MPI_Comm  comm)
{
  reduce_scatter__default(sbuf, rbuf, rcounts,dtype, op,comm);
  return MPI_SUCCESS;
}

static  int  intel_reduce_scatter_recursivehalving(const void *sbuf, void *rbuf,
                                                    const int *rcounts,
                                                    MPI_Datatype dtype,
                                                    MPI_Op  op,
                                                    MPI_Comm  comm)
{
  if(op==MPI_OP_NULL || op->is_commutative())
    return reduce_scatter__ompi_basic_recursivehalving(sbuf, rbuf, rcounts,dtype, op,comm);
  else
    return reduce_scatter__mvapich2(sbuf, rbuf, rcounts,dtype, op,comm);
}

int (*intel_reduce_scatter_functions_table[])( const void *sbuf, void *rbuf,
                                                    const int *rcounts,
                                                    MPI_Datatype dtype,
                                                    MPI_Op  op,
                                                    MPI_Comm  comm
                                                    ) ={
      intel_reduce_scatter_recursivehalving,
      reduce_scatter__mpich_pair,
      reduce_scatter__mpich_rdb,
      intel_reduce_scatter_reduce_scatterv,
      intel_reduce_scatter_reduce_scatterv
};

intel_tuning_table_element intel_reduce_scatter_table[] =
{
  {1,{
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
  }
  },
  {2,{
    { 2,2,{
      {6,1},
      {2147483647,2}
    }
    },
    { 4,7,{
      {5,4},
      {13,5},
      {59,3},
      {76,1},
      {91488,3},
      {680063,4},
      {2147483647,2}
    }
    },
    { 8,8,{
      {4,4},
      {11,5},
      {31,1},
      {69615,3},
      {202632,2},
      {396082,5},
      {1495696,4},
      {2147483647,2}
    }
    },
    { 16,1,{
      {4,4},
      {345,1},
      {79523,3},
      {2147483647,2}
    }
    },
    { 32,5,{
      {0,3},
      {4,4},
      {992,1},
      {71417,3},
      {2147483647,2}
    }
    },
    { 64,4,{
      {4,4},
      {1472,1},
      {196592,3},
      {2147483647,2}
    }
    },
    { 128,5,{
      {0,3},
      {4,4},
      {32892,1},
      {381072,3},
      {2147483647,2}
    }
    },
    { 2147483647,6,{
      {0,2},
      {4,4},
      {33262,1},
      {1571397,3},
      {2211398,5},
      {2147483647,4}
    }
    }
  }
  },
  {4,{
    { 4,7,{
      {12,4},
      {27,5},
      {49,3},
      {187,1},
      {405673,3},
      {594687,4},
      {2147483647,2}
    }
    },
    { 8,5,{
      {24,5},
      {155,1},
      {204501,3},
      {274267,5},
      {2147483647,4}
    }
    },
    { 16,6,{
      {63,1},
      {72,3},
      {264,1},
      {168421,3},
      {168421,4},
      {2147483647,2}
    }
    },
    { 32,10,{
      {0,3},
      {4,4},
      {12,1},
      {18,5},
      {419,1},
      {188739,3},
      {716329,4},
      {1365841,5},
      {2430194,2},
      {2147483647,4}
    }
    },
    { 64,8,{
      {0,3},
      {4,4},
      {17,5},
      {635,1},
      {202937,3},
      {308253,5},
      {1389874,4},
      {2147483647,2}
    }
    },
    { 128,8,{
      {0,3},
      {4,4},
      {16,5},
      {1238,1},
      {280097,3},
      {631434,5},
      {2605072,4},
      {2147483647,2}
    }
    },
    { 256,5,{
      {0,2},
      {4,4},
      {16,5},
      {2418,1},
      {2147483647,3}
    }
    },
    { 2147483647,6,{
      {0,2},
      {4,4},
      {16,5},
      {33182,1},
      {3763779,3},
      {2147483647,4}
    }
    }
  }
  },
  {8,{
    { 8,6,{
      {5,4},
      {494,1},
      {97739,3},
      {522836,2},
      {554174,5},
      {2147483647,2}
    }
    },
    { 16,8,{
      {5,4},
      {62,1},
      {94,3},
      {215,1},
      {185095,3},
      {454784,4},
      {607911,5},
      {2147483647,4}
    }
    },
    { 32,7,{
      {0,3},
      {4,4},
      {302,1},
      {250841,3},
      {665822,4},
      {1760980,5},
      {2147483647,4}
    }
    },
    { 64,8,{
      {0,3},
      {4,4},
      {41,5},
      {306,1},
      {332405,3},
      {1269189,4},
      {3712421,5},
      {2147483647,4}
    }
    },
    { 128,6,{
      {0,3},
      {4,4},
      {39,5},
      {526,1},
      {487878,3},
      {2147483647,4}
    }
    },
    { 256,8,{
      {0,2},
      {4,4},
      {36,5},
      {1382,1},
      {424162,3},
      {632881,5},
      {1127566,3},
      {2147483647,4}
    }
    },
    { 512,4,{
      {4,4},
      {34,5},
      {5884,1},
      {2147483647,3}
    }
    },
    { 2147483647,4,{
      {5,4},
      {32,5},
      {25105,1},
      {2147483647,3}
    }
    }
  }
  },
  {16,{
    { 4,7,{
      {12,4},
      {27,5},
      {49,3},
      {187,1},
      {405673,3},
      {594687,4},
      {2147483647,2}
    }
    },
    { 8,5,{
      {24,5},
      {155,1},
      {204501,3},
      {274267,5},
      {2147483647,4}
    }
    },
    { 16,6,{
      {63,1},
      {72,3},
      {264,1},
      {168421,3},
      {168421,4},
      {2147483647,2}
    }
    },
    { 32,10,{
      {0,3},
      {4,4},
      {12,1},
      {18,5},
      {419,1},
      {188739,3},
      {716329,4},
      {1365841,5},
      {2430194,2},
      {2147483647,4}
    }
    },
    { 64,8,{
      {0,3},
      {4,4},
      {17,5},
      {635,1},
      {202937,3},
      {308253,5},
      {1389874,4},
      {2147483647,2}
    }
    },
    { 128,8,{
      {0,3},
      {4,4},
      {16,5},
      {1238,1},
      {280097,3},
      {631434,5},
      {2605072,4},
      {2147483647,2}
    }
    },
    { 256,5,{
      {0,2},
      {4,4},
      {16,5},
      {2418,1},
      {2147483647,3}
    }
    },
    { 2147483647,6,{
      {0,2},
      {4,4},
      {16,5},
      {33182,1},
      {3763779,3},
      {2147483647,4}
    }
    }
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

int (*intel_allgather_functions_table[])(const void *sbuf, int scount,
                                              MPI_Datatype sdtype,
                                              void* rbuf, int rcount,
                                              MPI_Datatype rdtype,
                                              MPI_Comm  comm
                                                    ) ={
      allgather__rdb,
      allgather__bruck,
      allgather__ring,
      allgather__GB
};

intel_tuning_table_element intel_allgather_table[] =
{
  {1,{
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
  }
  },
  {2,{
    { 8,6,{
      {490,1},
      {558,2},
      {2319,1},
      {46227,3},
      {2215101,1},
      {2147483647,3}
    }
    },
    { 16,4,{
      {1005,1},
      {1042,2},
      {2059,1},
      {2147483647,3}
    }
    },
    { 2147483647,2,{
      {2454,1},
      {2147483647,3}
   }
   }
  }
  },
  {4,{
    { 8,2,{
      {2861,1},
      {2147483647,3}
    }
    },
    { 2147483647,2,{
      {605,1},
      {2147483647,3}
    }
    }
  }
  },
  {8,{
    { 16,4,{
      {66,1},
      {213,4},
      {514,1},
      {2147483647,3}
    }
    },
    { 32,4,{
      {91,1},
      {213,4},
      {514,1},
      {2147483647,3}
    }
    },
    { 64,4,{
      {71,1},
      {213,4},
      {514,1},
      {2147483647,3}
    }
    },
    { 128,2,{
      {305,1},
      {2147483647,3}
    }
    },
    { 2147483647,2,{
      {213,1},
      {2147483647,3}
    }
    }
  }
  },
  {16,{
    { 8,2,{
      {2861,1},
      {2147483647,3}
    }
    },
    { 2147483647,2,{
      {605,1},
      {2147483647,3}
    }
    }
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

int (*intel_allgatherv_functions_table[])(const void *sbuf, int scount,
                                               MPI_Datatype sdtype,
                                               void* rbuf, const int *rcounts,
                                               const int *rdispls,
                                               MPI_Datatype rdtype,
                                               MPI_Comm  comm
                                                    ) ={
      allgatherv__mpich_rdb,
      allgatherv__ompi_bruck,
      allgatherv__ring,
      allgatherv__GB
};

intel_tuning_table_element intel_allgatherv_table[] =
{
  {1,{
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
  }
  },
  {2,{
    { 4,3,{
      {3147,1},
      {5622,2},
      {2147483647,3}
    }
    },
    { 8,3,{
      {975,1},
      {4158,2},
      {2147483647,3}
    }
    },
    { 16,2,{
      {2146,1},
      {2147483647,3}
    }
    },
    { 32,4,{
      {81,1},
      {414,2},
      {1190,1},
      {2147483647,3}
    }
    },
    { 2147483647,5,{
      {1,2},
      {3,1},
      {783,2},
      {1782,4},
      {2147483647,3}
    }
    }
  }
  },
  {4,{
    { 8,2,{
      {2554,1},
      {2147483647,3}
    }
    },
    { 16,4,{
      {272,1},
      {657,2},
      {2078,1},
      {2147483647,3}
    }
    },
    { 32,2,{
      {1081,1},
      {2147483647,3}
    }
    },
    { 64,2,{
      {547,1},
      {2147483647,3}
    }
    },
    { 2147483647,5,{
      {19,1},
      {239,2},
      {327,1},
      {821,4},
      {2147483647,3}
    }
    }
  }
  },
  {8,{
    { 16,3,{
      {55,1},
      {514,2},
      {2147483647,3}
    }
    },
    { 32,4,{
      {53,1},
      {167,4},
      {514,2},
      {2147483647,3}
    }
    },
    { 64,3,{
      {13,1},
      {319,4},
      {2147483647,3}
    }
    },
    { 128,7,{
      {2,1},
      {11,2},
      {48,1},
      {201,2},
      {304,1},
      {1048,4},
      {2147483647,3}
    }
    },
    { 2147483647,5,{
      {5,1},
      {115,4},
      {129,1},
      {451,4},
      {2147483647,3}
    }
    }
  }
  },
  {16,{
    { 8,2,{
      {2554,1},
      {2147483647,3}
    }
    },
    { 16,4,{
      {272,1},
      {657,2},
      {2078,1},
      {2147483647,3}
    }
    },
    { 32,2,{
      {1081,1},
      {2147483647,3}
    }
    },
    { 64,2,{
      {547,1},
      {2147483647,3}
    }
    },
    { 2147483647,5,{
      {19,1},
      {239,2},
      {327,1},
      {821,4},
      {2147483647,3}
    }
    }
  }
  }
};


/* I_MPI_ADJUST_GATHER

MPI_Gather

1. Binomial algorithm
2. Topology aware binomial algorithm
3. Shumilin's algorithm

*/

int (*intel_gather_functions_table[])(const void *sbuf, int scount,
                                           MPI_Datatype sdtype,
                                           void* rbuf, int rcount,
                                           MPI_Datatype rdtype,
                                           int root,
                                           MPI_Comm  comm
                                                    ) ={
      gather__ompi_binomial,
      gather__ompi_binomial,
      gather__mvapich2
};

intel_tuning_table_element intel_gather_table[] =
{
  {1,{
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
  }
  },
  {2,{
    {2147483647,1,{
      {2147483647,3}
    }
    }
  }
  },
  {4,{
    {2147483647,1,{
      {2147483647,3}
    }
    }
  }
  },
  {8,{
    { 16,1,{
      {2147483647,3}
    }
    },
    { 32,2,{
      {9,2},
      {2147483647,3}
    }
    },
    { 64,2,{
      {784,2},
      {2147483647,3}
    }
    },
    { 128,3,{
      {160,3},
      {655,2},
      {2147483647,3}
    }
    },
    { 2147483647,1,{
      {2147483647,3}
    }
    }
  }
  },
  {16,{
    {2147483647,1,{
      {2147483647,3}
    }
    }
  }
  }
};


/* I_MPI_ADJUST_SCATTER

MPI_Scatter

1. Binomial algorithm
2. Topology aware binomial algorithm
3. Shumilin's algorithm

*/

int (*intel_scatter_functions_table[])(const void *sbuf, int scount,
                                            MPI_Datatype sdtype,
                                            void* rbuf, int rcount,
                                            MPI_Datatype rdtype,
                                            int root, MPI_Comm  comm
                                                    ) ={
      scatter__ompi_binomial,
      scatter__ompi_binomial,
      scatter__mvapich2
};

intel_tuning_table_element intel_scatter_table[] =
{
  {1,{
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
  }
  },
  {2,{
     {2147483647,1,{
       {2147483647,3}
     }
     }
  }
  },
  {4,{
    { 8,1,{
      {2147483647,3}
    }
    },
    { 16,2,{
      {140,3},
      {1302,1},
      {2147483647,3}
    }
    },
    { 32,2,{
      {159,3},
      {486,1},
      {2147483647,3}
    }
    },
    { 64,2,{
      {149,1},
      {2147483647,3}
    }
    },
    { 2147483647,2,{
      {139,1},
      {2147483647,3}
    }
    }
  }
  },
  {8,{
    { 16,4,{
      {587,1},
      {1370,2},
      {2102,1},
      {2147483647,3}
    }
    },
    { 32,3,{
      {1038,1},
      {2065,2},
      {2147483647,3}
    }
    },
    { 64,3,{
      {515,1},
      {2069,2},
      {2147483647,3}
    }
    },
    { 128,3,{
      {284,1},
      {796,2},
      {2147483647,3}
    }
    },
    { 2147483647,2,{
      {139,1},
      {2147483647,3}
    }
    }
  }
  },
  {16,{
    { 8,1,{
      {2147483647,3}
    }
    },
    { 16,3,{
      {140,3},
      {1302,1},
      {2147483647,3}
    }
    },
    { 32,3,{
      {159,3},
      {486,1},
      {2147483647,3}
    }
    },
    { 64,2,{
      {149,1},
      {2147483647,3}
    }
    },
    { 2147483647,2,{
      {139,1},
      {2147483647,3}
    }
    }
  }
  }
};



/* I_MPI_ADJUST_ALLTOALLV

MPI_Alltoallv

1. Isend/Irecv + waitall algorithm
2. Plum's algorithm

*/

int (*intel_alltoallv_functions_table[])(const void *sbuf, const int *scounts, const int *sdisps,
                                              MPI_Datatype sdtype,
                                              void *rbuf, const int *rcounts, const int *rdisps,
                                              MPI_Datatype rdtype,
                                              MPI_Comm  comm
                                                    ) ={
      alltoallv__ompi_basic_linear,
      alltoallv__bruck
};

intel_tuning_table_element intel_alltoallv_table[] =
{
  {1,{
    {2147483647,1,
        {
        {2147483647,1}
        }
    }
  }
  },
  {2,{
    {2147483647,1,
        {
        {2147483647,1}
        }
    }
  }
  },
  {4,{
    { 8,1,{
      {2147483647,1}//weirdly, intel reports the use of algo 0 here
    }
    },
    { 2147483647,2,{
      {4,1},//0 again
      {2147483647,2}
    }
    }
  }
  },
  {8,{
    { 16,1,{
      {2147483647,1}
    }
    },
    { 2147483647,2,{
      {0,1},//weird again, only for 0-sized messages
      {2147483647,2}
    }
    }
  }
  },
  {16,{
    { 8,1,{
      {2147483647,1}//0
    }
    },
    { 2147483647,2,{
      {4,1},//0
      {2147483647,2}
    }
    }
  }
  }
};


//These are collected from table 3.5-2 of the Intel MPI Reference Manual


#define SIZECOMP_reduce_scatter\
    int total_message_size = 0;\
    for (i = 0; i < comm_size; i++) { \
        total_message_size += rcounts[i];\
    }\
    size_t block_dsize = total_message_size*dtype->size();\

#define SIZECOMP_allreduce\
  size_t block_dsize =rcount * dtype->size();

#define SIZECOMP_alltoall\
  size_t block_dsize =send_count * send_type->size();

#define SIZECOMP_bcast\
  size_t block_dsize =count * datatype->size();

#define SIZECOMP_reduce\
  size_t block_dsize =count * datatype->size();

#define SIZECOMP_barrier\
  size_t block_dsize = 1;

#define SIZECOMP_allgather\
  size_t block_dsize =recv_count * recv_type->size();

#define SIZECOMP_allgatherv\
    int total_message_size = 0;\
    for (i = 0; i < comm_size; i++) { \
        total_message_size += recv_count[i];\
    }\
    size_t block_dsize = total_message_size*recv_type->size();

#define SIZECOMP_gather\
  int rank = comm->rank();\
  size_t block_dsize = (send_buff == MPI_IN_PLACE || rank ==root) ?\
                recv_count * recv_type->size() :\
                send_count * send_type->size();

#define SIZECOMP_scatter\
  int rank = comm->rank();\
  size_t block_dsize = (sendbuf == MPI_IN_PLACE || rank !=root ) ?\
                recvcount * recvtype->size() :\
                sendcount * sendtype->size();

#define SIZECOMP_alltoallv\
  size_t block_dsize = 1;

#define IMPI_COLL_SELECT(cat, ret, args, args2)                                                                        \
  ret _XBT_CONCAT2(cat, __impi)(COLL_UNPAREN args)                                                          \
  {                                                                                                                    \
    int comm_size = comm->size();                                                                                      \
    int i         = 0;                                                                                                 \
    _XBT_CONCAT(SIZECOMP_, cat)                                                                                        \
    i     = 0;                                                                                                         \
    int j = 0, k = 0;                                                                                                  \
    if (comm->get_leaders_comm() == MPI_COMM_NULL) {                                                                   \
      comm->init_smp();                                                                                                \
    }                                                                                                                  \
    int local_size = 1;                                                                                                \
    if (comm->is_uniform()) {                                                                                          \
      local_size = comm->get_intra_comm()->size();                                                                     \
    }                                                                                                                  \
    while (i < INTEL_MAX_NB_PPN && local_size != _XBT_CONCAT3(intel_, cat, _table)[i].ppn)                             \
      i++;                                                                                                             \
    if (i == INTEL_MAX_NB_PPN)                                                                                         \
      i = 0;                                                                                                           \
    while (comm_size > _XBT_CONCAT3(intel_, cat, _table)[i].elems[j].max_num_proc && j < INTEL_MAX_NB_THRESHOLDS)      \
      j++;                                                                                                             \
    while (block_dsize >= _XBT_CONCAT3(intel_, cat, _table)[i].elems[j].elems[k].max_size &&                           \
           k < _XBT_CONCAT3(intel_, cat, _table)[i].elems[j].num_elems)                                                \
      k++;                                                                                                             \
    return (_XBT_CONCAT3(intel_, cat,                                                                                  \
                         _functions_table)[_XBT_CONCAT3(intel_, cat, _table)[i].elems[j].elems[k].algo - 1] args2);    \
  }

COLL_APPLY(IMPI_COLL_SELECT, COLL_ALLGATHERV_SIG, (send_buff, send_count, send_type, recv_buff, recv_count, recv_disps, recv_type, comm))
COLL_APPLY(IMPI_COLL_SELECT, COLL_ALLREDUCE_SIG, (sbuf, rbuf, rcount, dtype, op, comm))
COLL_APPLY(IMPI_COLL_SELECT, COLL_GATHER_SIG, (send_buff, send_count, send_type, recv_buff, recv_count, recv_type, root, comm))
COLL_APPLY(IMPI_COLL_SELECT, COLL_ALLGATHER_SIG, (send_buff,send_count,send_type,recv_buff,recv_count,recv_type,comm))
COLL_APPLY(IMPI_COLL_SELECT, COLL_ALLTOALL_SIG,(send_buff, send_count, send_type, recv_buff, recv_count, recv_type,comm))
COLL_APPLY(IMPI_COLL_SELECT, COLL_ALLTOALLV_SIG, (send_buff, send_counts, send_disps, send_type, recv_buff, recv_counts, recv_disps, recv_type, comm))
COLL_APPLY(IMPI_COLL_SELECT, COLL_BCAST_SIG , (buf, count, datatype, root, comm))
COLL_APPLY(IMPI_COLL_SELECT, COLL_REDUCE_SIG,(buf,rbuf, count, datatype, op, root, comm))
COLL_APPLY(IMPI_COLL_SELECT, COLL_REDUCE_SCATTER_SIG ,(sbuf,rbuf, rcounts,dtype,op,comm))
COLL_APPLY(IMPI_COLL_SELECT, COLL_SCATTER_SIG ,(sendbuf, sendcount, sendtype,recvbuf, recvcount, recvtype,root, comm))
COLL_APPLY(IMPI_COLL_SELECT, COLL_BARRIER_SIG,(comm))

}
}
