/* selector for collective algorithms based on mvapich decision logic, with calibration from Stampede cluster at TACC*/
/* This is the tuning used by MVAPICH for Stampede platform based on (MV2_ARCH_INTEL_XEON_E5_2680_16,
 * MV2_HCA_MLX_CX_FDR) */

/* Copyright (c) 2009-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/************ Alltoall variables and initializers                        */

#ifndef SMPI_MVAPICH2_SELECTOR_STAMPEDE_HPP
#define SMPI_MVAPICH2_SELECTOR_STAMPEDE_HPP

#include <algorithm>

#define MV2_MAX_NB_THRESHOLDS 32

XBT_PUBLIC void smpi_coll_cleanup_mvapich2(void);

struct mv2_alltoall_tuning_element {
  int min;
  int max;
  int (*MV2_pt_Alltoall_function)(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                                  MPI_Datatype recvtype, MPI_Comm comm_ptr);
};

struct mv2_alltoall_tuning_table {
  int numproc;
  int size_table;
  mv2_alltoall_tuning_element algo_table[MV2_MAX_NB_THRESHOLDS];
  mv2_alltoall_tuning_element in_place_algo_table[MV2_MAX_NB_THRESHOLDS];
};

int (*MV2_Alltoall_function)(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                             MPI_Datatype recvtype, MPI_Comm comm_ptr) = NULL;

/* Indicates number of processes per node */
int* mv2_alltoall_table_ppn_conf = NULL;
/* Indicates total number of configurations */
int mv2_alltoall_num_ppn_conf                             = 1;
int* mv2_size_alltoall_tuning_table                       = NULL;
mv2_alltoall_tuning_table** mv2_alltoall_thresholds_table = NULL;

#define MPIR_Alltoall_bruck_MV2 simgrid::smpi::alltoall__bruck
#define MPIR_Alltoall_RD_MV2 simgrid::smpi::alltoall__rdb
#define MPIR_Alltoall_Scatter_dest_MV2 simgrid::smpi::alltoall__mvapich2_scatter_dest
#define MPIR_Alltoall_pairwise_MV2 simgrid::smpi::alltoall__pair
#define MPIR_Alltoall_inplace_MV2 simgrid::smpi::alltoall__ring

static void init_mv2_alltoall_tables_stampede()
{
  int agg_table_sum                      = 0;
  mv2_alltoall_tuning_table** table_ptrs = NULL;
  mv2_alltoall_num_ppn_conf              = 3;
  if (simgrid::smpi::colls::smpi_coll_cleanup_callback == NULL)
    simgrid::smpi::colls::smpi_coll_cleanup_callback = &smpi_coll_cleanup_mvapich2;
  mv2_alltoall_thresholds_table                      = new mv2_alltoall_tuning_table*[mv2_alltoall_num_ppn_conf];
  table_ptrs                                         = new mv2_alltoall_tuning_table*[mv2_alltoall_num_ppn_conf];
  mv2_size_alltoall_tuning_table                     = new int[mv2_alltoall_num_ppn_conf];
  mv2_alltoall_table_ppn_conf                        = new int[mv2_alltoall_num_ppn_conf];
  mv2_alltoall_table_ppn_conf[0]    = 1;
  mv2_size_alltoall_tuning_table[0] = 6;
  mv2_alltoall_tuning_table mv2_tmp_alltoall_thresholds_table_1ppn[] = {
      {
          2,
          1,
          {
              {0, -1, &MPIR_Alltoall_pairwise_MV2},
          },

          {
              {0, -1, &MPIR_Alltoall_inplace_MV2},
          },
      },

      {
          4,
          2,
          {
              {0, 262144, &MPIR_Alltoall_Scatter_dest_MV2}, {262144, -1, &MPIR_Alltoall_pairwise_MV2},
          },

          {
              {0, -1, &MPIR_Alltoall_inplace_MV2},
          },
      },

      {
          8,
          2,
          {
              {0, 8, &MPIR_Alltoall_RD_MV2}, {8, -1, &MPIR_Alltoall_Scatter_dest_MV2},
          },

          {
              {0, -1, &MPIR_Alltoall_inplace_MV2},
          },
      },

      {
          16,
          3,
          {
              {0, 64, &MPIR_Alltoall_RD_MV2},
              {64, 512, &MPIR_Alltoall_bruck_MV2},
              {512, -1, &MPIR_Alltoall_Scatter_dest_MV2},
          },

          {
              {0, -1, &MPIR_Alltoall_inplace_MV2},
          },
      },

      {
          32,
          3,
          {
              {0, 32, &MPIR_Alltoall_RD_MV2},
              {32, 2048, &MPIR_Alltoall_bruck_MV2},
              {2048, -1, &MPIR_Alltoall_Scatter_dest_MV2},
          },

          {
              {0, -1, &MPIR_Alltoall_inplace_MV2},
          },
      },

      {
          64,
          3,
          {
              {0, 8, &MPIR_Alltoall_RD_MV2},
              {8, 1024, &MPIR_Alltoall_bruck_MV2},
              {1024, -1, &MPIR_Alltoall_Scatter_dest_MV2},
          },

          {
              {0, -1, &MPIR_Alltoall_inplace_MV2},
          },
      },
  };
  table_ptrs[0]                                                      = mv2_tmp_alltoall_thresholds_table_1ppn;
  mv2_alltoall_table_ppn_conf[1]                                     = 2;
  mv2_size_alltoall_tuning_table[1]                                  = 6;
  mv2_alltoall_tuning_table mv2_tmp_alltoall_thresholds_table_2ppn[] = {
      {
          4,
          2,
          {
              {0, 32, &MPIR_Alltoall_RD_MV2}, {32, -1, &MPIR_Alltoall_Scatter_dest_MV2},
          },

          {
              {0, -1, &MPIR_Alltoall_inplace_MV2},
          },
      },

      {
          8,
          2,
          {
              {0, 64, &MPIR_Alltoall_RD_MV2}, {64, -1, &MPIR_Alltoall_Scatter_dest_MV2},
          },

          {
              {0, -1, &MPIR_Alltoall_inplace_MV2},
          },
      },

      {
          16,
          3,
          {
              {0, 64, &MPIR_Alltoall_RD_MV2},
              {64, 2048, &MPIR_Alltoall_bruck_MV2},
              {2048, -1, &MPIR_Alltoall_Scatter_dest_MV2},
          },

          {
              {0, -1, &MPIR_Alltoall_inplace_MV2},
          },
      },

      {
          32,
          3,
          {
              {0, 16, &MPIR_Alltoall_RD_MV2},
              {16, 2048, &MPIR_Alltoall_bruck_MV2},
              {2048, -1, &MPIR_Alltoall_Scatter_dest_MV2},
          },

          {
              {0, -1, &MPIR_Alltoall_inplace_MV2},
          },
      },

      {
          64,
          3,
          {
              {0, 8, &MPIR_Alltoall_RD_MV2},
              {8, 1024, &MPIR_Alltoall_bruck_MV2},
              {1024, -1, &MPIR_Alltoall_Scatter_dest_MV2},
          },

          {
              {0, -1, &MPIR_Alltoall_inplace_MV2},
          },
      },

      {
          128,
          3,
          {
              {0, 4, &MPIR_Alltoall_RD_MV2},
              {4, 2048, &MPIR_Alltoall_bruck_MV2},
              {2048, -1, &MPIR_Alltoall_Scatter_dest_MV2},
          },

          {
              {0, -1, &MPIR_Alltoall_inplace_MV2},
          },
      },
  };
  table_ptrs[1]                                                       = mv2_tmp_alltoall_thresholds_table_2ppn;
  mv2_alltoall_table_ppn_conf[2]                                      = 16;
  mv2_size_alltoall_tuning_table[2]                                   = 7;
  mv2_alltoall_tuning_table mv2_tmp_alltoall_thresholds_table_16ppn[] = {
      {
          16,
          2,
          {
              {0, 2048, &MPIR_Alltoall_bruck_MV2}, {2048, -1, &MPIR_Alltoall_Scatter_dest_MV2},
          },

          {
              {32768, -1, &MPIR_Alltoall_inplace_MV2},
          },
      },

      {
          32,
          2,
          {
              {0, 2048, &MPIR_Alltoall_bruck_MV2}, {2048, -1, &MPIR_Alltoall_Scatter_dest_MV2},
          },

          {
              {16384, -1, &MPIR_Alltoall_inplace_MV2},
          },
      },

      {
          64,
          3,
          {
              {0, 2048, &MPIR_Alltoall_bruck_MV2},
              {2048, 16384, &MPIR_Alltoall_Scatter_dest_MV2},
              {16384, -1, &MPIR_Alltoall_pairwise_MV2},
          },

          {
              {32768, 131072, &MPIR_Alltoall_inplace_MV2},
          },
      },

      {
          128,
          2,
          {
              {0, 2048, &MPIR_Alltoall_bruck_MV2}, {2048, -1, &MPIR_Alltoall_pairwise_MV2},
          },

          {
              {16384, 65536, &MPIR_Alltoall_inplace_MV2},
          },
      },

      {
          256,
          2,
          {
              {0, 1024, &MPIR_Alltoall_bruck_MV2}, {1024, -1, &MPIR_Alltoall_pairwise_MV2},
          },

          {
              {16384, 65536, &MPIR_Alltoall_inplace_MV2},
          },
      },

      {
          512,
          2,
          {
              {0, 1024, &MPIR_Alltoall_bruck_MV2}, {1024, -1, &MPIR_Alltoall_pairwise_MV2},
          },

          {
              {16384, 65536, &MPIR_Alltoall_inplace_MV2},
          },
      },
      {
          1024,
          2,
          {
              {0, 1024, &MPIR_Alltoall_bruck_MV2}, {1024, -1, &MPIR_Alltoall_pairwise_MV2},
          },

          {
              {16384, 65536, &MPIR_Alltoall_inplace_MV2},
          },
      },

  };
  table_ptrs[2] = mv2_tmp_alltoall_thresholds_table_16ppn;
  agg_table_sum = 0;
  for (int i = 0; i < mv2_alltoall_num_ppn_conf; i++) {
    agg_table_sum += mv2_size_alltoall_tuning_table[i];
  }
  mv2_alltoall_thresholds_table[0] = new mv2_alltoall_tuning_table[agg_table_sum];
  std::copy_n(table_ptrs[0], mv2_size_alltoall_tuning_table[0], mv2_alltoall_thresholds_table[0]);
  for (int i = 1; i < mv2_alltoall_num_ppn_conf; i++) {
    mv2_alltoall_thresholds_table[i] = mv2_alltoall_thresholds_table[i - 1] + mv2_size_alltoall_tuning_table[i - 1];
    std::copy_n(table_ptrs[i], mv2_size_alltoall_tuning_table[i], mv2_alltoall_thresholds_table[i]);
  }
  delete[] table_ptrs;
}

