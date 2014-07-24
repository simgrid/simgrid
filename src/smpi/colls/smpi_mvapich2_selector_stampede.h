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

extern int mv2_size_allgatherv_tuning_table;
extern mv2_allgatherv_tuning_table *mv2_allgatherv_thresholds_table;

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

extern int mv2_size_allreduce_tuning_table;
extern mv2_allreduce_tuning_table *mv2_allreduce_thresholds_table;
extern int mv2_use_old_allreduce;


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
#define MPIR_Allreduce_pt2pt_rs_MV2 smpi_coll_tuned_allreduce_rab1



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

extern int mv2_use_pipelined_bcast;
extern int mv2_pipelined_knomial_factor; 
extern int mv2_pipelined_zcpy_knomial_factor; 
extern int zcpy_knomial_factor;
extern int bcast_segment_size;

extern int mv2_size_bcast_tuning_table;
extern mv2_bcast_tuning_table *mv2_bcast_thresholds_table;
extern int mv2_use_old_bcast;

int mv2_size_bcast_tuning_table = 0;
mv2_bcast_tuning_table *mv2_bcast_thresholds_table = NULL;


int (*MV2_Bcast_function) (void *buffer, int count, MPI_Datatype datatype,
                           int root, MPI_Comm comm_ptr) = NULL;

int (*MV2_Bcast_intra_node_function) (void *buffer, int count, MPI_Datatype datatype,
                                      int root, MPI_Comm comm_ptr) = NULL;
                                      
                                      
*/

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

extern int mv2_size_reduce_tuning_table;
extern mv2_reduce_tuning_table *mv2_reduce_thresholds_table;
extern int mv2_use_old_reduce;

int mv2_size_reduce_tuning_table = 0;
mv2_reduce_tuning_table *mv2_reduce_thresholds_table = NULL;


int mv2_reduce_intra_knomial_factor = -1;
int mv2_reduce_inter_knomial_factor = -1;

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
                                 
                                 
#define MPIR_Reduce_inter_knomial_wrapper_MV2 smpi_coll_tuned_reduce_ompi_binomial
#define MPIR_Reduce_intra_knomial_wrapper_MV2 smpi_coll_tuned_reduce_ompi_binomial
#define MPIR_Reduce_binomial_MV2 smpi_coll_tuned_reduce_ompi_binomial
#define MPIR_Reduce_redscat_gather_MV2 smpi_coll_tuned_reduce_scatter_gather
#define MPIR_Reduce_shmem_MV2 smpi_coll_tuned_reduce_ompi_basic_linear

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

extern int mv2_size_red_scat_tuning_table;
extern mv2_red_scat_tuning_table *mv2_red_scat_thresholds_table;

int mv2_size_red_scat_tuning_table = 0;
mv2_red_scat_tuning_table *mv2_red_scat_thresholds_table = NULL;


int (*MV2_Red_scat_function)(void *sendbuf,
                             void *recvbuf,
                             int *recvcnts,
                             MPI_Datatype datatype,
                             MPI_Op op,
                             MPI_Comm comm_ptr);

#define MPIR_Reduce_Scatter_Basic_MV2 smpi_coll_tuned_reduce_scatter_mpich_noncomm
#define MPIR_Reduce_scatter_non_comm_MV2 smpi_coll_tuned_reduce_scatter_mpich_noncomm
#define MPIR_Reduce_scatter_Rec_Halving_MV2 smpi_coll_tuned_reduce_scatter_ompi_basic_recursivehalving
#define MPIR_Reduce_scatter_Pair_Wise_MV2 smpi_coll_tuned_reduce_scatter_mpich_pair



/* Indicates number of processes per node */
extern int *mv2_scatter_table_ppn_conf;
/* Indicates total number of configurations */
extern int mv2_scatter_num_ppn_conf;

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

extern int *mv2_size_scatter_tuning_table;
extern mv2_scatter_tuning_table **mv2_scatter_thresholds_table;


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

