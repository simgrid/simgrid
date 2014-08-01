/* selector for collective algorithms based on mvapich decision logic, with calibration from Stampede cluster at TACC*/

/* Copyright (c) 2009-2010, 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This is the tuning used by MVAPICH for Stampede platform based on (MV2_ARCH_INTEL_XEON_E5_2680_16, MV2_HCA_MLX_CX_FDR) */



/************ Alltoall variables and initializers                        */

#define MV2_MAX_NB_THRESHOLDS  32
typedef struct {
  int min;
  int max;
  int (*MV2_pt_Alltoall_function) (void *sendbuf, int sendcount, MPI_Datatype sendtype,
      void *recvbuf, int recvcount, MPI_Datatype recvtype,
      MPI_Comm comm_ptr );
} mv2_alltoall_tuning_element;

typedef struct {
  int numproc;
  int size_table;
  mv2_alltoall_tuning_element algo_table[MV2_MAX_NB_THRESHOLDS];
  mv2_alltoall_tuning_element in_place_algo_table[MV2_MAX_NB_THRESHOLDS];
} mv2_alltoall_tuning_table;

int (*MV2_Alltoall_function) (void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm_ptr)=NULL;

/* Indicates number of processes per node */
int *mv2_alltoall_table_ppn_conf = NULL;
/* Indicates total number of configurations */
int mv2_alltoall_num_ppn_conf = 1;
int *mv2_size_alltoall_tuning_table = NULL;
mv2_alltoall_tuning_table **mv2_alltoall_thresholds_table = NULL;


#define MPIR_Alltoall_bruck_MV2 smpi_coll_tuned_alltoall_bruck
#define MPIR_Alltoall_RD_MV2 smpi_coll_tuned_alltoall_rdb
#define MPIR_Alltoall_Scatter_dest_MV2 smpi_coll_tuned_alltoall_mvapich2_scatter_dest
#define MPIR_Alltoall_pairwise_MV2 smpi_coll_tuned_alltoall_pair
#define MPIR_Alltoall_inplace_MV2 smpi_coll_tuned_alltoall_ring 


static void init_mv2_alltoall_tables_stampede(){
  int i;
  int agg_table_sum = 0;
  mv2_alltoall_tuning_table **table_ptrs = NULL;
  mv2_alltoall_num_ppn_conf = 3;
  mv2_alltoall_thresholds_table = xbt_malloc(sizeof(mv2_alltoall_tuning_table *)
      * mv2_alltoall_num_ppn_conf);
  table_ptrs = xbt_malloc(sizeof(mv2_alltoall_tuning_table *)
      * mv2_alltoall_num_ppn_conf);
  mv2_size_alltoall_tuning_table = xbt_malloc(sizeof(int) *
      mv2_alltoall_num_ppn_conf);
  mv2_alltoall_table_ppn_conf = xbt_malloc(mv2_alltoall_num_ppn_conf * sizeof(int));
  mv2_alltoall_table_ppn_conf[0] = 1;
  mv2_size_alltoall_tuning_table[0] = 6;
  mv2_alltoall_tuning_table mv2_tmp_alltoall_thresholds_table_1ppn[] = {
      {2,
          1,
          {{0, -1, &MPIR_Alltoall_pairwise_MV2},
          },

          {{0, -1, &MPIR_Alltoall_inplace_MV2},
          },
      },

      {4,
          2,
          {{0, 262144, &MPIR_Alltoall_Scatter_dest_MV2},
              {262144, -1, &MPIR_Alltoall_pairwise_MV2},
          },

          {{0, -1, &MPIR_Alltoall_inplace_MV2},
          },
      },

      {8,
          2,
          {{0, 8, &MPIR_Alltoall_RD_MV2},
              {8, -1, &MPIR_Alltoall_Scatter_dest_MV2},
          },

          {{0, -1, &MPIR_Alltoall_inplace_MV2},
          },
      },

      {16,
          3,
          {{0, 64, &MPIR_Alltoall_RD_MV2},
              {64, 512, &MPIR_Alltoall_bruck_MV2},
              {512, -1, &MPIR_Alltoall_Scatter_dest_MV2},
          },

          {{0,-1, &MPIR_Alltoall_inplace_MV2},
          },
      },

      {32,
          3,
          {{0, 32, &MPIR_Alltoall_RD_MV2},
              {32, 2048, &MPIR_Alltoall_bruck_MV2},
              {2048, -1, &MPIR_Alltoall_Scatter_dest_MV2},
          },

          {{0, -1, &MPIR_Alltoall_inplace_MV2},
          },
      },

      {64,
          3,
          {{0, 8, &MPIR_Alltoall_RD_MV2},
              {8, 1024, &MPIR_Alltoall_bruck_MV2},
              {1024, -1, &MPIR_Alltoall_Scatter_dest_MV2},
          },

          {{0, -1, &MPIR_Alltoall_inplace_MV2},
          },
      },
  };
  table_ptrs[0] = mv2_tmp_alltoall_thresholds_table_1ppn;
  mv2_alltoall_table_ppn_conf[1] = 2;
  mv2_size_alltoall_tuning_table[1] = 6;
  mv2_alltoall_tuning_table mv2_tmp_alltoall_thresholds_table_2ppn[] = {
      {4,
          2,
          {{0, 32, &MPIR_Alltoall_RD_MV2},
              {32, -1, &MPIR_Alltoall_Scatter_dest_MV2},
          },

          {{0, -1, &MPIR_Alltoall_inplace_MV2},
          },
      },

      {8,
          2,
          {{0, 64, &MPIR_Alltoall_RD_MV2},
              {64, -1, &MPIR_Alltoall_Scatter_dest_MV2},
          },

          {{0, -1, &MPIR_Alltoall_inplace_MV2},
          },
      },

      {16,
          3,
          {{0, 64, &MPIR_Alltoall_RD_MV2},
              {64, 2048, &MPIR_Alltoall_bruck_MV2},
              {2048, -1, &MPIR_Alltoall_Scatter_dest_MV2},
          },

          {{0,-1, &MPIR_Alltoall_inplace_MV2},
          },
      },

      {32,
          3,
          {{0, 16, &MPIR_Alltoall_RD_MV2},
              {16, 2048, &MPIR_Alltoall_bruck_MV2},
              {2048, -1, &MPIR_Alltoall_Scatter_dest_MV2},
          },

          {{0, -1, &MPIR_Alltoall_inplace_MV2},
          },
      },

      {64,
          3,
          {{0, 8, &MPIR_Alltoall_RD_MV2},
              {8, 1024, &MPIR_Alltoall_bruck_MV2},
              {1024, -1, &MPIR_Alltoall_Scatter_dest_MV2},
          },

          {{0, -1, &MPIR_Alltoall_inplace_MV2},
          },
      },

      {128,
          3,
          {{0, 4, &MPIR_Alltoall_RD_MV2},
              {4, 2048, &MPIR_Alltoall_bruck_MV2},
              {2048, -1, &MPIR_Alltoall_Scatter_dest_MV2},
          },

          {{0, -1, &MPIR_Alltoall_inplace_MV2},
          },
      },
  };
  table_ptrs[1] = mv2_tmp_alltoall_thresholds_table_2ppn;
  mv2_alltoall_table_ppn_conf[2] = 16;
  mv2_size_alltoall_tuning_table[2] = 7;
  mv2_alltoall_tuning_table mv2_tmp_alltoall_thresholds_table_16ppn[] = {
      {16,
          2,
          {{0, 2048, &MPIR_Alltoall_bruck_MV2},
              {2048, -1,  &MPIR_Alltoall_Scatter_dest_MV2},
          },

          {{32768, -1, &MPIR_Alltoall_inplace_MV2},
          },
      },

      {32,
          2,
          {{0, 2048, &MPIR_Alltoall_bruck_MV2},
              {2048, -1, &MPIR_Alltoall_Scatter_dest_MV2},
          },

          {{16384, -1, &MPIR_Alltoall_inplace_MV2},
          },
      },

      {64,
          3,
          {{0, 2048, &MPIR_Alltoall_bruck_MV2},
              {2048, 16384, &MPIR_Alltoall_Scatter_dest_MV2},
              {16384, -1, &MPIR_Alltoall_pairwise_MV2},
          },

          {{32768, 131072, &MPIR_Alltoall_inplace_MV2},
          },
      },

      {128,
          2,
          {{0, 2048, &MPIR_Alltoall_bruck_MV2},
              {2048, -1, &MPIR_Alltoall_pairwise_MV2},
          },

          {{16384,65536, &MPIR_Alltoall_inplace_MV2},
          },
      },

      {256,
          2,
          {{0, 1024, &MPIR_Alltoall_bruck_MV2},
              {1024, -1, &MPIR_Alltoall_pairwise_MV2},
          },

          {{16384, 65536, &MPIR_Alltoall_inplace_MV2},
          },
      },

      {512,
          2,
          {{0, 1024, &MPIR_Alltoall_bruck_MV2},
              {1024, -1, &MPIR_Alltoall_pairwise_MV2},
          },

          {{16384, 65536, &MPIR_Alltoall_inplace_MV2},
          },
      },
      {1024,
          2,
          {{0, 1024, &MPIR_Alltoall_bruck_MV2},
              {1024, -1, &MPIR_Alltoall_pairwise_MV2},
          },

          {{16384, 65536, &MPIR_Alltoall_inplace_MV2},
          },
      },

  };
  table_ptrs[2] = mv2_tmp_alltoall_thresholds_table_16ppn;
  agg_table_sum = 0;
  for (i = 0; i < mv2_alltoall_num_ppn_conf; i++) {
      agg_table_sum += mv2_size_alltoall_tuning_table[i];
  }
  mv2_alltoall_thresholds_table[0] =
      xbt_malloc(agg_table_sum * sizeof (mv2_alltoall_tuning_table));
  memcpy(mv2_alltoall_thresholds_table[0], table_ptrs[0],
      (sizeof(mv2_alltoall_tuning_table)
          * mv2_size_alltoall_tuning_table[0]));
  for (i = 1; i < mv2_alltoall_num_ppn_conf; i++) {
      mv2_alltoall_thresholds_table[i] =
          mv2_alltoall_thresholds_table[i - 1]
                                        + mv2_size_alltoall_tuning_table[i - 1];
      memcpy(mv2_alltoall_thresholds_table[i], table_ptrs[i],
          (sizeof(mv2_alltoall_tuning_table)
              * mv2_size_alltoall_tuning_table[i]));
  }
  xbt_free(table_ptrs);


}