/************ Allgather variables and initializers                        */

struct mv2_allgather_tuning_element {
  int min;
  int max;
  int (*MV2_pt_Allgatherction)(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                               MPI_Datatype recvtype, MPI_Comm comm_ptr);
};

struct mv2_allgather_tuning_table {
  int numproc;
  int two_level[MV2_MAX_NB_THRESHOLDS];
  int size_inter_table;
  mv2_allgather_tuning_element inter_leader[MV2_MAX_NB_THRESHOLDS];
};

int (*MV2_Allgatherction)(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                          MPI_Datatype recvtype, MPI_Comm comm);

int* mv2_allgather_table_ppn_conf                           = NULL;
int mv2_allgather_num_ppn_conf                              = 1;
int* mv2_size_allgather_tuning_table                        = NULL;
mv2_allgather_tuning_table** mv2_allgather_thresholds_table = NULL;

static int MPIR_Allgather_RD_Allgather_Comm_MV2(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
                                                int recvcount, MPI_Datatype recvtype, MPI_Comm comm_ptr)
{
  return 0;
}

#define MPIR_Allgather_Bruck_MV2 simgrid::smpi::allgather__bruck
#define MPIR_Allgather_RD_MV2 simgrid::smpi::allgather__rdb
#define MPIR_Allgather_Ring_MV2 simgrid::smpi::allgather__ring
#define MPIR_2lvl_Allgather_MV2 simgrid::smpi::allgather__mvapich2_smp

