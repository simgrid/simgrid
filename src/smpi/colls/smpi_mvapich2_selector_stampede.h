/* selector for collective algorithms based on mvapich decision logic */

/* Copyright (c) 2009-2010, 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This is the tuning used by MVAPICH for Stampede platform based on (MV2_ARCH_INTEL_XEON_E5_2680_16, MV2_HCA_MLX_CX_FDR) */

/* Indicates number of processes per node */
extern int *mv2_alltoall_table_ppn_conf;
/* Indicates total number of configurations */
extern int mv2_alltoall_num_ppn_conf;
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

extern int *mv2_size_alltoall_tuning_table;
extern mv2_alltoall_tuning_table **mv2_alltoall_thresholds_table;
extern int mv2_use_old_alltoall;


int (*MV2_Alltoall_function) (void *sendbuf, int sendcount, MPI_Datatype sendtype,
                              void *recvbuf, int recvcount, MPI_Datatype recvtype,
                              MPI_Comm comm_ptr)=NULL;
                              
                              
int *mv2_alltoall_table_ppn_conf = NULL;
int mv2_alltoall_num_ppn_conf = 1;
int *mv2_size_alltoall_tuning_table = NULL;
mv2_alltoall_tuning_table **mv2_alltoall_thresholds_table = NULL;




#define MPIR_Alltoall_bruck_MV2 smpi_coll_tuned_alltoall_bruck
#define MPIR_Alltoall_RD_MV2 smpi_coll_tuned_alltoall_rdb
#define MPIR_Alltoall_Scatter_dest_MV2 smpi_coll_tuned_alltoall_ring
#define MPIR_Alltoall_pairwise_MV2 smpi_coll_tuned_alltoall_pair
#define MPIR_Alltoall_inplace_MV2 smpi_coll_tuned_alltoall_ring 



/* Indicates number of processes per node */
extern int *mv2_allgather_table_ppn_conf;
/* Indicates total number of configurations */
extern int mv2_allgather_num_ppn_conf;

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

extern int *mv2_size_allgather_tuning_table;
extern mv2_allgather_tuning_table **mv2_allgather_thresholds_table;
extern int mv2_use_old_allgather;

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

#define MPIR_Allgather_Bruck_MV2 smpi_coll_tuned_allgather_bruck
#define MPIR_Allgather_RD_MV2 smpi_coll_tuned_allgather_rdb
#define MPIR_Allgather_RD_Allgather_Comm_MV2 smpi_coll_tuned_allgather_rdb
#define MPIR_Allgather_Ring_MV2 smpi_coll_tuned_allgather_ring


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

extern int mv2_size_gather_tuning_table;
extern mv2_gather_tuning_table * mv2_gather_thresholds_table;

extern int mv2_user_gather_switch_point;
extern int mv2_use_two_level_gather;
extern int mv2_gather_direct_system_size_small;
extern int mv2_gather_direct_system_size_medium;
extern int mv2_use_direct_gather;

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
#define MPIR_Gather_MV2_two_level_Direct smpi_coll_tuned_gather_ompi_basic_linear
#define MPIR_Gather_intra smpi_coll_tuned_gather_mpich