/************ Allgather variables and initializers                        */

typedef struct {
  int min;
  int max;
  int (*MV2_pt_Allgather_function)(void *sendbuf,
      int sendcount,
      MPI_Datatype sendtype,
      void *recvbuf,
      int recvcount,
      MPI_Datatype recvtype, MPI_Comm comm_ptr);
} mv2_allgather_tuning_element;

typedef struct {
  int numproc;
  int two_level[MV2_MAX_NB_THRESHOLDS];
  int size_inter_table;
  mv2_allgather_tuning_element inter_leader[MV2_MAX_NB_THRESHOLDS];
} mv2_allgather_tuning_table;

int (*MV2_Allgather_function)(void *sendbuf,
    int sendcount,
    MPI_Datatype sendtype,
    void *recvbuf,
    int recvcount,
    MPI_Datatype recvtype, MPI_Comm comm);

int *mv2_allgather_table_ppn_conf = NULL;
int mv2_allgather_num_ppn_conf = 1;
int *mv2_size_allgather_tuning_table = NULL;
mv2_allgather_tuning_table **mv2_allgather_thresholds_table = NULL;

static int MPIR_Allgather_RD_Allgather_Comm_MV2( void *sendbuf,
                                 int sendcount,
                                 MPI_Datatype sendtype,
                                 void *recvbuf,
                                 int recvcount,
                                 MPI_Datatype recvtype, MPI_Comm comm_ptr)
{
    return 0;
}

#define MPIR_Allgather_Bruck_MV2 smpi_coll_tuned_allgather_bruck
#define MPIR_Allgather_RD_MV2 smpi_coll_tuned_allgather_rdb
#define MPIR_Allgather_Ring_MV2 smpi_coll_tuned_allgather_ring
#define MPIR_2lvl_Allgather_MV2 smpi_coll_tuned_allgather_mvapich2_smp