static void init_mv2_allgather_tables_stampede()
{
  int agg_table_sum = 0;

  if (simgrid::smpi::colls::smpi_coll_cleanup_callback == NULL)
    simgrid::smpi::colls::smpi_coll_cleanup_callback = &smpi_coll_cleanup_mvapich2;
  mv2_allgather_num_ppn_conf                         = 3;
  mv2_allgather_thresholds_table                     = new mv2_allgather_tuning_table*[mv2_allgather_num_ppn_conf];
  mv2_allgather_tuning_table** table_ptrs            = new mv2_allgather_tuning_table*[mv2_allgather_num_ppn_conf];
  mv2_size_allgather_tuning_table                    = new int[mv2_allgather_num_ppn_conf];
  mv2_allgather_table_ppn_conf                       = new int[mv2_allgather_num_ppn_conf];
  mv2_allgather_table_ppn_conf[0]    = 1;
  mv2_size_allgather_tuning_table[0] = 6;
  mv2_allgather_tuning_table mv2_tmp_allgather_thresholds_table_1ppn[] = {
      {
          2,
          {0},
          1,
          {
              {0, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
      {
          4,
          {0, 0},
          2,
          {
              {0, 262144, &MPIR_Allgather_RD_MV2}, {262144, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
      {
          8,
          {0, 0},
          2,
          {
              {0, 131072, &MPIR_Allgather_RD_MV2}, {131072, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
      {
          16,
          {0, 0},
          2,
          {
              {0, 131072, &MPIR_Allgather_RD_MV2}, {131072, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
      {
          32,
          {0, 0},
          2,
          {
              {0, 65536, &MPIR_Allgather_RD_MV2}, {65536, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
      {
          64,
          {0, 0},
          2,
          {
              {0, 32768, &MPIR_Allgather_RD_MV2}, {32768, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
  };
  table_ptrs[0]                                                        = mv2_tmp_allgather_thresholds_table_1ppn;
  mv2_allgather_table_ppn_conf[1]                                      = 2;
  mv2_size_allgather_tuning_table[1]                                   = 6;
  mv2_allgather_tuning_table mv2_tmp_allgather_thresholds_table_2ppn[] = {
      {
          4,
          {0, 0},
          2,
          {
              {0, 524288, &MPIR_Allgather_RD_MV2}, {524288, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
      {
          8,
          {0, 1, 0},
          2,
          {
              {0, 32768, &MPIR_Allgather_RD_MV2},
              {32768, 524288, &MPIR_Allgather_Ring_MV2},
              {524288, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
      {
          16,
          {0, 1, 0},
          2,
          {
              {0, 16384, &MPIR_Allgather_RD_MV2},
              {16384, 524288, &MPIR_Allgather_Ring_MV2},
              {524288, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
      {
          32,
          {1, 1, 0},
          2,
          {
              {0, 65536, &MPIR_Allgather_RD_MV2},
              {65536, 524288, &MPIR_Allgather_Ring_MV2},
              {524288, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
      {
          64,
          {1, 1, 0},
          2,
          {
              {0, 32768, &MPIR_Allgather_RD_MV2},
              {32768, 524288, &MPIR_Allgather_Ring_MV2},
              {524288, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
      {
          128,
          {1, 1, 0},
          2,
          {
              {0, 65536, &MPIR_Allgather_RD_MV2},
              {65536, 524288, &MPIR_Allgather_Ring_MV2},
              {524288, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
  };
  table_ptrs[1]                                                         = mv2_tmp_allgather_thresholds_table_2ppn;
  mv2_allgather_table_ppn_conf[2]                                       = 16;
  mv2_size_allgather_tuning_table[2]                                    = 6;
  mv2_allgather_tuning_table mv2_tmp_allgather_thresholds_table_16ppn[] = {
      {
          16,
          {0, 0},
          2,
          {
              {0, 1024, &MPIR_Allgather_RD_MV2}, {1024, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
      {
          32,
          {0, 0},
          2,
          {
              {0, 1024, &MPIR_Allgather_RD_Allgather_Comm_MV2}, {1024, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
      {
          64,
          {0, 0},
          2,
          {
              {0, 1024, &MPIR_Allgather_RD_Allgather_Comm_MV2}, {1024, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
      {
          128,
          {0, 0},
          2,
          {
              {0, 1024, &MPIR_Allgather_RD_Allgather_Comm_MV2}, {1024, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
      {
          256,
          {0, 0},
          2,
          {
              {0, 1024, &MPIR_Allgather_RD_Allgather_Comm_MV2}, {1024, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
      {
          512,
          {0, 0},
          2,
          {
              {0, 1024, &MPIR_Allgather_RD_Allgather_Comm_MV2}, {1024, -1, &MPIR_Allgather_Ring_MV2},
          },
      },

  };
  table_ptrs[2] = mv2_tmp_allgather_thresholds_table_16ppn;
  agg_table_sum = 0;
  for (int i = 0; i < mv2_allgather_num_ppn_conf; i++) {
    agg_table_sum += mv2_size_allgather_tuning_table[i];
  }
  mv2_allgather_thresholds_table[0] = new mv2_allgather_tuning_table[agg_table_sum];
  std::copy_n(table_ptrs[0], mv2_size_allgather_tuning_table[0], mv2_allgather_thresholds_table[0]);
  for (int i = 1; i < mv2_allgather_num_ppn_conf; i++) {
    mv2_allgather_thresholds_table[i] = mv2_allgather_thresholds_table[i - 1] + mv2_size_allgather_tuning_table[i - 1];
    std::copy_n(table_ptrs[i], mv2_size_allgather_tuning_table[i], mv2_allgather_thresholds_table[i]);
  }
  delete[] table_ptrs;
}

/************ Gather variables and initializers                        */

struct mv2_gather_tuning_element {
  int min;
  int max;
  int (*MV2_pt_Gather_function)(const void* sendbuf, int sendcnt, MPI_Datatype sendtype, void* recvbuf, int recvcnt,
                                MPI_Datatype recvtype, int root, MPI_Comm comm_ptr);
};

struct mv2_gather_tuning_table {
  int numproc;
  int size_inter_table;
  mv2_gather_tuning_element inter_leader[MV2_MAX_NB_THRESHOLDS];
  int size_intra_table;
  mv2_gather_tuning_element intra_node[MV2_MAX_NB_THRESHOLDS];
};

int mv2_size_gather_tuning_table                     = 7;
mv2_gather_tuning_table* mv2_gather_thresholds_table = NULL;

typedef int (*MV2_Gather_function_ptr)(const void* sendbuf, int sendcnt, MPI_Datatype sendtype, void* recvbuf, int recvcnt,
                                       MPI_Datatype recvtype, int root, MPI_Comm comm);

MV2_Gather_function_ptr MV2_Gather_inter_leader_function = NULL;
MV2_Gather_function_ptr MV2_Gather_intra_node_function   = NULL;

#define MPIR_Gather_MV2_Direct simgrid::smpi::gather__ompi_basic_linear
#define MPIR_Gather_MV2_two_level_Direct simgrid::smpi::gather__mvapich2_two_level
#define MPIR_Gather_intra simgrid::smpi::gather__mpich

static void init_mv2_gather_tables_stampede()
{

  if (simgrid::smpi::colls::smpi_coll_cleanup_callback == NULL)
    simgrid::smpi::colls::smpi_coll_cleanup_callback = &smpi_coll_cleanup_mvapich2;
  mv2_size_gather_tuning_table                       = 7;
  mv2_gather_thresholds_table                               = new mv2_gather_tuning_table[mv2_size_gather_tuning_table];
  mv2_gather_tuning_table mv2_tmp_gather_thresholds_table[] = {
      {16,
       2,
       {{0, 524288, &MPIR_Gather_MV2_Direct}, {524288, -1, &MPIR_Gather_intra}},
       1,
       {{0, -1, &MPIR_Gather_MV2_Direct}}},
      {32,
       3,
       {{0, 16384, &MPIR_Gather_MV2_Direct},
        {16384, 131072, &MPIR_Gather_intra},
        {131072, -1, &MPIR_Gather_MV2_two_level_Direct}},
       1,
       {{0, -1, &MPIR_Gather_intra}}},
      {64,
       3,
       {{0, 256, &MPIR_Gather_MV2_two_level_Direct},
        {256, 16384, &MPIR_Gather_MV2_Direct},
        {256, -1, &MPIR_Gather_MV2_two_level_Direct}},
       1,
       {{0, -1, &MPIR_Gather_intra}}},
      {128,
       3,
       {{0, 512, &MPIR_Gather_MV2_two_level_Direct},
        {512, 16384, &MPIR_Gather_MV2_Direct},
        {16384, -1, &MPIR_Gather_MV2_two_level_Direct}},
       1,
       {{0, -1, &MPIR_Gather_intra}}},
      {256,
       3,
       {{0, 512, &MPIR_Gather_MV2_two_level_Direct},
        {512, 16384, &MPIR_Gather_MV2_Direct},
        {16384, -1, &MPIR_Gather_MV2_two_level_Direct}},
       1,
       {{0, -1, &MPIR_Gather_intra}}},
      {512,
       3,
       {{0, 512, &MPIR_Gather_MV2_two_level_Direct},
        {512, 16384, &MPIR_Gather_MV2_Direct},
        {8196, -1, &MPIR_Gather_MV2_two_level_Direct}},
       1,
       {{0, -1, &MPIR_Gather_intra}}},
      {1024,
       3,
       {{0, 512, &MPIR_Gather_MV2_two_level_Direct},
        {512, 16384, &MPIR_Gather_MV2_Direct},
        {8196, -1, &MPIR_Gather_MV2_two_level_Direct}},
       1,
       {{0, -1, &MPIR_Gather_intra}}},
  };

  std::copy_n(mv2_tmp_gather_thresholds_table, mv2_size_gather_tuning_table, mv2_gather_thresholds_table);
}

/************ Allgatherv variables and initializers                        */

struct mv2_allgatherv_tuning_element {
  int min;
  int max;
  int (*MV2_pt_Allgatherv_function)(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, const int* recvcounts,
                                    const int* displs, MPI_Datatype recvtype, MPI_Comm commg);
};

struct mv2_allgatherv_tuning_table {
  int numproc;
  int size_inter_table;
  mv2_allgatherv_tuning_element inter_leader[MV2_MAX_NB_THRESHOLDS];
};

int (*MV2_Allgatherv_function)(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, const int* recvcounts,
                               const int* displs, MPI_Datatype recvtype, MPI_Comm comm);

int mv2_size_allgatherv_tuning_table                         = 0;
mv2_allgatherv_tuning_table* mv2_allgatherv_thresholds_table = NULL;

#define MPIR_Allgatherv_Rec_Doubling_MV2 simgrid::smpi::allgatherv__mpich_rdb
#define MPIR_Allgatherv_Bruck_MV2 simgrid::smpi::allgatherv__ompi_bruck
#define MPIR_Allgatherv_Ring_MV2 simgrid::smpi::allgatherv__mpich_ring

static void init_mv2_allgatherv_tables_stampede()
{
  if (simgrid::smpi::colls::smpi_coll_cleanup_callback == NULL)
    simgrid::smpi::colls::smpi_coll_cleanup_callback = &smpi_coll_cleanup_mvapich2;
  mv2_size_allgatherv_tuning_table                   = 6;
  mv2_allgatherv_thresholds_table = new mv2_allgatherv_tuning_table[mv2_size_allgatherv_tuning_table];
  mv2_allgatherv_tuning_table mv2_tmp_allgatherv_thresholds_table[] = {
      {
          16,
          2,
          {
              {0, 512, &MPIR_Allgatherv_Rec_Doubling_MV2}, {512, -1, &MPIR_Allgatherv_Ring_MV2},
          },
      },
      {
          32,
          2,
          {
              {0, 512, &MPIR_Allgatherv_Rec_Doubling_MV2}, {512, -1, &MPIR_Allgatherv_Ring_MV2},
          },
      },
      {
          64,
          2,
          {
              {0, 256, &MPIR_Allgatherv_Rec_Doubling_MV2}, {256, -1, &MPIR_Allgatherv_Ring_MV2},
          },
      },
      {
          128,
          2,
          {
              {0, 256, &MPIR_Allgatherv_Rec_Doubling_MV2}, {256, -1, &MPIR_Allgatherv_Ring_MV2},
          },
      },
      {
          256,
          2,
          {
              {0, 256, &MPIR_Allgatherv_Rec_Doubling_MV2}, {256, -1, &MPIR_Allgatherv_Ring_MV2},
          },
      },
      {
          512,
          2,
          {
              {0, 256, &MPIR_Allgatherv_Rec_Doubling_MV2}, {256, -1, &MPIR_Allgatherv_Ring_MV2},
          },
      },

  };
  std::copy_n(mv2_tmp_allgatherv_thresholds_table, mv2_size_allgatherv_tuning_table, mv2_allgatherv_thresholds_table);
}

/************ Allreduce variables and initializers                        */

struct mv2_allreduce_tuning_element {
  int min;
  int max;
  int (*MV2_pt_Allreducection)(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                               MPI_Comm comm);
};

struct mv2_allreduce_tuning_table {
  int numproc;
  int mcast_enabled;
  int is_two_level_allreduce[MV2_MAX_NB_THRESHOLDS];
  int size_inter_table;
  mv2_allreduce_tuning_element inter_leader[MV2_MAX_NB_THRESHOLDS];
  int size_intra_table;
  mv2_allreduce_tuning_element intra_node[MV2_MAX_NB_THRESHOLDS];
};

int (*MV2_Allreducection)(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                          MPI_Comm comm) = NULL;

int (*MV2_Allreduce_intra_function)(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                                    MPI_Comm comm) = NULL;

int mv2_size_allreduce_tuning_table                        = 0;
mv2_allreduce_tuning_table* mv2_allreduce_thresholds_table = NULL;

static int MPIR_Allreduce_mcst_reduce_two_level_helper_MV2(const void* sendbuf, void* recvbuf, int count,
                                                           MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  return 0;
}

static int MPIR_Allreduce_mcst_reduce_redscat_gather_MV2(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype,
                                                         MPI_Op op, MPI_Comm comm)
{
  return 0;
}

static int MPIR_Allreduce_reduce_p2p_MV2(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                                         MPI_Comm comm)
{
  simgrid::smpi::colls::reduce(sendbuf, recvbuf, count, datatype, op, 0, comm);
  return MPI_SUCCESS;
}

static int MPIR_Allreduce_reduce_shmem_MV2(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                                           MPI_Comm comm)
{
  simgrid::smpi::colls::reduce(sendbuf, recvbuf, count, datatype, op, 0, comm);
  return MPI_SUCCESS;
}

#define MPIR_Allreduce_pt2pt_rd_MV2 simgrid::smpi::allreduce__rdb
#define MPIR_Allreduce_pt2pt_rs_MV2 simgrid::smpi::allreduce__mvapich2_rs
#define MPIR_Allreduce_two_level_MV2 simgrid::smpi::allreduce__mvapich2_two_level

static void init_mv2_allreduce_tables_stampede()
{
  if (simgrid::smpi::colls::smpi_coll_cleanup_callback == NULL)
    simgrid::smpi::colls::smpi_coll_cleanup_callback = &smpi_coll_cleanup_mvapich2;
  mv2_size_allreduce_tuning_table                    = 8;
  mv2_allreduce_thresholds_table                     = new mv2_allreduce_tuning_table[mv2_size_allreduce_tuning_table];
  mv2_allreduce_tuning_table mv2_tmp_allreduce_thresholds_table[] = {
      {
          16,
          0,
          {1, 0},
          2,
          {
              {0, 1024, &MPIR_Allreduce_pt2pt_rd_MV2}, {1024, -1, &MPIR_Allreduce_pt2pt_rs_MV2},
          },
          2,
          {
              {0, 1024, &MPIR_Allreduce_reduce_shmem_MV2}, {1024, -1, &MPIR_Allreduce_reduce_p2p_MV2},
          },
      },
      {
          32,
          0,
          {1, 1, 0},
          3,
          {
              {0, 1024, &MPIR_Allreduce_pt2pt_rd_MV2},
              {1024, 16384, &MPIR_Allreduce_pt2pt_rd_MV2},
              {16384, -1, &MPIR_Allreduce_pt2pt_rs_MV2},
          },
          2,
          {
              {0, 1024, &MPIR_Allreduce_reduce_shmem_MV2}, {1024, 16384, &MPIR_Allreduce_reduce_p2p_MV2},
          },
      },
      {
          64,
          0,
          {1, 1, 0},
          3,
          {
              {0, 512, &MPIR_Allreduce_pt2pt_rd_MV2},
              {512, 16384, &MPIR_Allreduce_pt2pt_rd_MV2},
              {16384, -1, &MPIR_Allreduce_pt2pt_rs_MV2},
          },
          2,
          {
              {0, 512, &MPIR_Allreduce_reduce_shmem_MV2}, {512, 16384, &MPIR_Allreduce_reduce_p2p_MV2},
          },
      },
      {
          128,
          0,
          {1, 1, 0},
          3,
          {
              {0, 512, &MPIR_Allreduce_pt2pt_rd_MV2},
              {512, 16384, &MPIR_Allreduce_pt2pt_rd_MV2},
              {16384, -1, &MPIR_Allreduce_pt2pt_rs_MV2},
          },
          2,
          {
              {0, 512, &MPIR_Allreduce_reduce_shmem_MV2}, {512, 16384, &MPIR_Allreduce_reduce_p2p_MV2},
          },
      },
      {
          256,
          0,
          {1, 1, 0},
          3,
          {
              {0, 512, &MPIR_Allreduce_pt2pt_rd_MV2},
              {512, 16384, &MPIR_Allreduce_pt2pt_rd_MV2},
              {16384, -1, &MPIR_Allreduce_pt2pt_rs_MV2},
          },
          2,
          {
              {0, 512, &MPIR_Allreduce_reduce_shmem_MV2}, {512, -1, &MPIR_Allreduce_reduce_p2p_MV2},
          },
      },
      {
          512,
          0,
          {1, 1, 0},
          3,
          {
              {0, 512, &MPIR_Allreduce_pt2pt_rd_MV2},
              {512, 16384, &MPIR_Allreduce_pt2pt_rd_MV2},
              {16384, -1, &MPIR_Allreduce_pt2pt_rs_MV2},
          },
          2,
          {
              {0, 512, &MPIR_Allreduce_reduce_shmem_MV2}, {512, 16384, &MPIR_Allreduce_reduce_p2p_MV2},
          },
      },
      {
          1024,
          0,
          {1, 1, 1, 0},
          4,
          {
              {0, 512, &MPIR_Allreduce_pt2pt_rd_MV2},
              {512, 8192, &MPIR_Allreduce_pt2pt_rd_MV2},
              {8192, 65536, &MPIR_Allreduce_pt2pt_rs_MV2},
              {65536, -1, &MPIR_Allreduce_pt2pt_rs_MV2},
          },
          2,
          {
              {0, 512, &MPIR_Allreduce_reduce_shmem_MV2}, {512, -1, &MPIR_Allreduce_reduce_p2p_MV2},
          },
      },
      {
          2048,
          0,
          {1, 1, 1, 0},
          4,
          {
              {0, 64, &MPIR_Allreduce_pt2pt_rd_MV2},
              {64, 512, &MPIR_Allreduce_reduce_p2p_MV2},
              {512, 4096, &MPIR_Allreduce_mcst_reduce_two_level_helper_MV2},
              {4096, 16384, &MPIR_Allreduce_pt2pt_rs_MV2},
              {16384, -1, &MPIR_Allreduce_pt2pt_rs_MV2},
          },
          2,
          {
              {0, 512, &MPIR_Allreduce_reduce_shmem_MV2}, {512, -1, &MPIR_Allreduce_reduce_p2p_MV2},
          },
      },

  };
  std::copy_n(mv2_tmp_allreduce_thresholds_table, mv2_size_allreduce_tuning_table, mv2_allreduce_thresholds_table);
}

struct mv2_bcast_tuning_element {
  int min;
  int max;
  int (*MV2_pt_Bcast_function)(void* buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm_ptr);
  int zcpy_pipelined_knomial_factor;
};

struct mv2_bcast_tuning_table {
  int numproc;
  int bcast_segment_size;
  int intra_node_knomial_factor;
  int inter_node_knomial_factor;
  int is_two_level_bcast[MV2_MAX_NB_THRESHOLDS];
  int size_inter_table;
  mv2_bcast_tuning_element inter_leader[MV2_MAX_NB_THRESHOLDS];
  int size_intra_table;
  mv2_bcast_tuning_element intra_node[MV2_MAX_NB_THRESHOLDS];
};

int mv2_size_bcast_tuning_table                    = 0;
mv2_bcast_tuning_table* mv2_bcast_thresholds_table = NULL;

int (*MV2_Bcast_function)(void* buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm_ptr) = NULL;

int (*MV2_Bcast_intra_node_function)(void* buffer, int count, MPI_Datatype datatype, int root,
                                     MPI_Comm comm_ptr) = NULL;

int zcpy_knomial_factor               = 2;
int mv2_pipelined_zcpy_knomial_factor = -1;
int bcast_segment_size                = 8192;
int mv2_inter_node_knomial_factor     = 4;
int mv2_intra_node_knomial_factor     = 4;
#define mv2_bcast_two_level_system_size 64
#define mv2_bcast_short_msg 16384
#define mv2_bcast_large_msg 512 * 1024

#define INTRA_NODE_ROOT 0

#define MPIR_Pipelined_Bcast_Zcpy_MV2 simgrid::smpi::bcast__mpich
#define MPIR_Pipelined_Bcast_MV2 simgrid::smpi::bcast__mpich
#define MPIR_Bcast_binomial_MV2 simgrid::smpi::bcast__binomial_tree
#define MPIR_Bcast_scatter_ring_allgather_shm_MV2 simgrid::smpi::bcast__scatter_LR_allgather
#define MPIR_Bcast_scatter_doubling_allgather_MV2 simgrid::smpi::bcast__scatter_rdb_allgather
#define MPIR_Bcast_scatter_ring_allgather_MV2 simgrid::smpi::bcast__scatter_LR_allgather
#define MPIR_Shmem_Bcast_MV2 simgrid::smpi::bcast__mpich
#define MPIR_Bcast_tune_inter_node_helper_MV2 simgrid::smpi::bcast__mvapich2_inter_node
#define MPIR_Bcast_inter_node_helper_MV2 simgrid::smpi::bcast__mvapich2_inter_node
#define MPIR_Knomial_Bcast_intra_node_MV2 simgrid::smpi::bcast__mvapich2_knomial_intra_node
#define MPIR_Bcast_intra_MV2 simgrid::smpi::bcast__mvapich2_intra_node

static void init_mv2_bcast_tables_stampede()
{
  // Stampede,
  if (simgrid::smpi::colls::smpi_coll_cleanup_callback == NULL)
    simgrid::smpi::colls::smpi_coll_cleanup_callback = &smpi_coll_cleanup_mvapich2;
  mv2_size_bcast_tuning_table                        = 8;
  mv2_bcast_thresholds_table                         = new mv2_bcast_tuning_table[mv2_size_bcast_tuning_table];

  mv2_bcast_tuning_table mv2_tmp_bcast_thresholds_table[] = {
      {16,
       8192,
       4,
       4,
       {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
       11,
       {{0, 8, &MPIR_Pipelined_Bcast_Zcpy_MV2, 2},
        {8, 16, &MPIR_Pipelined_Bcast_Zcpy_MV2, 4},
        {16, 1024, &MPIR_Pipelined_Bcast_Zcpy_MV2, 2},
        {1024, 8192, &MPIR_Pipelined_Bcast_Zcpy_MV2, 4},
        {8192, 16384, &MPIR_Bcast_binomial_MV2, -1},
        {16384, 32768, &MPIR_Pipelined_Bcast_Zcpy_MV2, 4},
        {32768, 65536, &MPIR_Pipelined_Bcast_Zcpy_MV2, 2},
        {65536, 131072, &MPIR_Bcast_scatter_ring_allgather_shm_MV2, -1},
        {131072, 262144, &MPIR_Bcast_scatter_ring_allgather_MV2, -1},
        {262144, 524288, &MPIR_Bcast_scatter_doubling_allgather_MV2, -1},
        {524288, -1, &MPIR_Bcast_scatter_ring_allgather_MV2, -1}},
       11,
       {{0, 8, &MPIR_Shmem_Bcast_MV2, 2},
        {8, 16, &MPIR_Shmem_Bcast_MV2, 4},
        {16, 1024, &MPIR_Shmem_Bcast_MV2, 2},
        {1024, 8192, &MPIR_Shmem_Bcast_MV2, 4},
        {8192, 16384, &MPIR_Shmem_Bcast_MV2, -1},
        {16384, 32768, &MPIR_Shmem_Bcast_MV2, 4},
        {32768, 65536, &MPIR_Shmem_Bcast_MV2, 2},
        {65536, 131072, &MPIR_Shmem_Bcast_MV2, -1},
        {131072, 262144, &MPIR_Shmem_Bcast_MV2, -1},
        {262144, 524288, &MPIR_Shmem_Bcast_MV2, -1},
        {524288, -1, &MPIR_Shmem_Bcast_MV2, -1}}},
      {32,
       8192,
       4,
       4,
       {1, 1, 1, 1, 1, 1, 1, 1},
       8,
       {{0, 128, &MPIR_Pipelined_Bcast_Zcpy_MV2, 2},
        {128, 256, &MPIR_Pipelined_Bcast_Zcpy_MV2, 4},
        {256, 32768, &MPIR_Pipelined_Bcast_Zcpy_MV2, 2},
        {32768, 65536, &MPIR_Pipelined_Bcast_Zcpy_MV2, 4},
        {65536, 131072, &MPIR_Pipelined_Bcast_Zcpy_MV2, 2},
        {131072, 262144, &MPIR_Pipelined_Bcast_Zcpy_MV2, 8},
        {262144, 524288, &MPIR_Pipelined_Bcast_Zcpy_MV2, 2},
        {524288, -1, &MPIR_Pipelined_Bcast_Zcpy_MV2, 8}},
       8,
       {{0, 128, &MPIR_Shmem_Bcast_MV2, 2},
        {128, 256, &MPIR_Shmem_Bcast_MV2, 4},
        {256, 32768, &MPIR_Shmem_Bcast_MV2, 2},
        {32768, 65536, &MPIR_Shmem_Bcast_MV2, 4},
        {65536, 131072, &MPIR_Shmem_Bcast_MV2, 2},
        {131072, 262144, &MPIR_Shmem_Bcast_MV2, 8},
        {262144, 524288, &MPIR_Shmem_Bcast_MV2, 2},
        {524288, -1, &MPIR_Shmem_Bcast_MV2, 8}}},
      {64,
       8192,
       4,
       4,
       {1, 1, 1, 1, 1, 1, 1, 1, 1},
       9,
       {{0, 2, &MPIR_Pipelined_Bcast_Zcpy_MV2, 4},
        {2, 4, &MPIR_Pipelined_Bcast_Zcpy_MV2, 8},
        {4, 16, &MPIR_Pipelined_Bcast_Zcpy_MV2, 4},
        {16, 32, &MPIR_Pipelined_Bcast_Zcpy_MV2, 8},
        {32, 128, &MPIR_Pipelined_Bcast_Zcpy_MV2, 4},
        {128, 256, &MPIR_Pipelined_Bcast_Zcpy_MV2, 8},
        {256, 4096, &MPIR_Pipelined_Bcast_Zcpy_MV2, 4},
        {4096, 32768, &MPIR_Pipelined_Bcast_Zcpy_MV2, 8},
        {32768, -1, &MPIR_Pipelined_Bcast_Zcpy_MV2, 2}},
       9,
       {{0, 2, &MPIR_Shmem_Bcast_MV2, 4},
        {2, 4, &MPIR_Shmem_Bcast_MV2, 8},
        {4, 16, &MPIR_Shmem_Bcast_MV2, 4},
        {16, 32, &MPIR_Shmem_Bcast_MV2, 8},
        {32, 128, &MPIR_Shmem_Bcast_MV2, 4},
        {128, 256, &MPIR_Shmem_Bcast_MV2, 8},
        {256, 4096, &MPIR_Shmem_Bcast_MV2, 4},
        {4096, 32768, &MPIR_Shmem_Bcast_MV2, 8},
        {32768, -1, &MPIR_Shmem_Bcast_MV2, 2}}},
      {128,
       8192,
       4,
       4,
       {1, 1, 1, 0},
       4,
       {{0, 8192, &MPIR_Pipelined_Bcast_Zcpy_MV2, 8},
        {8192, 16384, &MPIR_Pipelined_Bcast_Zcpy_MV2, 4},
        {16384, 524288, &MPIR_Pipelined_Bcast_Zcpy_MV2, 2},
        {524288, -1, &MPIR_Bcast_scatter_ring_allgather_MV2, -1}},
       4,
       {{0, 8192, &MPIR_Shmem_Bcast_MV2, 8},
        {8192, 16384, &MPIR_Shmem_Bcast_MV2, 4},
        {16384, 524288, &MPIR_Shmem_Bcast_MV2, 2},
        {524288, -1, NULL, -1}}},
      {256,
       8192,
       4,
       4,
       {1, 1, 1, 1, 1},
       5,
       {{0, 16384, &MPIR_Pipelined_Bcast_Zcpy_MV2, 4},
        {16384, 131072, &MPIR_Pipelined_Bcast_Zcpy_MV2, 2},
        {131072, 262144, &MPIR_Bcast_scatter_ring_allgather_shm_MV2, -1},
        {262144, 524288, &MPIR_Pipelined_Bcast_Zcpy_MV2, 2},
        {524288, -1, &MPIR_Bcast_scatter_ring_allgather_shm_MV2, -1}},
       5,
       {{0, 16384, &MPIR_Shmem_Bcast_MV2, 4},
        {16384, 131072, &MPIR_Shmem_Bcast_MV2, 2},
        {131072, 262144, &MPIR_Shmem_Bcast_MV2, -1},
        {262144, 524288, &MPIR_Shmem_Bcast_MV2, 2},
        {524288, -1, &MPIR_Shmem_Bcast_MV2, -1}}},
      {512,
       8192,
       4,
       4,
       {1, 1, 1, 1, 1},
       5,
       {{0, 4096, &MPIR_Pipelined_Bcast_Zcpy_MV2, 8},
        {4096, 16384, &MPIR_Pipelined_Bcast_Zcpy_MV2, 4},
        {16384, 131072, &MPIR_Pipelined_Bcast_Zcpy_MV2, 2},
        {131072, 262144, &MPIR_Pipelined_Bcast_MV2, -1},
        {262144, -1, &MPIR_Bcast_scatter_ring_allgather_shm_MV2, -1}},
       5,
       {{0, 4096, &MPIR_Shmem_Bcast_MV2, 8},
        {4096, 16384, &MPIR_Shmem_Bcast_MV2, 4},
        {16384, 131072, &MPIR_Shmem_Bcast_MV2, 2},
        {131072, 262144, &MPIR_Shmem_Bcast_MV2, -1},
        {262144, -1, &MPIR_Shmem_Bcast_MV2, -1}}},
      {1024,
       8192,
       4,
       4,
       {1, 1, 1, 1, 1},
       5,
       {{0, 8192, &MPIR_Pipelined_Bcast_Zcpy_MV2, 8},
        {8192, 16384, &MPIR_Pipelined_Bcast_Zcpy_MV2, 4},
        {16384, 65536, &MPIR_Pipelined_Bcast_Zcpy_MV2, 2},
        {65536, 524288, &MPIR_Pipelined_Bcast_MV2, -1},
        {524288, -1, &MPIR_Bcast_scatter_ring_allgather_shm_MV2, -1}},
       5,
       {{0, 8192, &MPIR_Shmem_Bcast_MV2, 8},
        {8192, 16384, &MPIR_Shmem_Bcast_MV2, 4},
        {16384, 65536, &MPIR_Shmem_Bcast_MV2, 2},
        {65536, 524288, &MPIR_Shmem_Bcast_MV2, -1},
        {524288, -1, &MPIR_Shmem_Bcast_MV2, -1}}},
      {2048,
       8192,
       4,
       4,
       {1, 1, 1, 1, 1, 1, 1},
       7,
       {{0, 16, &MPIR_Pipelined_Bcast_Zcpy_MV2, 8},
        {16, 32, &MPIR_Pipelined_Bcast_Zcpy_MV2, 4},
        {32, 4096, &MPIR_Pipelined_Bcast_Zcpy_MV2, 8},
        {4096, 16384, &MPIR_Pipelined_Bcast_Zcpy_MV2, 4},
        {16384, 32768, &MPIR_Pipelined_Bcast_Zcpy_MV2, 2},
        {32768, 524288, &MPIR_Pipelined_Bcast_MV2, -1},
        {524288, -1, &MPIR_Bcast_scatter_ring_allgather_shm_MV2, -1}},
       7,
       {{0, 16, &MPIR_Shmem_Bcast_MV2, 8},
        {16, 32, &MPIR_Shmem_Bcast_MV2, 4},
        {32, 4096, &MPIR_Shmem_Bcast_MV2, 8},
        {4096, 16384, &MPIR_Shmem_Bcast_MV2, 4},
        {16384, 32768, &MPIR_Shmem_Bcast_MV2, 2},
        {32768, 524288, &MPIR_Shmem_Bcast_MV2, -1},
        {524288, -1, &MPIR_Shmem_Bcast_MV2, -1}}}};

  std::copy_n(mv2_tmp_bcast_thresholds_table, mv2_size_bcast_tuning_table, mv2_bcast_thresholds_table);
}

/************ Reduce variables and initializers                        */

struct mv2_reduce_tuning_element {
  int min;
  int max;
  int (*MV2_pt_Reduce_function)(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root,
                                MPI_Comm comm_ptr);
};

struct mv2_reduce_tuning_table {
  int numproc;
  int inter_k_degree;
  int intra_k_degree;
  int is_two_level_reduce[MV2_MAX_NB_THRESHOLDS];
  int size_inter_table;
  mv2_reduce_tuning_element inter_leader[MV2_MAX_NB_THRESHOLDS];
  int size_intra_table;
  mv2_reduce_tuning_element intra_node[MV2_MAX_NB_THRESHOLDS];
};

int mv2_size_reduce_tuning_table                     = 0;
mv2_reduce_tuning_table* mv2_reduce_thresholds_table = NULL;

int mv2_reduce_intra_knomial_factor = -1;
int mv2_reduce_inter_knomial_factor = -1;

int (*MV2_Reduce_function)(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root,
                           MPI_Comm comm_ptr) = NULL;

int (*MV2_Reduce_intra_function)(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root,
                                 MPI_Comm comm_ptr) = NULL;

#define MPIR_Reduce_inter_knomial_wrapper_MV2 simgrid::smpi::reduce__mvapich2_knomial
#define MPIR_Reduce_intra_knomial_wrapper_MV2 simgrid::smpi::reduce__mvapich2_knomial
#define MPIR_Reduce_binomial_MV2 simgrid::smpi::reduce__binomial
#define MPIR_Reduce_redscat_gather_MV2 simgrid::smpi::reduce__scatter_gather
#define MPIR_Reduce_shmem_MV2 simgrid::smpi::reduce__ompi_basic_linear
#define MPIR_Reduce_two_level_helper_MV2 simgrid::smpi::reduce__mvapich2_two_level

static void init_mv2_reduce_tables_stampede()
{
  if (simgrid::smpi::colls::smpi_coll_cleanup_callback == NULL)
    simgrid::smpi::colls::smpi_coll_cleanup_callback = &smpi_coll_cleanup_mvapich2;
  /*Stampede*/
  mv2_size_reduce_tuning_table = 8;
  mv2_reduce_thresholds_table                               = new mv2_reduce_tuning_table[mv2_size_reduce_tuning_table];
  mv2_reduce_tuning_table mv2_tmp_reduce_thresholds_table[] = {
      {
          16,
          4,
          4,
          {1, 0, 0},
          3,
          {
              {0, 262144, &MPIR_Reduce_inter_knomial_wrapper_MV2},
              {262144, 1048576, &MPIR_Reduce_binomial_MV2},
              {1048576, -1, &MPIR_Reduce_redscat_gather_MV2},
          },
          2,
          {
              {0, 65536, &MPIR_Reduce_shmem_MV2}, {65536, -1, &MPIR_Reduce_binomial_MV2},
          },
      },
      {
          32,
          4,
          4,
          {1, 1, 1, 1, 0, 0, 0},
          7,
          {
              {0, 8192, &MPIR_Reduce_inter_knomial_wrapper_MV2},
              {8192, 16384, &MPIR_Reduce_inter_knomial_wrapper_MV2},
              {16384, 32768, &MPIR_Reduce_inter_knomial_wrapper_MV2},
              {32768, 65536, &MPIR_Reduce_binomial_MV2},
              {65536, 262144, &MPIR_Reduce_inter_knomial_wrapper_MV2},
              {262144, 1048576, &MPIR_Reduce_binomial_MV2},
              {1048576, -1, &MPIR_Reduce_redscat_gather_MV2},
          },
          6,
          {
              {0, 8192, &MPIR_Reduce_shmem_MV2},
              {8192, 16384, &MPIR_Reduce_intra_knomial_wrapper_MV2},
              {16384, 32768, &MPIR_Reduce_shmem_MV2},
              {32768, 65536, &MPIR_Reduce_shmem_MV2},
              {65536, 262144, &MPIR_Reduce_shmem_MV2},
              {262144, -1, &MPIR_Reduce_binomial_MV2},
          },
      },
      {
          64,
          4,
          4,
          {1, 1, 1, 1, 0},
          5,
          {
              {0, 8192, &MPIR_Reduce_inter_knomial_wrapper_MV2},
              {8192, 16384, &MPIR_Reduce_inter_knomial_wrapper_MV2},
              {16384, 65536, &MPIR_Reduce_binomial_MV2},
              {65536, 262144, &MPIR_Reduce_inter_knomial_wrapper_MV2},
              {262144, -1, &MPIR_Reduce_redscat_gather_MV2},
          },
          5,
          {
              {0, 8192, &MPIR_Reduce_shmem_MV2},
              {8192, 16384, &MPIR_Reduce_intra_knomial_wrapper_MV2},
              {16384, 65536, &MPIR_Reduce_shmem_MV2},
              {65536, 262144, &MPIR_Reduce_intra_knomial_wrapper_MV2},
              {262144, -1, &MPIR_Reduce_binomial_MV2},
          },
      },
      {
          128,
          4,
          4,
          {1, 0, 1, 0, 1, 0},
          6,
          {
              {0, 8192, &MPIR_Reduce_inter_knomial_wrapper_MV2},
              {8192, 16384, &MPIR_Reduce_inter_knomial_wrapper_MV2},
              {16384, 65536, &MPIR_Reduce_binomial_MV2},
              {65536, 262144, &MPIR_Reduce_inter_knomial_wrapper_MV2},
              {262144, 1048576, &MPIR_Reduce_binomial_MV2},
              {1048576, -1, &MPIR_Reduce_redscat_gather_MV2},
          },
          5,
          {
              {0, 8192, &MPIR_Reduce_shmem_MV2},
              {8192, 16384, &MPIR_Reduce_intra_knomial_wrapper_MV2},
              {16384, 65536, &MPIR_Reduce_shmem_MV2},
              {65536, 262144, &MPIR_Reduce_intra_knomial_wrapper_MV2},
              {262144, -1, &MPIR_Reduce_binomial_MV2},
          },
      },
      {
          256,
          4,
          4,
          {1, 1, 1, 0, 1, 1, 0},
          7,
          {
              {0, 8192, &MPIR_Reduce_inter_knomial_wrapper_MV2},
              {8192, 16384, &MPIR_Reduce_inter_knomial_wrapper_MV2},
              {16384, 32768, &MPIR_Reduce_binomial_MV2},
              {32768, 65536, &MPIR_Reduce_binomial_MV2},
              {65536, 262144, &MPIR_Reduce_binomial_MV2},
              {262144, 1048576, &MPIR_Reduce_binomial_MV2},
              {1048576, -1, &MPIR_Reduce_redscat_gather_MV2},
          },
          6,
          {
              {0, 8192, &MPIR_Reduce_shmem_MV2},
              {8192, 16384, &MPIR_Reduce_intra_knomial_wrapper_MV2},
              {16384, 32768, &MPIR_Reduce_shmem_MV2},
              {32768, 65536, &MPIR_Reduce_shmem_MV2},
              {65536, 262144, &MPIR_Reduce_intra_knomial_wrapper_MV2},
              {262144, -1, &MPIR_Reduce_binomial_MV2},
          },
      },
      {
          512,
          4,
          4,
          {1, 0, 1, 1, 1, 0},
          6,
          {
              {0, 8192, &MPIR_Reduce_inter_knomial_wrapper_MV2},
              {8192, 16384, &MPIR_Reduce_inter_knomial_wrapper_MV2},
              {16384, 65536, &MPIR_Reduce_binomial_MV2},
              {65536, 262144, &MPIR_Reduce_binomial_MV2},
              {262144, 1048576, &MPIR_Reduce_binomial_MV2},
              {1048576, -1, &MPIR_Reduce_redscat_gather_MV2},
          },
          5,
          {
              {0, 8192, &MPIR_Reduce_shmem_MV2},
              {8192, 16384, &MPIR_Reduce_intra_knomial_wrapper_MV2},
              {16384, 65536, &MPIR_Reduce_shmem_MV2},
              {65536, 262144, &MPIR_Reduce_intra_knomial_wrapper_MV2},
              {262144, -1, &MPIR_Reduce_binomial_MV2},
          },
      },
      {
          1024,
          4,
          4,
          {1, 0, 1, 1, 1},
          5,
          {
              {0, 8192, &MPIR_Reduce_inter_knomial_wrapper_MV2},
              {8192, 16384, &MPIR_Reduce_inter_knomial_wrapper_MV2},
              {16384, 65536, &MPIR_Reduce_binomial_MV2},
              {65536, 262144, &MPIR_Reduce_binomial_MV2},
              {262144, -1, &MPIR_Reduce_binomial_MV2},
          },
          5,
          {
              {0, 8192, &MPIR_Reduce_shmem_MV2},
              {8192, 16384, &MPIR_Reduce_intra_knomial_wrapper_MV2},
              {16384, 65536, &MPIR_Reduce_shmem_MV2},
              {65536, 262144, &MPIR_Reduce_intra_knomial_wrapper_MV2},
              {262144, -1, &MPIR_Reduce_binomial_MV2},
          },
      },
      {
          2048,
          4,
          4,
          {1, 0, 1, 1, 1, 1},
          6,
          {
              {0, 2048, &MPIR_Reduce_inter_knomial_wrapper_MV2},
              {2048, 4096, &MPIR_Reduce_inter_knomial_wrapper_MV2},
              {4096, 16384, &MPIR_Reduce_binomial_MV2},
              {16384, 65536, &MPIR_Reduce_binomial_MV2},
              {65536, 131072, &MPIR_Reduce_binomial_MV2},
              {131072, -1, &MPIR_Reduce_binomial_MV2},
          },
          6,
          {
              {0, 2048, &MPIR_Reduce_shmem_MV2},
              {2048, 4096, &MPIR_Reduce_shmem_MV2},
              {4096, 16384, &MPIR_Reduce_shmem_MV2},
              {16384, 65536, &MPIR_Reduce_intra_knomial_wrapper_MV2},
              {65536, 131072, &MPIR_Reduce_binomial_MV2},
              {131072, -1, &MPIR_Reduce_shmem_MV2},
          },
      },

  };
  std::copy_n(mv2_tmp_reduce_thresholds_table, mv2_size_reduce_tuning_table, mv2_reduce_thresholds_table);
}

/************ Reduce scatter variables and initializers                        */

struct mv2_red_scat_tuning_element {
  int min;
  int max;
  int (*MV2_pt_Red_scat_function)(const void* sendbuf, void* recvbuf, const int* recvcnts, MPI_Datatype datatype, MPI_Op op,
                                  MPI_Comm comm_ptr);
};

struct mv2_red_scat_tuning_table {
  int numproc;
  int size_inter_table;
  mv2_red_scat_tuning_element inter_leader[MV2_MAX_NB_THRESHOLDS];
};

int mv2_size_red_scat_tuning_table                       = 0;
mv2_red_scat_tuning_table* mv2_red_scat_thresholds_table = NULL;

int (*MV2_Red_scat_function)(const void* sendbuf, void* recvbuf, const int* recvcnts, MPI_Datatype datatype, MPI_Op op,
                             MPI_Comm comm_ptr);

static int MPIR_Reduce_Scatter_Basic_MV2(const void* sendbuf, void* recvbuf, const int* recvcnts, MPI_Datatype datatype, MPI_Op op,
                                         MPI_Comm comm)
{
  simgrid::smpi::reduce_scatter__default(sendbuf, recvbuf, recvcnts, datatype, op, comm);
  return MPI_SUCCESS;
}
#define MPIR_Reduce_scatter_non_comm_MV2 simgrid::smpi::reduce_scatter__mpich_noncomm
#define MPIR_Reduce_scatter_Rec_Halving_MV2 simgrid::smpi::reduce_scatter__ompi_basic_recursivehalving
#define MPIR_Reduce_scatter_Pair_Wise_MV2 simgrid::smpi::reduce_scatter__mpich_pair

static void init_mv2_reduce_scatter_tables_stampede()
{
  if (simgrid::smpi::colls::smpi_coll_cleanup_callback == NULL)
    simgrid::smpi::colls::smpi_coll_cleanup_callback = &smpi_coll_cleanup_mvapich2;
  mv2_size_red_scat_tuning_table                     = 6;
  mv2_red_scat_thresholds_table                      = new mv2_red_scat_tuning_table[mv2_size_red_scat_tuning_table];
  mv2_red_scat_tuning_table mv2_tmp_red_scat_thresholds_table[] = {
      {
          16,
          3,
          {
              {0, 64, &MPIR_Reduce_Scatter_Basic_MV2},
              {64, 65536, &MPIR_Reduce_scatter_Rec_Halving_MV2},
              {65536, -1, &MPIR_Reduce_scatter_Pair_Wise_MV2},
          },
      },
      {
          32,
          3,
          {
              {0, 64, &MPIR_Reduce_Scatter_Basic_MV2},
              {64, 131072, &MPIR_Reduce_scatter_Rec_Halving_MV2},
              {131072, -1, &MPIR_Reduce_scatter_Pair_Wise_MV2},
          },
      },
      {
          64,
          3,
          {
              {0, 1024, &MPIR_Reduce_Scatter_Basic_MV2},
              {1024, 262144, &MPIR_Reduce_scatter_Rec_Halving_MV2},
              {262144, -1, &MPIR_Reduce_scatter_Pair_Wise_MV2},
          },
      },
      {
          128,
          2,
          {
              {0, 128, &MPIR_Reduce_Scatter_Basic_MV2}, {128, -1, &MPIR_Reduce_scatter_Rec_Halving_MV2},
          },
      },
      {
          256,
          2,
          {
              {0, 128, &MPIR_Reduce_Scatter_Basic_MV2}, {128, -1, &MPIR_Reduce_scatter_Rec_Halving_MV2},
          },
      },
      {
          512,
          2,
          {
              {0, 256, &MPIR_Reduce_Scatter_Basic_MV2}, {256, -1, &MPIR_Reduce_scatter_Rec_Halving_MV2},
          },
      },

  };
  std::copy_n(mv2_tmp_red_scat_thresholds_table, mv2_size_red_scat_tuning_table, mv2_red_scat_thresholds_table);
}

/************ Scatter variables and initializers                        */

struct mv2_scatter_tuning_element {
  int min;
  int max;
  int (*MV2_pt_Scatter_function)(const void* sendbuf, int sendcnt, MPI_Datatype sendtype, void* recvbuf, int recvcnt,
                                 MPI_Datatype recvtype, int root, MPI_Comm comm);
};

struct mv2_scatter_tuning_table {
  int numproc;
  int size_inter_table;
  mv2_scatter_tuning_element inter_leader[MV2_MAX_NB_THRESHOLDS];
  int size_intra_table;
  mv2_scatter_tuning_element intra_node[MV2_MAX_NB_THRESHOLDS];
};

int* mv2_scatter_table_ppn_conf                         = NULL;
int mv2_scatter_num_ppn_conf                            = 1;
int* mv2_size_scatter_tuning_table                      = NULL;
mv2_scatter_tuning_table** mv2_scatter_thresholds_table = NULL;

int (*MV2_Scatter_function)(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                            MPI_Datatype recvtype, int root, MPI_Comm comm) = NULL;

int (*MV2_Scatter_intra_function)(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                                  MPI_Datatype recvtype, int root, MPI_Comm comm) = NULL;
int MPIR_Scatter_mcst_wrap_MV2(const void* sendbuf, int sendcnt, MPI_Datatype sendtype, void* recvbuf, int recvcnt,
                               MPI_Datatype recvtype, int root, MPI_Comm comm_ptr);

int MPIR_Scatter_mcst_wrap_MV2(const void* sendbuf, int sendcnt, MPI_Datatype sendtype, void* recvbuf, int recvcnt,
                               MPI_Datatype recvtype, int root, MPI_Comm comm_ptr)
{
  return 0;
}

#define MPIR_Scatter_MV2_Binomial simgrid::smpi::scatter__ompi_binomial
#define MPIR_Scatter_MV2_Direct simgrid::smpi::scatter__ompi_basic_linear
#define MPIR_Scatter_MV2_two_level_Binomial simgrid::smpi::scatter__mvapich2_two_level_binomial
#define MPIR_Scatter_MV2_two_level_Direct simgrid::smpi::scatter__mvapich2_two_level_direct

static void init_mv2_scatter_tables_stampede()
{
  if (simgrid::smpi::colls::smpi_coll_cleanup_callback == NULL)
    simgrid::smpi::colls::smpi_coll_cleanup_callback = &smpi_coll_cleanup_mvapich2;

  int agg_table_sum = 0;
  mv2_scatter_num_ppn_conf              = 3;
  mv2_scatter_thresholds_table          = new mv2_scatter_tuning_table*[mv2_scatter_num_ppn_conf];
  mv2_scatter_tuning_table** table_ptrs = new mv2_scatter_tuning_table*[mv2_scatter_num_ppn_conf];
  mv2_size_scatter_tuning_table         = new int[mv2_scatter_num_ppn_conf];
  mv2_scatter_table_ppn_conf            = new int[mv2_scatter_num_ppn_conf];
  mv2_scatter_table_ppn_conf[0]    = 1;
  mv2_size_scatter_tuning_table[0] = 6;
  mv2_scatter_tuning_table mv2_tmp_scatter_thresholds_table_1ppn[] = {
      {
          2,
          1,
          {
              {0, -1, &MPIR_Scatter_MV2_Binomial},
          },
          1,
          {
              {0, -1, &MPIR_Scatter_MV2_Binomial},
          },
      },

      {
          4,
          1,
          {
              {0, -1, &MPIR_Scatter_MV2_Direct},
          },
          1,
          {
              {0, -1, &MPIR_Scatter_MV2_Direct},
          },
      },

      {
          8,
          1,
          {
              {0, -1, &MPIR_Scatter_MV2_Direct},
          },
          1,
          {
              {0, -1, &MPIR_Scatter_MV2_Direct},
          },
      },

      {
          16,
          1,
          {
              {0, -1, &MPIR_Scatter_MV2_Direct},
          },
          1,
          {
              {0, -1, &MPIR_Scatter_MV2_Direct},
          },
      },

      {
          32,
          1,
          {
              {0, -1, &MPIR_Scatter_MV2_Direct},
          },
          1,
          {
              {0, -1, &MPIR_Scatter_MV2_Direct},
          },
      },

      {
          64,
          2,
          {
              {0, 32, &MPIR_Scatter_MV2_Binomial}, {32, -1, &MPIR_Scatter_MV2_Direct},
          },
          1,
          {
              {0, -1, &MPIR_Scatter_MV2_Binomial},
          },
      },
  };
  table_ptrs[0]                                                    = mv2_tmp_scatter_thresholds_table_1ppn;
  mv2_scatter_table_ppn_conf[1]                                    = 2;
  mv2_size_scatter_tuning_table[1]                                 = 6;
  mv2_scatter_tuning_table mv2_tmp_scatter_thresholds_table_2ppn[] = {
      {
          4,
          2,
          {
              {0, 4096, &MPIR_Scatter_MV2_Binomial}, {4096, -1, &MPIR_Scatter_MV2_Direct},
          },
          1,
          {
              {0, -1, &MPIR_Scatter_MV2_Direct},
          },
      },

      {
          8,
          2,
          {
              {0, 512, &MPIR_Scatter_MV2_two_level_Direct}, {512, -1, &MPIR_Scatter_MV2_Direct},
          },
          1,
          {
              {0, -1, &MPIR_Scatter_MV2_Binomial},
          },
      },

      {
          16,
          2,
          {
              {0, 2048, &MPIR_Scatter_MV2_two_level_Direct}, {2048, -1, &MPIR_Scatter_MV2_Direct},
          },
          1,
          {
              {0, -1, &MPIR_Scatter_MV2_Binomial},
          },
      },

      {
          32,
          2,
          {
              {0, 2048, &MPIR_Scatter_MV2_two_level_Direct}, {2048, -1, &MPIR_Scatter_MV2_Direct},
          },
          1,
          {
              {0, -1, &MPIR_Scatter_MV2_Binomial},
          },
      },

      {
          64,
          2,
          {
              {0, 8192, &MPIR_Scatter_MV2_two_level_Direct}, {8192, -1, &MPIR_Scatter_MV2_Direct},
          },
          1,
          {
              {0, -1, &MPIR_Scatter_MV2_Binomial},
          },
      },

      {
          128,
          4,
          {
              {0, 16, &MPIR_Scatter_MV2_Binomial},
              {16, 128, &MPIR_Scatter_MV2_two_level_Binomial},
              {128, 16384, &MPIR_Scatter_MV2_two_level_Direct},
              {16384, -1, &MPIR_Scatter_MV2_Direct},
          },
          1,
          {
              {0, 128, &MPIR_Scatter_MV2_Direct}, {128, -1, &MPIR_Scatter_MV2_Binomial},
          },
      },
  };
  table_ptrs[1]                                                     = mv2_tmp_scatter_thresholds_table_2ppn;
  mv2_scatter_table_ppn_conf[2]                                     = 16;
  mv2_size_scatter_tuning_table[2]                                  = 8;
  mv2_scatter_tuning_table mv2_tmp_scatter_thresholds_table_16ppn[] = {
      {
          16,
          2,
          {
              {0, 256, &MPIR_Scatter_MV2_Binomial}, {256, -1, &MPIR_Scatter_MV2_Direct},
          },
          1,
          {
              {0, -1, &MPIR_Scatter_MV2_Direct},
          },
      },

      {
          32,
          2,
          {
              {0, 512, &MPIR_Scatter_MV2_Binomial}, {512, -1, &MPIR_Scatter_MV2_Direct},
          },
          1,
          {
              {0, -1, &MPIR_Scatter_MV2_Direct},
          },
      },

      {
          64,
          2,
          {
              {0, 1024, &MPIR_Scatter_MV2_two_level_Direct}, {1024, -1, &MPIR_Scatter_MV2_Direct},
          },
          1,
          {
              {0, -1, &MPIR_Scatter_MV2_Direct},
          },
      },

      {
          128,
          4,
          {
              {0, 16, &MPIR_Scatter_mcst_wrap_MV2},
              {0, 16, &MPIR_Scatter_MV2_two_level_Direct},
              {16, 2048, &MPIR_Scatter_MV2_two_level_Direct},
              {2048, -1, &MPIR_Scatter_MV2_Direct},
          },
          1,
          {
              {0, -1, &MPIR_Scatter_MV2_Direct},
          },
      },

      {
          256,
          4,
          {
              {0, 16, &MPIR_Scatter_mcst_wrap_MV2},
              {0, 16, &MPIR_Scatter_MV2_two_level_Direct},
              {16, 2048, &MPIR_Scatter_MV2_two_level_Direct},
              {2048, -1, &MPIR_Scatter_MV2_Direct},
          },
          1,
          {
              {0, -1, &MPIR_Scatter_MV2_Direct},
          },
      },

      {
          512,
          4,
          {
              {0, 16, &MPIR_Scatter_mcst_wrap_MV2},
              {16, 16, &MPIR_Scatter_MV2_two_level_Direct},
              {16, 4096, &MPIR_Scatter_MV2_two_level_Direct},
              {4096, -1, &MPIR_Scatter_MV2_Direct},
          },
          1,
          {
              {0, -1, &MPIR_Scatter_MV2_Binomial},
          },
      },
      {
          1024,
          5,
          {
              {0, 16, &MPIR_Scatter_mcst_wrap_MV2},
              {0, 16, &MPIR_Scatter_MV2_Binomial},
              {16, 32, &MPIR_Scatter_MV2_Binomial},
              {32, 4096, &MPIR_Scatter_MV2_two_level_Direct},
              {4096, -1, &MPIR_Scatter_MV2_Direct},
          },
          1,
          {
              {0, -1, &MPIR_Scatter_MV2_Binomial},
          },
      },
      {
          2048,
          7,
          {
              {0, 16, &MPIR_Scatter_mcst_wrap_MV2},
              {0, 16, &MPIR_Scatter_MV2_two_level_Binomial},
              {16, 128, &MPIR_Scatter_MV2_two_level_Binomial},
              {128, 1024, &MPIR_Scatter_MV2_two_level_Direct},
              {1024, 16384, &MPIR_Scatter_MV2_two_level_Direct},
              {16384, 65536, &MPIR_Scatter_MV2_Direct},
              {65536, -1, &MPIR_Scatter_MV2_two_level_Direct},
          },
          6,
          {
              {0, 16, &MPIR_Scatter_MV2_Binomial},
              {16, 128, &MPIR_Scatter_MV2_Binomial},
              {128, 1024, &MPIR_Scatter_MV2_Binomial},
              {1024, 16384, &MPIR_Scatter_MV2_Direct},
              {16384, 65536, &MPIR_Scatter_MV2_Direct},
              {65536, -1, &MPIR_Scatter_MV2_Direct},
          },
      },
  };
  table_ptrs[2] = mv2_tmp_scatter_thresholds_table_16ppn;
  agg_table_sum = 0;
  for (int i = 0; i < mv2_scatter_num_ppn_conf; i++) {
    agg_table_sum += mv2_size_scatter_tuning_table[i];
  }
  mv2_scatter_thresholds_table[0] = new mv2_scatter_tuning_table[agg_table_sum];
  std::copy_n(table_ptrs[0], mv2_size_scatter_tuning_table[0], mv2_scatter_thresholds_table[0]);
  for (int i = 1; i < mv2_scatter_num_ppn_conf; i++) {
    mv2_scatter_thresholds_table[i] = mv2_scatter_thresholds_table[i - 1] + mv2_size_scatter_tuning_table[i - 1];
    std::copy_n(table_ptrs[i], mv2_size_scatter_tuning_table[i], mv2_scatter_thresholds_table[i]);
  }
  delete[] table_ptrs;
}

#endif