static void init_mv2_allgather_tables_stampede(){
  int i;
  int agg_table_sum = 0;
  mv2_allgather_tuning_table **table_ptrs = NULL;
  mv2_allgather_num_ppn_conf = 3;
  mv2_allgather_thresholds_table
  = xbt_malloc(sizeof(mv2_allgather_tuning_table *)
      * mv2_allgather_num_ppn_conf);
  table_ptrs = xbt_malloc(sizeof(mv2_allgather_tuning_table *)
      * mv2_allgather_num_ppn_conf);
  mv2_size_allgather_tuning_table = xbt_malloc(sizeof(int) *
      mv2_allgather_num_ppn_conf);
  mv2_allgather_table_ppn_conf
  = xbt_malloc(mv2_allgather_num_ppn_conf * sizeof(int));
  mv2_allgather_table_ppn_conf[0] = 1;
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
          {0,0},
          2,
          {
              {0, 262144, &MPIR_Allgather_RD_MV2},
              {262144, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
      {
          8,
          {0,0},
          2,
          {
              {0, 131072, &MPIR_Allgather_RD_MV2},
              {131072, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
      {
          16,
          {0,0},
          2,
          {
              {0, 131072, &MPIR_Allgather_RD_MV2},
              {131072, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
      {
          32,
          {0,0},
          2,
          {
              {0, 65536, &MPIR_Allgather_RD_MV2},
              {65536, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
      {
          64,
          {0,0},
          2,
          {
              {0, 32768, &MPIR_Allgather_RD_MV2},
              {32768, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
  };
  table_ptrs[0] = mv2_tmp_allgather_thresholds_table_1ppn;
  mv2_allgather_table_ppn_conf[1] = 2;
  mv2_size_allgather_tuning_table[1] = 6;
  mv2_allgather_tuning_table mv2_tmp_allgather_thresholds_table_2ppn[] = {
      {
          4,
          {0,0},
          2,
          {
              {0, 524288, &MPIR_Allgather_RD_MV2},
              {524288, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
      {
          8,
          {0,1,0},
          2,
          {
              {0, 32768, &MPIR_Allgather_RD_MV2},
              {32768, 524288, &MPIR_Allgather_Ring_MV2},
              {524288, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
      {
          16,
          {0,1,0},
          2,
          {
              {0, 16384, &MPIR_Allgather_RD_MV2},
              {16384, 524288, &MPIR_Allgather_Ring_MV2},
              {524288, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
      {
          32,
          {1,1,0},
          2,
          {
              {0, 65536, &MPIR_Allgather_RD_MV2},
              {65536, 524288, &MPIR_Allgather_Ring_MV2},
              {524288, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
      {
          64,
          {1,1,0},
          2,
          {
              {0, 32768, &MPIR_Allgather_RD_MV2},
              {32768, 524288, &MPIR_Allgather_Ring_MV2},
              {524288, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
      {
          128,
          {1,1,0},
          2,
          {
              {0, 65536, &MPIR_Allgather_RD_MV2},
              {65536, 524288, &MPIR_Allgather_Ring_MV2},
              {524288, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
  };
  table_ptrs[1] = mv2_tmp_allgather_thresholds_table_2ppn;
  mv2_allgather_table_ppn_conf[2] = 16;
  mv2_size_allgather_tuning_table[2] = 6;
  mv2_allgather_tuning_table mv2_tmp_allgather_thresholds_table_16ppn[] = {
      {
          16,
          {0,0},
          2,
          {
              {0, 1024, &MPIR_Allgather_RD_MV2},
              {1024, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
      {
          32,
          {0,0},
          2,
          {
              {0, 1024, &MPIR_Allgather_RD_Allgather_Comm_MV2},
              {1024, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
      {
          64,
          {0,0},
          2,
          {
              {0, 1024, &MPIR_Allgather_RD_Allgather_Comm_MV2},
              {1024, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
      {
          128,
          {0,0},
          2,
          {
              {0, 1024, &MPIR_Allgather_RD_Allgather_Comm_MV2},
              {1024, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
      {
          256,
          {0,0},
          2,
          {
              {0, 1024, &MPIR_Allgather_RD_Allgather_Comm_MV2},
              {1024, -1, &MPIR_Allgather_Ring_MV2},
          },
      },
      {
          512,
          {0,0},
          2,
          {
              {0, 1024, &MPIR_Allgather_RD_Allgather_Comm_MV2},
              {1024, -1, &MPIR_Allgather_Ring_MV2},
          },
      },

  };
  table_ptrs[2] = mv2_tmp_allgather_thresholds_table_16ppn;
  agg_table_sum = 0;
  for (i = 0; i < mv2_allgather_num_ppn_conf; i++) {
      agg_table_sum += mv2_size_allgather_tuning_table[i];
  }
  mv2_allgather_thresholds_table[0] =
      xbt_malloc(agg_table_sum * sizeof (mv2_allgather_tuning_table));
  memcpy(mv2_allgather_thresholds_table[0], table_ptrs[0],
      (sizeof(mv2_allgather_tuning_table)
          * mv2_size_allgather_tuning_table[0]));
  for (i = 1; i < mv2_allgather_num_ppn_conf; i++) {
      mv2_allgather_thresholds_table[i] =
          mv2_allgather_thresholds_table[i - 1]
                                         + mv2_size_allgather_tuning_table[i - 1];
      memcpy(mv2_allgather_thresholds_table[i], table_ptrs[i],
          (sizeof(mv2_allgather_tuning_table)
              * mv2_size_allgather_tuning_table[i]));
  }
  xbt_free(table_ptrs);
}


/************ Gather variables and initializers                        */

typedef struct {
  int min;
  int max;
  int (*MV2_pt_Gather_function)(void *sendbuf, int sendcnt,
      MPI_Datatype sendtype, void *recvbuf, int recvcnt,
      MPI_Datatype recvtype, int root, MPI_Comm  comm_ptr);
} mv2_gather_tuning_element;


typedef struct {
  int numproc;
  int size_inter_table;
  mv2_gather_tuning_element inter_leader[MV2_MAX_NB_THRESHOLDS];
  int size_intra_table;
  mv2_gather_tuning_element intra_node[MV2_MAX_NB_THRESHOLDS];
} mv2_gather_tuning_table;

int mv2_size_gather_tuning_table=7;
mv2_gather_tuning_table * mv2_gather_thresholds_table=NULL; 

typedef int (*MV2_Gather_function_ptr) (void *sendbuf,
    int sendcnt,
    MPI_Datatype sendtype,
    void *recvbuf,
    int recvcnt,
    MPI_Datatype recvtype,
    int root, MPI_Comm comm);

MV2_Gather_function_ptr MV2_Gather_inter_leader_function = NULL;
MV2_Gather_function_ptr MV2_Gather_intra_node_function = NULL;


#define MPIR_Gather_MV2_Direct smpi_coll_tuned_gather_ompi_basic_linear
#define MPIR_Gather_MV2_two_level_Direct smpi_coll_tuned_gather_mvapich2_two_level
#define MPIR_Gather_intra smpi_coll_tuned_gather_mpich


static void init_mv2_gather_tables_stampede(){

  mv2_size_gather_tuning_table=7;
  mv2_gather_thresholds_table = xbt_malloc(mv2_size_gather_tuning_table*
      sizeof (mv2_gather_tuning_table));
  mv2_gather_tuning_table mv2_tmp_gather_thresholds_table[]={
      {16,
          2,{{0, 524288, &MPIR_Gather_MV2_Direct},
              {524288, -1, &MPIR_Gather_intra}},
              1,{{0, -1, &MPIR_Gather_MV2_Direct}}},
              {32,
                  3,{{0, 16384, &MPIR_Gather_MV2_Direct},
                      {16384, 131072, &MPIR_Gather_intra},
                      {131072, -1, &MPIR_Gather_MV2_two_level_Direct}},
                      1,{{0, -1, &MPIR_Gather_intra}}},
                      {64,
                          3,{{0, 256, &MPIR_Gather_MV2_two_level_Direct},
                              {256, 16384, &MPIR_Gather_MV2_Direct},
                              {256, -1, &MPIR_Gather_MV2_two_level_Direct}},
                              1,{{0, -1, &MPIR_Gather_intra}}},
                              {128,
                                  3,{{0, 512, &MPIR_Gather_MV2_two_level_Direct},
                                      {512, 16384, &MPIR_Gather_MV2_Direct},
                                      {16384, -1, &MPIR_Gather_MV2_two_level_Direct}},
                                      1,{{0, -1, &MPIR_Gather_intra}}},
                                      {256,
                                          3,{{0, 512, &MPIR_Gather_MV2_two_level_Direct},
                                              {512, 16384, &MPIR_Gather_MV2_Direct},
                                              {16384, -1, &MPIR_Gather_MV2_two_level_Direct}},
                                              1,{{0, -1, &MPIR_Gather_intra}}},
                                              {512,
                                                  3,{{0, 512, &MPIR_Gather_MV2_two_level_Direct},
                                                      {512, 16384, &MPIR_Gather_MV2_Direct},
                                                      {8196, -1, &MPIR_Gather_MV2_two_level_Direct}},
                                                      1,{{0, -1, &MPIR_Gather_intra}}},
                                                      {1024,
                                                          3,{{0, 512, &MPIR_Gather_MV2_two_level_Direct},
                                                              {512, 16384, &MPIR_Gather_MV2_Direct},
                                                              {8196, -1, &MPIR_Gather_MV2_two_level_Direct}},
                                                              1,{{0, -1, &MPIR_Gather_intra}}},
  };

  memcpy(mv2_gather_thresholds_table, mv2_tmp_gather_thresholds_table,
      mv2_size_gather_tuning_table * sizeof (mv2_gather_tuning_table));

}


/************ Allgatherv variables and initializers                        */

typedef struct {
  int min;
  int max;
  int (*MV2_pt_Allgatherv_function)(void *sendbuf,
      int sendcount,
      MPI_Datatype sendtype,
      void *recvbuf,
      int *recvcounts,
      int *displs,
      MPI_Datatype recvtype,
      MPI_Comm commg);
} mv2_allgatherv_tuning_element;

typedef struct {
  int numproc;
  int size_inter_table;
  mv2_allgatherv_tuning_element inter_leader[MV2_MAX_NB_THRESHOLDS];
} mv2_allgatherv_tuning_table;

int (*MV2_Allgatherv_function)(void *sendbuf,
    int sendcount,
    MPI_Datatype sendtype,
    void *recvbuf,
    int *recvcounts,
    int *displs,
    MPI_Datatype recvtype,
    MPI_Comm comm);

int mv2_size_allgatherv_tuning_table = 0;
mv2_allgatherv_tuning_table *mv2_allgatherv_thresholds_table = NULL;

#define MPIR_Allgatherv_Rec_Doubling_MV2 smpi_coll_tuned_allgatherv_mpich_rdb
#define MPIR_Allgatherv_Bruck_MV2 smpi_coll_tuned_allgatherv_ompi_bruck
#define MPIR_Allgatherv_Ring_MV2 smpi_coll_tuned_allgatherv_mpich_ring


static void init_mv2_allgatherv_tables_stampede(){
  mv2_size_allgatherv_tuning_table = 6;
  mv2_allgatherv_thresholds_table = xbt_malloc(mv2_size_allgatherv_tuning_table *
      sizeof (mv2_allgatherv_tuning_table));
  mv2_allgatherv_tuning_table mv2_tmp_allgatherv_thresholds_table[] = {
      {
          16,
          2,
          {
              {0, 512, &MPIR_Allgatherv_Rec_Doubling_MV2},
              {512, -1, &MPIR_Allgatherv_Ring_MV2},
          },
      },
      {
          32,
          2,
          {
              {0, 512, &MPIR_Allgatherv_Rec_Doubling_MV2},
              {512, -1, &MPIR_Allgatherv_Ring_MV2},
          },
      },
      {
          64,
          2,
          {
              {0, 256, &MPIR_Allgatherv_Rec_Doubling_MV2},
              {256, -1, &MPIR_Allgatherv_Ring_MV2},
          },
      },
      {
          128,
          2,
          {
              {0, 256, &MPIR_Allgatherv_Rec_Doubling_MV2},
              {256, -1, &MPIR_Allgatherv_Ring_MV2},
          },
      },
      {
          256,
          2,
          {
              {0, 256, &MPIR_Allgatherv_Rec_Doubling_MV2},
              {256, -1, &MPIR_Allgatherv_Ring_MV2},
          },
      },
      {
          512,
          2,
          {
              {0, 256, &MPIR_Allgatherv_Rec_Doubling_MV2},
              {256, -1, &MPIR_Allgatherv_Ring_MV2},
          },
      },

  };
  memcpy(mv2_allgatherv_thresholds_table, mv2_tmp_allgatherv_thresholds_table,
      mv2_size_allgatherv_tuning_table * sizeof (mv2_allgatherv_tuning_table));
}


/************ Allreduce variables and initializers                        */

typedef struct {
  int min;
  int max;
  int (*MV2_pt_Allreduce_function)(void *sendbuf,
      void *recvbuf,
      int count,
      MPI_Datatype datatype,
      MPI_Op op, MPI_Comm comm);
} mv2_allreduce_tuning_element;

typedef struct {
  int numproc;
  int mcast_enabled;
  int is_two_level_allreduce[MV2_MAX_NB_THRESHOLDS];
  int size_inter_table;
  mv2_allreduce_tuning_element inter_leader[MV2_MAX_NB_THRESHOLDS];
  int size_intra_table;
  mv2_allreduce_tuning_element intra_node[MV2_MAX_NB_THRESHOLDS];
} mv2_allreduce_tuning_table;


int (*MV2_Allreduce_function)(void *sendbuf,
    void *recvbuf,
    int count,
    MPI_Datatype datatype,
    MPI_Op op, MPI_Comm comm)=NULL;


int (*MV2_Allreduce_intra_function)( void *sendbuf,
    void *recvbuf,
    int count,
    MPI_Datatype datatype,
    MPI_Op op, MPI_Comm comm)=NULL;

int mv2_size_allreduce_tuning_table = 0;
mv2_allreduce_tuning_table *mv2_allreduce_thresholds_table = NULL;





static int MPIR_Allreduce_mcst_reduce_two_level_helper_MV2( void *sendbuf,
    void *recvbuf,
    int count,
    MPI_Datatype datatype,
    MPI_Op op, MPI_Comm comm)
{ 
  return 0;
}

static  int MPIR_Allreduce_mcst_reduce_redscat_gather_MV2( void *sendbuf,
    void *recvbuf,
    int count,
    MPI_Datatype datatype,
    MPI_Op op, MPI_Comm  comm)
{
  return 0;
}

static  int MPIR_Allreduce_reduce_p2p_MV2( void *sendbuf,
    void *recvbuf,
    int count,
    MPI_Datatype datatype,
    MPI_Op op, MPI_Comm  comm)
{
  mpi_coll_reduce_fun(sendbuf,recvbuf,count,datatype,op,0,comm);
  return MPI_SUCCESS;
}

static  int MPIR_Allreduce_reduce_shmem_MV2( void *sendbuf,
    void *recvbuf,
    int count,
    MPI_Datatype datatype,
    MPI_Op op, MPI_Comm  comm)
{
  mpi_coll_reduce_fun(sendbuf,recvbuf,count,datatype,op,0,comm);
  return MPI_SUCCESS;
}

#define MPIR_Allreduce_pt2pt_rd_MV2 smpi_coll_tuned_allreduce_rdb
#define MPIR_Allreduce_pt2pt_rs_MV2 smpi_coll_tuned_allreduce_mvapich2_rs
#define MPIR_Allreduce_two_level_MV2 smpi_coll_tuned_allreduce_mvapich2_two_level


static void init_mv2_allreduce_tables_stampede(){
  mv2_size_allreduce_tuning_table = 8;
  mv2_allreduce_thresholds_table = xbt_malloc(mv2_size_allreduce_tuning_table *
      sizeof (mv2_allreduce_tuning_table));
  mv2_allreduce_tuning_table mv2_tmp_allreduce_thresholds_table[] = {
      {
          16,
          0,
          {1, 0},
          2,
          {
              {0, 1024, &MPIR_Allreduce_pt2pt_rd_MV2},
              {1024, -1, &MPIR_Allreduce_pt2pt_rs_MV2},
          },
          2,
          {
              {0, 1024, &MPIR_Allreduce_reduce_shmem_MV2},
              {1024, -1, &MPIR_Allreduce_reduce_p2p_MV2},
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
              {0, 1024, &MPIR_Allreduce_reduce_shmem_MV2},
              {1024, 16384, &MPIR_Allreduce_reduce_p2p_MV2},
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
              {0, 512, &MPIR_Allreduce_reduce_shmem_MV2},
              {512, 16384, &MPIR_Allreduce_reduce_p2p_MV2},
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
              {0, 512, &MPIR_Allreduce_reduce_shmem_MV2},
              {512, 16384, &MPIR_Allreduce_reduce_p2p_MV2},
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
              {0, 512, &MPIR_Allreduce_reduce_shmem_MV2},
              {512, -1, &MPIR_Allreduce_reduce_p2p_MV2},
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
              {0, 512, &MPIR_Allreduce_reduce_shmem_MV2},
              {512, 16384, &MPIR_Allreduce_reduce_p2p_MV2},
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
              {0, 512, &MPIR_Allreduce_reduce_shmem_MV2},
              {512, -1, &MPIR_Allreduce_reduce_p2p_MV2},
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
              {0, 512, &MPIR_Allreduce_reduce_shmem_MV2},
              {512, -1, &MPIR_Allreduce_reduce_p2p_MV2},
          },
      },

  };
  memcpy(mv2_allreduce_thresholds_table, mv2_tmp_allreduce_thresholds_table,
      mv2_size_allreduce_tuning_table * sizeof (mv2_allreduce_tuning_table));
}


/*
Bcast deactivated for now, defaults to mpich one
typedef struct {
    int min;
    int max;
    int (*MV2_pt_Bcast_function) (void *buf, int count, MPI_Datatype datatype,
                                  int root, MPI_Comm comm_ptr);
    int zcpy_pipelined_knomial_factor;
} mv2_bcast_tuning_element;

typedef struct {
    int numproc;
    int bcast_segment_size;
    int intra_node_knomial_factor;
    int inter_node_knomial_factor;
    int is_two_level_bcast[MV2_MAX_NB_THRESHOLDS];
    int size_inter_table;
    mv2_bcast_tuning_element inter_leader[MV2_MAX_NB_THRESHOLDS];
    int size_intra_table;
    mv2_bcast_tuning_element intra_node[MV2_MAX_NB_THRESHOLDS];
} mv2_bcast_tuning_table;

int mv2_size_bcast_tuning_table = 0;
mv2_bcast_tuning_table *mv2_bcast_thresholds_table = NULL;


int (*MV2_Bcast_function) (void *buffer, int count, MPI_Datatype datatype,
                           int root, MPI_Comm comm_ptr) = NULL;

int (*MV2_Bcast_intra_node_function) (void *buffer, int count, MPI_Datatype datatype,
                                      int root, MPI_Comm comm_ptr) = NULL;


 */


/*
static void init_mv2_bcast_tables_stampede(){
 //Stampede,
        mv2_size_bcast_tuning_table=8;
        mv2_bcast_thresholds_table = xbt_malloc(mv2_size_bcast_tuning_table *
                                                 sizeof (mv2_bcast_tuning_table));

  mv2_bcast_tuning_table mv2_tmp_bcast_thresholds_table[]={
    {
            16,
            8192, 4, 4,
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            11,
            {
              {0, 8, &MPIR_Pipelined_Bcast_Zcpy_MV2, 2},
              {8, 16, &MPIR_Pipelined_Bcast_Zcpy_MV2, 4},
              {16, 1024, &MPIR_Pipelined_Bcast_Zcpy_MV2, 2},
              {1024, 8192, &MPIR_Pipelined_Bcast_Zcpy_MV2, 4},
              {8192, 16384, &MPIR_Bcast_binomial_MV2, -1},
              {16384, 32768, &MPIR_Pipelined_Bcast_Zcpy_MV2, 4},
              {32768, 65536, &MPIR_Pipelined_Bcast_Zcpy_MV2, 2},
              {65536, 131072, &MPIR_Bcast_scatter_ring_allgather_shm_MV2, -1},
              {131072, 262144, &MPIR_Bcast_scatter_ring_allgather_MV2, -1},
              {262144, 524288, &MPIR_Bcast_scatter_doubling_allgather_MV2, -1},
              {524288, -1, &MPIR_Bcast_scatter_ring_allgather_MV2, -1}
            },
            11,
            {
              {0, 8, &MPIR_Shmem_Bcast_MV2, 2},
              {8, 16, &MPIR_Shmem_Bcast_MV2, 4},
              {16, 1024, &MPIR_Shmem_Bcast_MV2, 2},
              {1024, 8192, &MPIR_Shmem_Bcast_MV2, 4},
              {8192, 16384, &MPIR_Shmem_Bcast_MV2, -1},
              {16384, 32768, &MPIR_Shmem_Bcast_MV2, 4},
              {32768, 65536, &MPIR_Shmem_Bcast_MV2, 2},
              {65536, 131072, &MPIR_Shmem_Bcast_MV2, -1},
              {131072, 262144, &MPIR_Shmem_Bcast_MV2, -1},
              {262144, 524288, &MPIR_Shmem_Bcast_MV2, -1},
              {524288, -1, &MPIR_Shmem_Bcast_MV2, -1}
            }
    },
    {
            32,
            8192, 4, 4,
            {1, 1, 1, 1, 1, 1, 1, 1},
            8,
            {
              {0, 128, &MPIR_Pipelined_Bcast_Zcpy_MV2, 2},
              {128, 256, &MPIR_Pipelined_Bcast_Zcpy_MV2, 4},
              {256, 32768, &MPIR_Pipelined_Bcast_Zcpy_MV2, 2},
              {32768, 65536, &MPIR_Pipelined_Bcast_Zcpy_MV2, 4},
              {65536, 131072, &MPIR_Pipelined_Bcast_Zcpy_MV2, 2},
              {131072, 262144, &MPIR_Pipelined_Bcast_Zcpy_MV2, 8},
              {262144, 524288, &MPIR_Pipelined_Bcast_Zcpy_MV2, 2},
              {524288, -1, &MPIR_Pipelined_Bcast_Zcpy_MV2, 8}
            },
            8,
            {
              {0, 128, &MPIR_Shmem_Bcast_MV2, 2},
              {128, 256, &MPIR_Shmem_Bcast_MV2, 4},
              {256, 32768, &MPIR_Shmem_Bcast_MV2, 2},
              {32768, 65536, &MPIR_Shmem_Bcast_MV2, 4},
              {65536, 131072, &MPIR_Shmem_Bcast_MV2, 2},
              {131072, 262144, &MPIR_Shmem_Bcast_MV2, 8},
              {262144, 524288, &MPIR_Shmem_Bcast_MV2, 2},
              {524288, -1, &MPIR_Shmem_Bcast_MV2, 8}
            }
    },
    {
            64,
            8192, 4, 4,
            {1, 1, 1, 1, 1, 1, 1, 1, 1},
            9,
            {
              {0, 2, &MPIR_Pipelined_Bcast_Zcpy_MV2, 4},
              {2, 4, &MPIR_Pipelined_Bcast_Zcpy_MV2, 8},
              {4, 16, &MPIR_Pipelined_Bcast_Zcpy_MV2, 4},
              {16, 32, &MPIR_Pipelined_Bcast_Zcpy_MV2, 8},
              {32, 128, &MPIR_Pipelined_Bcast_Zcpy_MV2, 4},
              {128, 256, &MPIR_Pipelined_Bcast_Zcpy_MV2, 8},
              {256, 4096, &MPIR_Pipelined_Bcast_Zcpy_MV2, 4},
              {4096, 32768, &MPIR_Pipelined_Bcast_Zcpy_MV2, 8},
              {32768, -1, &MPIR_Pipelined_Bcast_Zcpy_MV2, 2}
            },
            9,
            {
              {0, 2, &MPIR_Shmem_Bcast_MV2, 4},
              {2, 4, &MPIR_Shmem_Bcast_MV2, 8},
              {4, 16, &MPIR_Shmem_Bcast_MV2, 4},
              {16, 32, &MPIR_Shmem_Bcast_MV2, 8},
              {32, 128, &MPIR_Shmem_Bcast_MV2, 4},
              {128, 256, &MPIR_Shmem_Bcast_MV2, 8},
              {256, 4096, &MPIR_Shmem_Bcast_MV2, 4},
              {4096, 32768, &MPIR_Shmem_Bcast_MV2, 8},
              {32768, -1, &MPIR_Shmem_Bcast_MV2, 2}
            }
    },
    {
            128,
            8192, 4, 4,
            {1, 1, 1, 0},
            4,
            {
              {0, 8192, &MPIR_Pipelined_Bcast_Zcpy_MV2, 8},
              {8192, 16384, &MPIR_Pipelined_Bcast_Zcpy_MV2, 4},
              {16384, 524288, &MPIR_Pipelined_Bcast_Zcpy_MV2, 2},
              {524288, -1, &MPIR_Bcast_scatter_ring_allgather_MV2, -1}
            },
            4,
            {
              {0, 8192, &MPIR_Shmem_Bcast_MV2, 8},
              {8192, 16384, &MPIR_Shmem_Bcast_MV2, 4},
              {16384, 524288, &MPIR_Shmem_Bcast_MV2, 2},
              {524288, -1, NULL, -1}
            }
    },
    {
            256,
            8192, 4, 4,
            {1, 1, 1, 1, 1},
            5,
            {
              {0, 16384, &MPIR_Pipelined_Bcast_Zcpy_MV2, 4},
              {16384, 131072, &MPIR_Pipelined_Bcast_Zcpy_MV2, 2},
              {131072, 262144, &MPIR_Bcast_scatter_ring_allgather_shm_MV2, -1},
              {262144, 524288, &MPIR_Pipelined_Bcast_Zcpy_MV2, 2},
              {524288, -1, &MPIR_Bcast_scatter_ring_allgather_shm_MV2, -1}
            },
            5,
            {
              {0, 16384, &MPIR_Shmem_Bcast_MV2, 4},
              {16384, 131072, &MPIR_Shmem_Bcast_MV2, 2},
              {131072, 262144, &MPIR_Shmem_Bcast_MV2, -1},
              {262144, 524288, &MPIR_Shmem_Bcast_MV2, 2},
              {524288, -1, &MPIR_Shmem_Bcast_MV2, -1}
            }
    },
    {
            512,
            8192, 4, 4,
            {1, 1, 1, 1, 1},
            5,
            {
              {0, 4096, &MPIR_Pipelined_Bcast_Zcpy_MV2, 8},
              {4096, 16384, &MPIR_Pipelined_Bcast_Zcpy_MV2, 4},
              {16384, 131072, &MPIR_Pipelined_Bcast_Zcpy_MV2, 2},
              {131072, 262144, &MPIR_Pipelined_Bcast_MV2, -1},
              {262144, -1, &MPIR_Bcast_scatter_ring_allgather_shm_MV2, -1}
            },
            5,
            {
              {0, 4096, &MPIR_Shmem_Bcast_MV2, 8},
              {4096, 16384, &MPIR_Shmem_Bcast_MV2, 4},
              {16384, 131072, &MPIR_Shmem_Bcast_MV2, 2},
              {131072, 262144, &MPIR_Shmem_Bcast_MV2, -1},
              {262144, -1, &MPIR_Shmem_Bcast_MV2, -1}
            }
    },
    {
            1024,
            8192, 4, 4,
            {1, 1, 1, 1, 1},
            5,
            {
              {0, 8192, &MPIR_Pipelined_Bcast_Zcpy_MV2, 8},
              {8192, 16384, &MPIR_Pipelined_Bcast_Zcpy_MV2, 4},
              {16384, 65536, &MPIR_Pipelined_Bcast_Zcpy_MV2, 2},
              {65536, 524288, &MPIR_Pipelined_Bcast_MV2, -1},
              {524288, -1, &MPIR_Bcast_scatter_ring_allgather_shm_MV2, -1}
            },
            5,
            {
              {0, 8192, &MPIR_Shmem_Bcast_MV2, 8},
              {8192, 16384, &MPIR_Shmem_Bcast_MV2, 4},
              {16384, 65536, &MPIR_Shmem_Bcast_MV2, 2},
              {65536, 524288, &MPIR_Shmem_Bcast_MV2, -1},
              {524288, -1, &MPIR_Shmem_Bcast_MV2, -1}
            }
    },
    {
            2048,
            8192, 4, 4,
            {1, 1, 1, 1, 1, 1, 1},
            7,
            {
              {0, 16, &MPIR_Pipelined_Bcast_Zcpy_MV2, 8},
              {16, 32, &MPIR_Pipelined_Bcast_Zcpy_MV2, 4},
              {32, 4096, &MPIR_Pipelined_Bcast_Zcpy_MV2, 8},
              {4096, 16384, &MPIR_Pipelined_Bcast_Zcpy_MV2, 4},
              {16384, 32768, &MPIR_Pipelined_Bcast_Zcpy_MV2, 2},
              {32768, 524288, &MPIR_Pipelined_Bcast_MV2, -1},
              {524288, -1, &MPIR_Bcast_scatter_ring_allgather_shm_MV2, -1}
            },
            7,
            {
              {0, 16, &MPIR_Shmem_Bcast_MV2, 8},
              {16, 32, &MPIR_Shmem_Bcast_MV2, 4},
              {32, 4096, &MPIR_Shmem_Bcast_MV2, 8},
              {4096, 16384, &MPIR_Shmem_Bcast_MV2, 4},
              {16384, 32768, &MPIR_Shmem_Bcast_MV2, 2},
              {32768, 524288, &MPIR_Shmem_Bcast_MV2, -1},
              {524288, -1, &MPIR_Shmem_Bcast_MV2, -1}
            }
    }
  };

        memcpy(mv2_bcast_thresholds_table, mv2_tmp_bcast_thresholds_table,
                    mv2_size_bcast_tuning_table * sizeof (mv2_bcast_tuning_table));
}*/


/************ Reduce variables and initializers                        */

typedef struct {
  int min;
  int max;
  int (*MV2_pt_Reduce_function)(void *sendbuf,
      void *recvbuf,
      int count,
      MPI_Datatype datatype,
      MPI_Op op,
      int root,
      MPI_Comm  comm_ptr);
} mv2_reduce_tuning_element;

typedef struct {
  int numproc;
  int inter_k_degree;
  int intra_k_degree;
  int is_two_level_reduce[MV2_MAX_NB_THRESHOLDS];
  int size_inter_table;
  mv2_reduce_tuning_element inter_leader[MV2_MAX_NB_THRESHOLDS];
  int size_intra_table;
  mv2_reduce_tuning_element intra_node[MV2_MAX_NB_THRESHOLDS];
} mv2_reduce_tuning_table;

int mv2_size_reduce_tuning_table = 0;
mv2_reduce_tuning_table *mv2_reduce_thresholds_table = NULL;


int mv2_reduce_intra_knomial_factor = 2;
int mv2_reduce_inter_knomial_factor = 2;

int (*MV2_Reduce_function)( void *sendbuf,
    void *recvbuf,
    int count,
    MPI_Datatype datatype,
    MPI_Op op,
    int root,
    MPI_Comm  comm_ptr)=NULL;

int (*MV2_Reduce_intra_function)( void *sendbuf,
    void *recvbuf,
    int count,
    MPI_Datatype datatype,
    MPI_Op op,
    int root,
    MPI_Comm  comm_ptr)=NULL;


#define MPIR_Reduce_inter_knomial_wrapper_MV2 smpi_coll_tuned_reduce_mvapich2_knomial
#define MPIR_Reduce_intra_knomial_wrapper_MV2 smpi_coll_tuned_reduce_mvapich2_knomial
#define MPIR_Reduce_binomial_MV2 smpi_coll_tuned_reduce_binomial
#define MPIR_Reduce_redscat_gather_MV2 smpi_coll_tuned_reduce_scatter_gather
#define MPIR_Reduce_shmem_MV2 smpi_coll_tuned_reduce_ompi_basic_linear



static void init_mv2_reduce_tables_stampede(){
  /*Stampede*/
  mv2_size_reduce_tuning_table = 8;
  mv2_reduce_thresholds_table = xbt_malloc(mv2_size_reduce_tuning_table *
      sizeof (mv2_reduce_tuning_table));
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
              {0, 65536, &MPIR_Reduce_shmem_MV2},
              {65536,-1,  &MPIR_Reduce_binomial_MV2},
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
              {262144,-1,  &MPIR_Reduce_binomial_MV2},
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
          {1, 0, 1, 1, 1,1},
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
  memcpy(mv2_reduce_thresholds_table, mv2_tmp_reduce_thresholds_table,
      mv2_size_reduce_tuning_table * sizeof (mv2_reduce_tuning_table));
}

/************ Reduce scatter variables and initializers                        */

typedef struct {
  int min;
  int max;
  int (*MV2_pt_Red_scat_function)(void *sendbuf,
      void *recvbuf,
      int *recvcnts,
      MPI_Datatype datatype,
      MPI_Op op,
      MPI_Comm comm_ptr);
} mv2_red_scat_tuning_element;

typedef struct {
  int numproc;
  int size_inter_table;
  mv2_red_scat_tuning_element inter_leader[MV2_MAX_NB_THRESHOLDS];
} mv2_red_scat_tuning_table;

int mv2_size_red_scat_tuning_table = 0;
mv2_red_scat_tuning_table *mv2_red_scat_thresholds_table = NULL;


int (*MV2_Red_scat_function)(void *sendbuf,
    void *recvbuf,
    int *recvcnts,
    MPI_Datatype datatype,
    MPI_Op op,
    MPI_Comm comm_ptr);



static  int MPIR_Reduce_Scatter_Basic_MV2(void *sendbuf,
    void *recvbuf,
    int *recvcnts,
    MPI_Datatype datatype,
    MPI_Op op,
    MPI_Comm comm)
{
  smpi_mpi_reduce_scatter(sendbuf,recvbuf,recvcnts,datatype,op,comm);
  return MPI_SUCCESS;
}
#define MPIR_Reduce_scatter_non_comm_MV2 smpi_coll_tuned_reduce_scatter_mpich_noncomm
#define MPIR_Reduce_scatter_Rec_Halving_MV2 smpi_coll_tuned_reduce_scatter_ompi_basic_recursivehalving
#define MPIR_Reduce_scatter_Pair_Wise_MV2 smpi_coll_tuned_reduce_scatter_mpich_pair




static void init_mv2_reduce_scatter_tables_stampede(){
  mv2_size_red_scat_tuning_table = 6;
  mv2_red_scat_thresholds_table = xbt_malloc(mv2_size_red_scat_tuning_table *
      sizeof (mv2_red_scat_tuning_table));
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
              {0, 128, &MPIR_Reduce_Scatter_Basic_MV2},
              {128, -1, &MPIR_Reduce_scatter_Rec_Halving_MV2},
          },
      },
      {
          256,
          2,
          {
              {0, 128, &MPIR_Reduce_Scatter_Basic_MV2},
              {128, -1, &MPIR_Reduce_scatter_Rec_Halving_MV2},
          },
      },
      {
          512,
          2,
          {
              {0, 256, &MPIR_Reduce_Scatter_Basic_MV2},
              {256, -1, &MPIR_Reduce_scatter_Rec_Halving_MV2},
          },
      },

  };
  memcpy(mv2_red_scat_thresholds_table, mv2_tmp_red_scat_thresholds_table,
      mv2_size_red_scat_tuning_table * sizeof (mv2_red_scat_tuning_table));
}

/************ Scatter variables and initializers                        */

typedef struct {
  int min;
  int max;
  int (*MV2_pt_Scatter_function)(void *sendbuf,
      int sendcnt,
      MPI_Datatype sendtype,
      void *recvbuf,
      int recvcnt,
      MPI_Datatype recvtype,
      int root, MPI_Comm comm);
} mv2_scatter_tuning_element;

typedef struct {
  int numproc;
  int size_inter_table;
  mv2_scatter_tuning_element inter_leader[MV2_MAX_NB_THRESHOLDS];
  int size_intra_table;
  mv2_scatter_tuning_element intra_node[MV2_MAX_NB_THRESHOLDS];
} mv2_scatter_tuning_table;


int *mv2_scatter_table_ppn_conf = NULL;
int mv2_scatter_num_ppn_conf = 1;
int *mv2_size_scatter_tuning_table = NULL;
mv2_scatter_tuning_table **mv2_scatter_thresholds_table = NULL;

int (*MV2_Scatter_function) (void *sendbuf, int sendcount, MPI_Datatype sendtype,
    void *recvbuf, int recvcount, MPI_Datatype recvtype,
    int root, MPI_Comm comm)=NULL;

int (*MV2_Scatter_intra_function) (void *sendbuf, int sendcount, MPI_Datatype sendtype,
    void *recvbuf, int recvcount, MPI_Datatype recvtype,
    int root, MPI_Comm comm)=NULL;
int MPIR_Scatter_mcst_wrap_MV2(void *sendbuf,
    int sendcnt,
    MPI_Datatype sendtype,
    void *recvbuf,
    int recvcnt,
    MPI_Datatype recvtype,
    int root, MPI_Comm comm_ptr);

int MPIR_Scatter_mcst_wrap_MV2(void *sendbuf,
    int sendcnt,
    MPI_Datatype sendtype,
    void *recvbuf,
    int recvcnt,
    MPI_Datatype recvtype,
    int root, MPI_Comm comm_ptr)
{
  return 0;
}

#define MPIR_Scatter_MV2_Binomial smpi_coll_tuned_scatter_ompi_binomial
#define MPIR_Scatter_MV2_Direct smpi_coll_tuned_scatter_ompi_basic_linear
#define MPIR_Scatter_MV2_two_level_Binomial smpi_coll_tuned_scatter_ompi_binomial
#define MPIR_Scatter_MV2_two_level_Direct smpi_coll_tuned_scatter_ompi_basic_linear




static void init_mv2_scatter_tables_stampede(){
  {
    int agg_table_sum = 0;
    int i;
    mv2_scatter_tuning_table **table_ptrs = NULL;
    mv2_scatter_num_ppn_conf = 3;
    mv2_scatter_thresholds_table
    = xbt_malloc(sizeof(mv2_scatter_tuning_table *)
        * mv2_scatter_num_ppn_conf);
    table_ptrs = xbt_malloc(sizeof(mv2_scatter_tuning_table *)
        * mv2_scatter_num_ppn_conf);
    mv2_size_scatter_tuning_table = xbt_malloc(sizeof(int) *
        mv2_scatter_num_ppn_conf);
    mv2_scatter_table_ppn_conf
    = xbt_malloc(mv2_scatter_num_ppn_conf * sizeof(int));
    mv2_scatter_table_ppn_conf[0] = 1;
    mv2_size_scatter_tuning_table[0] = 6;
    mv2_scatter_tuning_table mv2_tmp_scatter_thresholds_table_1ppn[] = {
        {2,
            1,
            {
                {0, -1, &MPIR_Scatter_MV2_Binomial},
            },
            1,
            {
                {0, -1, &MPIR_Scatter_MV2_Binomial},
            },
        },

        {4,
            1,
            {
                {0, -1, &MPIR_Scatter_MV2_Direct},
            },
            1,
            {
                {0, -1, &MPIR_Scatter_MV2_Direct},
            },
        },

        {8,
            1,
            {
                {0, -1, &MPIR_Scatter_MV2_Direct},
            },
            1,
            {
                {0, -1, &MPIR_Scatter_MV2_Direct},
            },
        },

        {16,
            1,
            {
                {0, -1, &MPIR_Scatter_MV2_Direct},
            },
            1,
            {
                {0, -1, &MPIR_Scatter_MV2_Direct},
            },
        },

        {32,
            1,
            {
                {0, -1, &MPIR_Scatter_MV2_Direct},
            },
            1,
            {
                {0, -1, &MPIR_Scatter_MV2_Direct},
            },
        },

        {64,
            2,
            {
                {0, 32, &MPIR_Scatter_MV2_Binomial},
                {32, -1, &MPIR_Scatter_MV2_Direct},
            },
            1,
            {
                {0, -1, &MPIR_Scatter_MV2_Binomial},
            },
        },
    };
    table_ptrs[0] = mv2_tmp_scatter_thresholds_table_1ppn;
    mv2_scatter_table_ppn_conf[1] = 2;
    mv2_size_scatter_tuning_table[1] = 6;
    mv2_scatter_tuning_table mv2_tmp_scatter_thresholds_table_2ppn[] = {
        {4,
            2,
            {
                {0, 4096, &MPIR_Scatter_MV2_Binomial},
                {4096, -1, &MPIR_Scatter_MV2_Direct},
            },
            1,
            {
                {0, -1, &MPIR_Scatter_MV2_Direct},
            },
        },

        {8,
            2,
            {
                {0, 512, &MPIR_Scatter_MV2_two_level_Direct},
                {512, -1, &MPIR_Scatter_MV2_Direct},
            },
            1,
            {
                {0, -1, &MPIR_Scatter_MV2_Binomial},
            },
        },

        {16,
            2,
            {
                {0, 2048, &MPIR_Scatter_MV2_two_level_Direct},
                {2048, -1, &MPIR_Scatter_MV2_Direct},
            },
            1,
            {
                {0, -1, &MPIR_Scatter_MV2_Binomial},
            },
        },

        {32,
            2,
            {
                {0, 2048, &MPIR_Scatter_MV2_two_level_Direct},
                {2048, -1, &MPIR_Scatter_MV2_Direct},
            },
            1,
            {
                {0, -1, &MPIR_Scatter_MV2_Binomial},
            },
        },

        {64,
            2,
            {
                {0, 8192, &MPIR_Scatter_MV2_two_level_Direct},
                {8192, -1, &MPIR_Scatter_MV2_Direct},
            },
            1,
            {
                {0, -1, &MPIR_Scatter_MV2_Binomial},
            },
        },

        {128,
            4,
            {
                {0, 16, &MPIR_Scatter_MV2_Binomial},
                {16, 128, &MPIR_Scatter_MV2_two_level_Binomial},
                {128, 16384, &MPIR_Scatter_MV2_two_level_Direct},
                {16384, -1, &MPIR_Scatter_MV2_Direct},
            },
            1,
            {
                {0, 128, &MPIR_Scatter_MV2_Direct},
                {128, -1, &MPIR_Scatter_MV2_Binomial},
            },
        },
    };
    table_ptrs[1] = mv2_tmp_scatter_thresholds_table_2ppn;
    mv2_scatter_table_ppn_conf[2] = 16;
    mv2_size_scatter_tuning_table[2] = 8;
    mv2_scatter_tuning_table mv2_tmp_scatter_thresholds_table_16ppn[] = {
        {
            16,
            2,
            {
                {0, 256, &MPIR_Scatter_MV2_Binomial},
                {256, -1, &MPIR_Scatter_MV2_Direct},
            },
            1,
            {
                { 0, -1, &MPIR_Scatter_MV2_Direct},
            },
        },

        {
            32,
            2,
            {
                {0, 512, &MPIR_Scatter_MV2_Binomial},
                {512, -1, &MPIR_Scatter_MV2_Direct},
            },
            1,
            {
                { 0, -1, &MPIR_Scatter_MV2_Direct},
            },
        },

        {
            64,
            2,
            {
                {0, 1024, &MPIR_Scatter_MV2_two_level_Direct},
                {1024, -1, &MPIR_Scatter_MV2_Direct},
            },
            1,
            {
                { 0, -1, &MPIR_Scatter_MV2_Direct},
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
                { 0, -1, &MPIR_Scatter_MV2_Direct},
            },
        },

        {
            256,
            4,
            {
                {0, 16, &MPIR_Scatter_mcst_wrap_MV2},
                {0, 16, &MPIR_Scatter_MV2_two_level_Direct},
                {16, 2048, &MPIR_Scatter_MV2_two_level_Direct},
                {2048, -1,  &MPIR_Scatter_MV2_Direct},
            },
            1,
            {
                { 0, -1, &MPIR_Scatter_MV2_Direct},
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
                { 0, -1, &MPIR_Scatter_MV2_Binomial},
            },
        },
        {
            1024,
            5,
            {
                {0, 16, &MPIR_Scatter_mcst_wrap_MV2},
                {0, 16,  &MPIR_Scatter_MV2_Binomial},
                {16, 32, &MPIR_Scatter_MV2_Binomial},
                {32, 4096, &MPIR_Scatter_MV2_two_level_Direct},
                {4096, -1, &MPIR_Scatter_MV2_Direct},
            },
            1,
            {
                { 0, -1, &MPIR_Scatter_MV2_Binomial},
            },
        },
        {
            2048,
            7,
            {
                {0, 16, &MPIR_Scatter_mcst_wrap_MV2},
                {0, 16,  &MPIR_Scatter_MV2_two_level_Binomial},
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
    for (i = 0; i < mv2_scatter_num_ppn_conf; i++) {
        agg_table_sum += mv2_size_scatter_tuning_table[i];
    }
    mv2_scatter_thresholds_table[0] =
        xbt_malloc(agg_table_sum * sizeof (mv2_scatter_tuning_table));
    memcpy(mv2_scatter_thresholds_table[0], table_ptrs[0],
        (sizeof(mv2_scatter_tuning_table)
            * mv2_size_scatter_tuning_table[0]));
    for (i = 1; i < mv2_scatter_num_ppn_conf; i++) {
        mv2_scatter_thresholds_table[i] =
            mv2_scatter_thresholds_table[i - 1]
                                         + mv2_size_scatter_tuning_table[i - 1];
        memcpy(mv2_scatter_thresholds_table[i], table_ptrs[i],
            (sizeof(mv2_scatter_tuning_table)
                * mv2_size_scatter_tuning_table[i]));
    }
    xbt_free(table_ptrs);
  }
}

