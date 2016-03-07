/* Copyright (c) 2007, 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_PRIVATE_H
#define SMPI_PRIVATE_H

#include "src/internal_config.h"
#include "xbt.h"
#include "xbt/base.h"
#include "xbt/xbt_os_time.h"
#include "xbt/synchro_core.h"
#include "simgrid/simix.h"
#include "src/include/smpi/smpi_interface.h"
#include "smpi/smpi.h"
#include "src/instr/instr_private.h"

SG_BEGIN_DECL()

struct s_smpi_process_data;
typedef struct s_smpi_process_data *smpi_process_data_t;

#define PERSISTENT     0x1
#define NON_PERSISTENT 0x2
#define SEND           0x4
#define RECV           0x8
#define RECV_DELETE    0x10
#define ISEND          0x20
#define SSEND          0x40
#define PREPARED       0x80
#define FINISHED       0x100
#define RMA            0x200
#define ACCUMULATE     0x400

enum smpi_process_state{
  SMPI_UNINITIALIZED,
  SMPI_INITIALIZED,
  SMPI_FINALIZED
};

// this struct is here to handle the problem of non-contignous data
// for each such structure these function should be implemented (vector
// index hvector hindex struct)
typedef struct s_smpi_subtype{
  void (*serialize)(const void * input, void *output, int count, void* subtype);
  void (*unserialize)(const void * input, void *output, int count, void* subtype, MPI_Op op);
  void (*subtype_free)(MPI_Datatype* type);
} s_smpi_subtype_t;

typedef struct s_smpi_mpi_datatype{
  char* name;
  size_t size;
  /* this let us know if a serialization is required*/
  size_t has_subtype;
  MPI_Aint lb;
  MPI_Aint ub;
  int flags;
  xbt_dict_t attributes;
  /* this let us know how to serialize and unserialize*/
  void *substruct;
  int in_use;
} s_smpi_mpi_datatype_t;

#define COLL_TAG_REDUCE -112
#define COLL_TAG_SCATTER -223
#define COLL_TAG_SCATTERV -334
#define COLL_TAG_GATHER -445
#define COLL_TAG_ALLGATHER -556
#define COLL_TAG_ALLGATHERV -667
#define COLL_TAG_BARRIER -778
#define COLL_TAG_REDUCE_SCATTER -889
#define COLL_TAG_ALLTOALLV -1000
#define COLL_TAG_ALLTOALL -1112
#define COLL_TAG_GATHERV -2223
#define COLL_TAG_BCAST -3334
#define COLL_TAG_ALLREDUCE -4445

#define MPI_COMM_UNINITIALIZED ((MPI_Comm)-1)

typedef struct s_smpi_mpi_request {
  void *buf;
  /* in the case of non-contiguous memory the user address should be keep
   * to unserialize the data inside the user memory*/
  void *old_buf;
  /* this let us know how to unserialize at the end of
   * the communication*/
  MPI_Datatype old_type;
  size_t size;
  int src;
  int dst;
  int tag;
  //to handle cases where we have an unknown sender
  //We can't override src, tag, and size, because the request may be reused later
  int real_src;
  int real_tag;
  int truncated;
  size_t real_size;
  MPI_Comm comm;
  smx_synchro_t action;
  unsigned flags;
  int detached;
  MPI_Request detached_sender;
  int refcount;
  MPI_Op op;
  int send;
  int recv;
} s_smpi_mpi_request_t;

typedef struct s_smpi_mpi_comm_key_elem {
  MPI_Comm_copy_attr_function* copy_fn;
  MPI_Comm_delete_attr_function* delete_fn;
} s_smpi_mpi_comm_key_elem_t; 
typedef struct s_smpi_mpi_comm_key_elem *smpi_comm_key_elem;

typedef struct s_smpi_mpi_type_key_elem {
  MPI_Type_copy_attr_function* copy_fn;
  MPI_Type_delete_attr_function* delete_fn;
} s_smpi_mpi_type_key_elem_t; 
typedef struct s_smpi_mpi_type_key_elem *smpi_type_key_elem;

typedef struct s_smpi_mpi_info {
  xbt_dict_t info_dict;
  int refcount;
} s_smpi_mpi_info_t; 

XBT_PRIVATE void smpi_process_destroy(void);
XBT_PRIVATE void smpi_process_finalize(void);
XBT_PRIVATE int smpi_process_finalized(void);
XBT_PRIVATE int smpi_process_initialized(void);
XBT_PRIVATE void smpi_process_mark_as_initialized(void);

struct s_smpi_mpi_cart_topology;
typedef struct s_smpi_mpi_cart_topology *MPIR_Cart_Topology;

struct s_smpi_mpi_graph_topology;
typedef struct s_smpi_mpi_graph_topology *MPIR_Graph_Topology;

struct s_smpi_dist_graph_topology;
typedef struct s_smpi_dist_graph_topology *MPIR_Dist_Graph_Topology;

// MPI_Topology defined in smpi.h, as it is public

XBT_PRIVATE void smpi_topo_destroy(MPI_Topology topo);
XBT_PRIVATE MPI_Topology smpi_topo_create(MPIR_Topo_type kind);
XBT_PRIVATE void smpi_cart_topo_destroy(MPIR_Cart_Topology cart);
XBT_PRIVATE MPI_Topology smpi_cart_topo_create(int ndims);
XBT_PRIVATE int smpi_mpi_cart_create(MPI_Comm comm_old, int ndims, int dims[], int periods[], int reorder,
                                     MPI_Comm *comm_cart);
XBT_PRIVATE int smpi_mpi_cart_sub(MPI_Comm comm, const int remain_dims[], MPI_Comm *newcomm);
XBT_PRIVATE int smpi_mpi_cart_coords(MPI_Comm comm, int rank, int maxdims, int coords[]);
XBT_PRIVATE int smpi_mpi_cart_get(MPI_Comm comm, int maxdims, int* dims, int* periods, int* coords);
XBT_PRIVATE int smpi_mpi_cart_rank(MPI_Comm comm, int* coords, int* rank);
XBT_PRIVATE int smpi_mpi_cart_shift(MPI_Comm comm, int direction, int disp, int *rank_source, int *rank_dest);
XBT_PRIVATE int smpi_mpi_cartdim_get(MPI_Comm comm, int *ndims);
XBT_PRIVATE int smpi_mpi_dims_create(int nnodes, int ndims, int dims[]);

XBT_PRIVATE smpi_process_data_t smpi_process_data(void);
XBT_PRIVATE smpi_process_data_t smpi_process_remote_data(int index);
// smpi_process_[set/get]_user_data must be public
/* XBT_PRIVATE void smpi_process_set_user_data(void *); */
/* XBT_PRIVATE void* smpi_process_get_user_data(void); */
XBT_PRIVATE int smpi_process_count(void);
XBT_PRIVATE MPI_Comm smpi_process_comm_world(void);
XBT_PRIVATE MPI_Comm smpi_process_get_comm_intra(void);
XBT_PRIVATE void smpi_process_set_comm_intra(MPI_Comm comm);
XBT_PRIVATE smx_mailbox_t smpi_process_mailbox(void);
XBT_PRIVATE smx_mailbox_t smpi_process_remote_mailbox(int index);
XBT_PRIVATE smx_mailbox_t smpi_process_mailbox_small(void);
XBT_PRIVATE smx_mailbox_t smpi_process_remote_mailbox_small(int index);
XBT_PRIVATE xbt_mutex_t smpi_process_mailboxes_mutex(void);
XBT_PRIVATE xbt_mutex_t smpi_process_remote_mailboxes_mutex(int index);
XBT_PRIVATE xbt_os_timer_t smpi_process_timer(void);
XBT_PRIVATE void smpi_process_simulated_start(void);
XBT_PRIVATE double smpi_process_simulated_elapsed(void);
XBT_PRIVATE void smpi_process_set_sampling(int s);
XBT_PRIVATE int smpi_process_get_sampling(void);
XBT_PRIVATE void smpi_process_set_replaying(int s);
XBT_PRIVATE int smpi_process_get_replaying(void);

XBT_PRIVATE void smpi_deployment_register_process(const char* instance_id, int rank, int index, MPI_Comm**, xbt_bar_t*);
XBT_PRIVATE void smpi_deployment_cleanup_instances(void);

XBT_PRIVATE void smpi_comm_copy_buffer_callback(smx_synchro_t comm, void *buff, size_t buff_size);

XBT_PRIVATE void smpi_comm_null_copy_buffer_callback(smx_synchro_t comm, void *buff, size_t buff_size);

XBT_PRIVATE void print_request(const char *message, MPI_Request request);

XBT_PRIVATE int smpi_enabled(void);
XBT_PRIVATE void smpi_global_init(void);
XBT_PRIVATE void smpi_global_destroy(void);
XBT_PRIVATE double smpi_mpi_wtime(void);

XBT_PRIVATE int is_datatype_valid(MPI_Datatype datatype);

XBT_PRIVATE size_t smpi_datatype_size(MPI_Datatype datatype);
XBT_PRIVATE MPI_Aint smpi_datatype_lb(MPI_Datatype datatype);
XBT_PRIVATE MPI_Aint smpi_datatype_ub(MPI_Datatype datatype);
XBT_PRIVATE int smpi_datatype_dup(MPI_Datatype datatype, MPI_Datatype* new_t);
XBT_PRIVATE int smpi_datatype_extent(MPI_Datatype datatype, MPI_Aint * lb, MPI_Aint * extent);
XBT_PRIVATE MPI_Aint smpi_datatype_get_extent(MPI_Datatype datatype);
XBT_PRIVATE void smpi_datatype_get_name(MPI_Datatype datatype, char* name, int* length);
XBT_PRIVATE void smpi_datatype_set_name(MPI_Datatype datatype, char* name);
XBT_PRIVATE int smpi_datatype_copy(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount,
                       MPI_Datatype recvtype);
XBT_PRIVATE void smpi_datatype_use(MPI_Datatype type);
XBT_PRIVATE void smpi_datatype_unuse(MPI_Datatype type);

XBT_PRIVATE int smpi_datatype_contiguous(int count, MPI_Datatype old_type, MPI_Datatype* new_type, MPI_Aint lb);
XBT_PRIVATE int smpi_datatype_vector(int count, int blocklen, int stride, MPI_Datatype old_type,
                       MPI_Datatype* new_type);

XBT_PRIVATE int smpi_datatype_hvector(int count, int blocklen, MPI_Aint stride,MPI_Datatype old_type,
                       MPI_Datatype* new_type);
XBT_PRIVATE int smpi_datatype_indexed(int count, int* blocklens, int* indices, MPI_Datatype old_type,
                       MPI_Datatype* new_type);
XBT_PRIVATE int smpi_datatype_hindexed(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype old_type,
                       MPI_Datatype* new_type);
XBT_PRIVATE int smpi_datatype_struct(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype* old_types,
                       MPI_Datatype* new_type);

XBT_PRIVATE void smpi_datatype_create(MPI_Datatype* new_type, int size,int lb, int ub, int has_subtype,
                       void *struct_type, int flags);

XBT_PRIVATE void smpi_datatype_free(MPI_Datatype* type);
XBT_PRIVATE void smpi_datatype_commit(MPI_Datatype* datatype);

XBT_PRIVATE int smpi_mpi_unpack(void* inbuf, int insize, int* position, void* outbuf, int outcount, MPI_Datatype type,
                       MPI_Comm comm);
XBT_PRIVATE int smpi_mpi_pack(void* inbuf, int incount, MPI_Datatype type, void* outbuf, int outcount, int* position,
                       MPI_Comm comm);

XBT_PRIVATE void smpi_empty_status(MPI_Status * status);
XBT_PRIVATE MPI_Op smpi_op_new(MPI_User_function * function, int commute);
XBT_PRIVATE int smpi_op_is_commute(MPI_Op op);
XBT_PRIVATE void smpi_op_destroy(MPI_Op op);
XBT_PRIVATE void smpi_op_apply(MPI_Op op, void *invec, void *inoutvec, int *len, MPI_Datatype * datatype);

XBT_PRIVATE MPI_Group smpi_group_new(int size);
XBT_PRIVATE MPI_Group smpi_group_copy(MPI_Group origin);
XBT_PRIVATE void smpi_group_destroy(MPI_Group group);
XBT_PRIVATE void smpi_group_set_mapping(MPI_Group group, int index, int rank);
XBT_PRIVATE int smpi_group_index(MPI_Group group, int rank);
XBT_PRIVATE int smpi_group_rank(MPI_Group group, int index);
XBT_PRIVATE int smpi_group_use(MPI_Group group);
XBT_PRIVATE int smpi_group_unuse(MPI_Group group);
XBT_PRIVATE int smpi_group_size(MPI_Group group);
XBT_PRIVATE int smpi_group_compare(MPI_Group group1, MPI_Group group2);
XBT_PRIVATE int smpi_group_incl(MPI_Group group, int n, int* ranks, MPI_Group* newgroup);

XBT_PRIVATE MPI_Topology smpi_comm_topo(MPI_Comm comm);
XBT_PRIVATE MPI_Comm smpi_comm_new(MPI_Group group, MPI_Topology topo);
XBT_PRIVATE void smpi_comm_destroy(MPI_Comm comm);
XBT_PRIVATE MPI_Group smpi_comm_group(MPI_Comm comm);
XBT_PRIVATE int smpi_comm_size(MPI_Comm comm);
XBT_PRIVATE void smpi_comm_get_name(MPI_Comm comm, char* name, int* len);
XBT_PRIVATE int smpi_comm_rank(MPI_Comm comm);
XBT_PRIVATE MPI_Comm smpi_comm_split(MPI_Comm comm, int color, int key);
XBT_PRIVATE int smpi_comm_dup(MPI_Comm comm, MPI_Comm* newcomm);
XBT_PRIVATE void smpi_comm_use(MPI_Comm comm);
XBT_PRIVATE void smpi_comm_unuse(MPI_Comm comm);
XBT_PRIVATE void smpi_comm_set_leaders_comm(MPI_Comm comm, MPI_Comm leaders);
XBT_PRIVATE void smpi_comm_set_intra_comm(MPI_Comm comm, MPI_Comm leaders);
XBT_PRIVATE int* smpi_comm_get_non_uniform_map(MPI_Comm comm);
XBT_PRIVATE int* smpi_comm_get_leaders_map(MPI_Comm comm);
XBT_PRIVATE MPI_Comm smpi_comm_get_leaders_comm(MPI_Comm comm);
XBT_PRIVATE MPI_Comm smpi_comm_get_intra_comm(MPI_Comm comm);
XBT_PRIVATE int smpi_comm_is_uniform(MPI_Comm comm);
XBT_PRIVATE int smpi_comm_is_blocked(MPI_Comm comm);
XBT_PRIVATE void smpi_comm_init_smp(MPI_Comm comm);

XBT_PRIVATE int smpi_comm_c2f(MPI_Comm comm);
XBT_PRIVATE MPI_Comm smpi_comm_f2c(int comm);
XBT_PRIVATE int smpi_group_c2f(MPI_Group group);
XBT_PRIVATE MPI_Group smpi_group_f2c(int group);
XBT_PRIVATE int smpi_request_c2f(MPI_Request req);
XBT_PRIVATE MPI_Request smpi_request_f2c(int req);
XBT_PRIVATE int smpi_type_c2f(MPI_Datatype datatype);
XBT_PRIVATE MPI_Datatype smpi_type_f2c(int datatype);
XBT_PRIVATE int smpi_op_c2f(MPI_Op op);
XBT_PRIVATE MPI_Op smpi_op_f2c(int op);
XBT_PRIVATE int smpi_win_c2f(MPI_Win win);
XBT_PRIVATE MPI_Win smpi_win_f2c(int win);
XBT_PRIVATE int smpi_info_c2f(MPI_Info info);
XBT_PRIVATE MPI_Info smpi_info_f2c(int info);

XBT_PRIVATE MPI_Request smpi_mpi_send_init(void *buf, int count, MPI_Datatype datatype, int dst, int tag,
                                           MPI_Comm comm);
XBT_PRIVATE MPI_Request smpi_mpi_recv_init(void *buf, int count, MPI_Datatype datatype, int src, int tag,
                                           MPI_Comm comm);
XBT_PRIVATE MPI_Request smpi_mpi_ssend_init(void *buf, int count, MPI_Datatype datatype, int dst, int tag,
                                            MPI_Comm comm);
XBT_PRIVATE void smpi_mpi_start(MPI_Request request);
XBT_PRIVATE void smpi_mpi_startall(int count, MPI_Request * requests);
XBT_PRIVATE void smpi_mpi_request_free(MPI_Request * request);
XBT_PRIVATE MPI_Request smpi_isend_init(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm);
XBT_PRIVATE MPI_Request smpi_mpi_isend(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm);
XBT_PRIVATE MPI_Request smpi_issend_init(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm);
XBT_PRIVATE MPI_Request smpi_mpi_issend(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm);
XBT_PRIVATE MPI_Request smpi_irecv_init(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm);
XBT_PRIVATE MPI_Request smpi_mpi_irecv(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm);
XBT_PRIVATE MPI_Request smpi_rma_send_init(void *buf, int count, MPI_Datatype datatype, int src, int dst, int tag,
                                           MPI_Comm comm, MPI_Op op);
XBT_PRIVATE MPI_Request smpi_rma_recv_init(void *buf, int count, MPI_Datatype datatype, int src, int dst, int tag,
                                           MPI_Comm comm, MPI_Op op);
XBT_PRIVATE void smpi_mpi_recv(void *buf, int count, MPI_Datatype datatype, int src,int tag, MPI_Comm comm,
                               MPI_Status * status);
XBT_PRIVATE void smpi_mpi_send(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm);
XBT_PRIVATE void smpi_mpi_ssend(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm);
XBT_PRIVATE void smpi_mpi_sendrecv(void *sendbuf, int sendcount, MPI_Datatype sendtype, int dst, int sendtag,
                                   void *recvbuf, int recvcount, MPI_Datatype recvtype, int src, int recvtag,
                                   MPI_Comm comm, MPI_Status * status);
XBT_PRIVATE int smpi_mpi_test(MPI_Request * request, MPI_Status * status);
XBT_PRIVATE int smpi_mpi_testany(int count, MPI_Request requests[], int *index, MPI_Status * status);
XBT_PRIVATE int smpi_mpi_testall(int count, MPI_Request requests[], MPI_Status status[]);
XBT_PRIVATE void smpi_mpi_probe(int source, int tag, MPI_Comm comm, MPI_Status* status);
XBT_PRIVATE void smpi_mpi_iprobe(int source, int tag, MPI_Comm comm, int* flag, MPI_Status* status);
XBT_PRIVATE int smpi_mpi_get_count(MPI_Status * status, MPI_Datatype datatype);
XBT_PRIVATE void smpi_mpi_wait(MPI_Request * request, MPI_Status * status);
XBT_PRIVATE int smpi_mpi_waitany(int count, MPI_Request requests[], MPI_Status * status);
XBT_PRIVATE int smpi_mpi_waitall(int count, MPI_Request requests[], MPI_Status status[]);
XBT_PRIVATE int smpi_mpi_waitsome(int incount, MPI_Request requests[], int *indices, MPI_Status status[]);
XBT_PRIVATE int smpi_mpi_testsome(int incount, MPI_Request requests[], int *indices, MPI_Status status[]);
XBT_PRIVATE void smpi_mpi_bcast(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
XBT_PRIVATE void smpi_mpi_barrier(MPI_Comm comm);
XBT_PRIVATE void smpi_mpi_gather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                     void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
XBT_PRIVATE void smpi_mpi_reduce_scatter(void *sendbuf, void *recvbuf, int *recvcounts,
                       MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
XBT_PRIVATE void smpi_mpi_gatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                      void *recvbuf, int *recvcounts, int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm);
XBT_PRIVATE void smpi_mpi_allgather(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                        int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
XBT_PRIVATE void smpi_mpi_allgatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                         int *recvcounts, int *displs, MPI_Datatype recvtype, MPI_Comm comm);
XBT_PRIVATE void smpi_mpi_scatter(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                      void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
XBT_PRIVATE void smpi_mpi_scatterv(void *sendbuf, int *sendcounts, int *displs, MPI_Datatype sendtype,
                      void *recvbuf, int recvcount,MPI_Datatype recvtype, int root, MPI_Comm comm);
XBT_PRIVATE void smpi_mpi_reduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root,
                      MPI_Comm comm);
XBT_PRIVATE void smpi_mpi_allreduce(void *sendbuf, void *recvbuf, int count,MPI_Datatype datatype, MPI_Op op,
                      MPI_Comm comm);
XBT_PRIVATE void smpi_mpi_scan(void *sendbuf, void *recvbuf, int count,MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
XBT_PRIVATE void smpi_mpi_exscan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                      MPI_Comm comm);

XBT_PRIVATE int smpi_mpi_win_free( MPI_Win* win);

XBT_PRIVATE MPI_Win smpi_mpi_win_create( void *base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm);

XBT_PRIVATE void smpi_mpi_win_get_name(MPI_Win win, char* name, int* length);
XBT_PRIVATE void smpi_mpi_win_get_group(MPI_Win win, MPI_Group* group);
XBT_PRIVATE void smpi_mpi_win_set_name(MPI_Win win, char* name);

XBT_PRIVATE int smpi_mpi_win_fence( int assert,  MPI_Win win);

XBT_PRIVATE int smpi_mpi_win_post(MPI_Group group, int assert, MPI_Win win);
XBT_PRIVATE int smpi_mpi_win_start(MPI_Group group, int assert, MPI_Win win);
XBT_PRIVATE int smpi_mpi_win_complete(MPI_Win win);
XBT_PRIVATE int smpi_mpi_win_wait(MPI_Win win);

XBT_PRIVATE int smpi_mpi_get( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win);
XBT_PRIVATE int smpi_mpi_put( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win);
XBT_PRIVATE int smpi_mpi_accumulate( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win);

XBT_PRIVATE void nary_tree_bcast(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm, int arity);
XBT_PRIVATE void nary_tree_barrier(MPI_Comm comm, int arity);

XBT_PRIVATE int smpi_coll_tuned_alltoall_ompi2(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                                      int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
XBT_PRIVATE int smpi_coll_tuned_alltoall_bruck(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                                   int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
XBT_PRIVATE int smpi_coll_tuned_alltoall_basic_linear(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                          void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
XBT_PRIVATE int smpi_coll_basic_alltoallv(void *sendbuf, int *sendcounts, int *senddisps, MPI_Datatype sendtype,
                              void *recvbuf, int *recvcounts, int *recvdisps, MPI_Datatype recvtype, MPI_Comm comm);

XBT_PRIVATE int smpi_comm_keyval_create(MPI_Comm_copy_attr_function* copy_fn, MPI_Comm_delete_attr_function* delete_fn,
                                        int* keyval, void* extra_state);
XBT_PRIVATE int smpi_comm_keyval_free(int* keyval);
XBT_PRIVATE int smpi_comm_attr_get(MPI_Comm comm, int keyval, void* attr_value, int* flag);
XBT_PRIVATE int smpi_comm_attr_delete(MPI_Comm comm, int keyval);
XBT_PRIVATE int smpi_comm_attr_put(MPI_Comm comm, int keyval, void* attr_value);
XBT_PRIVATE int smpi_type_attr_delete(MPI_Datatype type, int keyval);
XBT_PRIVATE int smpi_type_attr_get(MPI_Datatype type, int keyval, void* attr_value, int* flag);
XBT_PRIVATE int smpi_type_attr_put(MPI_Datatype type, int keyval, void* attr_value);
XBT_PRIVATE int smpi_type_keyval_create(MPI_Type_copy_attr_function* copy_fn, MPI_Type_delete_attr_function* delete_fn,
                                        int* keyval, void* extra_state);
XBT_PRIVATE int smpi_type_keyval_free(int* keyval);
// utilities
extern XBT_PRIVATE double smpi_cpu_threshold;
extern XBT_PRIVATE double smpi_running_power;
extern XBT_PRIVATE int smpi_privatize_global_variables;
extern XBT_PRIVATE char* smpi_start_data_exe; //start of the data+bss segment of the executable
extern XBT_PRIVATE int smpi_size_data_exe; //size of the data+bss segment of the executable

XBT_PRIVATE void smpi_switch_data_segment(int dest);
XBT_PRIVATE void smpi_really_switch_data_segment(int dest);
XBT_PRIVATE int smpi_is_privatisation_file(char* file);

XBT_PRIVATE void smpi_get_executable_global_size(void);
XBT_PRIVATE void smpi_initialize_global_memory_segments(void);
XBT_PRIVATE void smpi_destroy_global_memory_segments(void);
XBT_PRIVATE void smpi_bench_destroy(void);
XBT_PRIVATE void smpi_bench_begin(void);
XBT_PRIVATE void smpi_bench_end(void);

XBT_PRIVATE void* smpi_get_tmp_sendbuffer(int size);
XBT_PRIVATE void* smpi_get_tmp_recvbuffer(int size);
XBT_PRIVATE void  smpi_free_tmp_buffer(void* buf);

XBT_PRIVATE int smpi_comm_attr_get(MPI_Comm comm, int keyval, void* attr_value, int* flag);
XBT_PRIVATE XBT_PRIVATE int smpi_comm_attr_delete(MPI_Comm comm, int keyval);
XBT_PRIVATE int smpi_comm_attr_put(MPI_Comm comm, int keyval, void* attr_value);

// f77 wrappers
void mpi_init_(int*);
void mpi_finalize_(int*);
void mpi_abort_(int* comm, int* errorcode, int* ierr);
void mpi_comm_rank_(int* comm, int* rank, int* ierr);
void mpi_comm_size_(int* comm, int* size, int* ierr);
double mpi_wtime_(void);
double mpi_wtick_(void);
void mpi_initialized_(int* flag, int* ierr);

void mpi_comm_dup_(int* comm, int* newcomm, int* ierr);
void mpi_comm_create_(int* comm, int* group, int* newcomm, int* ierr);
void mpi_comm_free_(int* comm, int* ierr);
void mpi_comm_split_(int* comm, int* color, int* key, int* comm_out, int* ierr);
void mpi_group_incl_(int* group, int* n, int* key, int* group_out, int* ierr) ;
void mpi_comm_group_(int* comm, int* group_out,  int* ierr);
void mpi_send_init_(void *buf, int* count, int* datatype, int* dst, int* tag, int* comm, int* request, int* ierr);
void mpi_isend_(void *buf, int* count, int* datatype, int* dst, int* tag, int* comm, int* request, int* ierr);
void mpi_irsend_(void *buf, int* count, int* datatype, int* dst, int* tag, int* comm, int* request, int* ierr);
void mpi_send_(void* buf, int* count, int* datatype, int* dst, int* tag, int* comm, int* ierr);
void mpi_rsend_(void* buf, int* count, int* datatype, int* dst, int* tag, int* comm, int* ierr);
void mpi_recv_init_(void *buf, int* count, int* datatype, int* src, int* tag, int* comm, int* request, int* ierr);
void mpi_irecv_(void *buf, int* count, int* datatype, int* src, int* tag, int* comm, int* request, int* ierr);
void mpi_recv_(void* buf, int* count, int* datatype, int* src, int* tag, int* comm, MPI_Status* status, int* ierr);
void mpi_start_(int* request, int* ierr);
void mpi_startall_(int* count, int* requests, int* ierr);
void mpi_wait_(int* request, MPI_Status* status, int* ierr);
void mpi_waitany_(int* count, int* requests, int* index, MPI_Status* status, int* ierr);
void mpi_waitall_(int* count, int* requests, MPI_Status* status, int* ierr);

void mpi_barrier_(int* comm, int* ierr);
void mpi_bcast_(void* buf, int* count, int* datatype, int* root, int* comm, int* ierr);
void mpi_reduce_(void* sendbuf, void* recvbuf, int* count, int* datatype, int* op, int* root, int* comm, int* ierr);
void mpi_allreduce_(void* sendbuf, void* recvbuf, int* count, int* datatype, int* op, int* comm, int* ierr);
void mpi_reduce_scatter_(void* sendbuf, void* recvbuf, int* recvcounts, int* datatype, int* op, int* comm, int* ierr) ;
void mpi_scatter_(void* sendbuf, int* sendcount, int* sendtype, void* recvbuf, int* recvcount, int* recvtype,
                   int* root, int* comm, int* ierr);
void mpi_scatterv_(void* sendbuf, int* sendcounts, int* displs, int* sendtype,
                   void* recvbuf, int* recvcount, int* recvtype, int* root, int* comm, int* ierr);
void mpi_gather_(void* sendbuf, int* sendcount, int* sendtype, void* recvbuf, int* recvcount, int* recvtype,
                  int* root, int* comm, int* ierr);
void mpi_gatherv_(void* sendbuf, int* sendcount, int* sendtype,
                  void* recvbuf, int* recvcounts, int* displs, int* recvtype, int* root, int* comm, int* ierr);
void mpi_allgather_(void* sendbuf, int* sendcount, int* sendtype,
                     void* recvbuf, int* recvcount, int* recvtype, int* comm, int* ierr);
void mpi_allgatherv_(void* sendbuf, int* sendcount, int* sendtype,
                     void* recvbuf, int* recvcount,int* displs, int* recvtype, int* comm, int* ierr) ;
void mpi_type_size_(int* datatype, int *size, int* ierr);

void mpi_scan_(void* sendbuf, void* recvbuf, int* count, int* datatype, int* op, int* comm, int* ierr);
void mpi_alltoall_(void* sendbuf, int* sendcount, int* sendtype,
                    void* recvbuf, int* recvcount, int* recvtype, int* comm, int* ierr);
void mpi_alltoallv_(void* sendbuf, int* sendcounts, int* senddisps, int* sendtype,
                    void* recvbuf, int* recvcounts, int* recvdisps, int* recvtype, int* comm, int* ierr);
void mpi_get_processor_name_(char *name, int *resultlen, int* ierr);
void mpi_test_ (int * request, int *flag, MPI_Status * status, int* ierr);
void mpi_testall_ (int* count, int * requests,  int *flag, MPI_Status * statuses, int* ierr);
void mpi_get_count_(MPI_Status * status, int* datatype, int *count, int* ierr);
void mpi_type_extent_(int* datatype, MPI_Aint * extent, int* ierr);
void mpi_attr_get_(int* comm, int* keyval, void* attr_value, int* flag, int* ierr );
void mpi_type_commit_(int* datatype,  int* ierr);
void mpi_type_vector_(int* count, int* blocklen, int* stride, int* old_type, int* newtype,  int* ierr);
void mpi_type_create_vector_(int* count, int* blocklen, int* stride, int* old_type, int* newtype,  int* ierr);
void mpi_type_hvector_(int* count, int* blocklen, MPI_Aint* stride, int* old_type, int* newtype,  int* ierr);
void mpi_type_create_hvector_(int* count, int* blocklen, MPI_Aint* stride, int* old_type, int* newtype,  int* ierr);
void mpi_type_free_(int* datatype, int* ierr);
void mpi_type_lb_(int* datatype, MPI_Aint * extent, int* ierr);
void mpi_type_ub_(int* datatype, MPI_Aint * extent, int* ierr);
void mpi_win_fence_( int* assert,  int* win, int* ierr);
void mpi_win_free_( int* win, int* ierr);
void mpi_win_create_( int *base, MPI_Aint* size, int* disp_unit, int* info, int* comm, int *win, int* ierr);
void mpi_win_set_name_ (int*  win, char * name, int* ierr, int size);
void mpi_win_get_name_ (int*  win, char * name, int* len, int* ierr);
void mpi_win_post_(int* group, int assert, int* win, int* ierr);
void mpi_win_start_(int* group, int assert, int* win, int* ierr);
void mpi_win_complete_(int* win, int* ierr);
void mpi_win_wait_(int* win, int* ierr);
void mpi_info_create_( int *info, int* ierr);
void mpi_info_set_( int *info, char *key, char *value, int* ierr, unsigned int keylen, unsigned int valuelen);
void mpi_info_free_(int* info, int* ierr);
void mpi_get_( int *origin_addr, int* origin_count, int* origin_datatype, int* target_rank,
    MPI_Aint* target_disp, int* target_count, int* target_datatype, int* win, int* ierr);
void mpi_put_( int *origin_addr, int* origin_count, int* origin_datatype, int* target_rank,
    MPI_Aint* target_disp, int* target_count, int* target_datatype, int* win, int* ierr);
void mpi_accumulate_( int *origin_addr, int* origin_count, int* origin_datatype, int* target_rank,
    MPI_Aint* target_disp, int* target_count, int* target_datatype, int* op, int* win, int* ierr);
void mpi_error_string_(int* errorcode, char* string, int* resultlen, int* ierr);
void mpi_sendrecv_(void* sendbuf, int* sendcount, int* sendtype, int* dst, int* sendtag, void *recvbuf, int* recvcount,
                int* recvtype, int* src, int* recvtag, int* comm, MPI_Status* status, int* ierr);

void mpi_finalized_ (int * flag, int* ierr);
void mpi_init_thread_ (int *required, int *provided, int* ierr);
void mpi_query_thread_ (int *provided, int* ierr);
void mpi_is_thread_main_ (int *flag, int* ierr);
void mpi_address_ (void *location, MPI_Aint * address, int* ierr);
void mpi_get_address_ (void *location, MPI_Aint * address, int* ierr);
void mpi_type_dup_ (int*  datatype, int* newdatatype, int* ierr);
void mpi_type_set_name_ (int*  datatype, char * name, int* ierr, int size);
void mpi_type_get_name_ (int*  datatype, char * name, int* len, int* ierr);
void mpi_type_get_attr_ (int* type, int* type_keyval, void *attribute_val, int* flag, int* ierr);
void mpi_type_set_attr_ (int* type, int* type_keyval, void *attribute_val, int* ierr);
void mpi_type_delete_attr_ (int* type, int* type_keyval, int* ierr);
void mpi_type_create_keyval_ (void* copy_fn, void* delete_fn, int* keyval, void* extra_state, int* ierr);
void mpi_type_free_keyval_ (int* keyval, int* ierr) ;
void mpi_pcontrol_ (int* level , int* ierr);
void mpi_type_get_extent_ (int* datatype, MPI_Aint * lb, MPI_Aint * extent, int* ierr);
void mpi_type_get_true_extent_ (int* datatype, MPI_Aint * lb, MPI_Aint * extent, int* ierr);
void mpi_op_create_ (void * function, int* commute, int* op, int* ierr);
void mpi_op_free_ (int* op, int* ierr);
void mpi_group_free_ (int* group, int* ierr);
void mpi_group_size_ (int* group, int *size, int* ierr);
void mpi_group_rank_ (int* group, int *rank, int* ierr);
void mpi_group_translate_ranks_ (int* group1, int* n, int *ranks1, int* group2, int *ranks2, int* ierr);
void mpi_group_compare_ (int* group1, int* group2, int *result, int* ierr);
void mpi_group_union_ (int* group1, int* group2, int* newgroup, int* ierr);
void mpi_group_intersection_ (int* group1, int* group2, int* newgroup, int* ierr);
void mpi_group_difference_ (int* group1, int* group2, int* newgroup, int* ierr);
void mpi_group_excl_ (int* group, int* n, int *ranks, int* newgroup, int* ierr);
void mpi_group_range_incl_ (int* group, int* n, int ranges[][3], int* newgroup, int* ierr);
void mpi_group_range_excl_ (int* group, int* n, int ranges[][3], int* newgroup, int* ierr);
void mpi_comm_get_attr_ (int* comm, int* comm_keyval, void *attribute_val, int *flag, int* ierr);
void mpi_comm_set_attr_ (int* comm, int* comm_keyval, void *attribute_val, int* ierr);
void mpi_comm_delete_attr_ (int* comm, int* comm_keyval, int* ierr);
void mpi_comm_create_keyval_ (void* copy_fn, void* delete_fn, int* keyval, void* extra_state, int* ierr);
void mpi_comm_free_keyval_ (int* keyval, int* ierr) ;
void mpi_comm_get_name_ (int* comm, char* name, int* len, int* ierr);
void mpi_comm_compare_ (int* comm1, int* comm2, int *result, int* ierr);
void mpi_comm_disconnect_ (int* comm, int* ierr);
void mpi_request_free_ (int* request, int* ierr);
void mpi_sendrecv_replace_ (void *buf, int* count, int* datatype, int* dst, int* sendtag, int* src, int* recvtag,
 int* comm, MPI_Status* status, int* ierr);
void mpi_testany_ (int* count, int* requests, int *index, int *flag, MPI_Status* status, int* ierr);
void mpi_waitsome_ (int* incount, int* requests, int *outcount, int *indices, MPI_Status* status, int* ierr);
void mpi_reduce_local_ (void *inbuf, void *inoutbuf, int* count, int* datatype, int* op, int* ierr);
void mpi_reduce_scatter_block_ (void *sendbuf, void *recvbuf, int* recvcount, int* datatype, int* op, int* comm,
                                int* ierr);
void mpi_pack_size_ (int* incount, int* datatype, int* comm, int* size, int* ierr) ;
void mpi_cart_coords_ (int* comm, int* rank, int* maxdims, int* coords, int* ierr) ;
void mpi_cart_create_ (int* comm_old, int* ndims, int* dims, int* periods, int* reorder, int*  comm_cart, int* ierr) ;
void mpi_cart_get_ (int* comm, int* maxdims, int* dims, int* periods, int* coords, int* ierr) ;
void mpi_cart_map_ (int* comm_old, int* ndims, int* dims, int* periods, int* newrank, int* ierr) ;
void mpi_cart_rank_ (int* comm, int* coords, int* rank, int* ierr) ;
void mpi_cart_shift_ (int* comm, int* direction, int* displ, int* source, int* dest, int* ierr) ;
void mpi_cart_sub_ (int* comm, int* remain_dims, int*  comm_new, int* ierr) ;
void mpi_cartdim_get_ (int* comm, int* ndims, int* ierr) ;
void mpi_graph_create_ (int* comm_old, int* nnodes, int* index, int* edges, int* reorder, int*  comm_graph, int* ierr) ;
void mpi_graph_get_ (int* comm, int* maxindex, int* maxedges, int* index, int* edges, int* ierr) ;
void mpi_graph_map_ (int* comm_old, int* nnodes, int* index, int* edges, int* newrank, int* ierr) ;
void mpi_graph_neighbors_ (int* comm, int* rank, int* maxneighbors, int* neighbors, int* ierr) ;
void mpi_graph_neighbors_count_ (int* comm, int* rank, int* nneighbors, int* ierr) ;
void mpi_graphdims_get_ (int* comm, int* nnodes, int* nedges, int* ierr) ;
void mpi_topo_test_ (int* comm, int* top_type, int* ierr) ;
void mpi_error_class_ (int* errorcode, int* errorclass, int* ierr) ;
void mpi_errhandler_create_ (void* function, void* errhandler, int* ierr) ;
void mpi_errhandler_free_ (void* errhandler, int* ierr) ;
void mpi_errhandler_get_ (int* comm, void* errhandler, int* ierr) ;
void mpi_errhandler_set_ (int* comm, void* errhandler, int* ierr) ;
void mpi_comm_set_errhandler_ (int* comm, void* errhandler, int* ierr) ;
void mpi_comm_get_errhandler_ (int* comm, void* errhandler, int* ierr) ;
void mpi_type_contiguous_ (int* count, int* old_type, int*  newtype, int* ierr) ;
void mpi_cancel_ (int*  request, int* ierr) ;
void mpi_buffer_attach_ (void* buffer, int* size, int* ierr) ;
void mpi_buffer_detach_ (void* buffer, int* size, int* ierr) ;
void mpi_testsome_ (int* incount, int*  requests, int* outcount, int* indices, MPI_Status*  statuses, int* ierr) ;
void mpi_comm_test_inter_ (int* comm, int* flag, int* ierr) ;
void mpi_unpack_ (void* inbuf, int* insize, int* position, void* outbuf, int* outcount, int* type, int* comm,
                  int* ierr) ;
void mpi_pack_external_size_ (char *datarep, int* incount, int* datatype, MPI_Aint *size, int* ierr);
void mpi_pack_external_ (char *datarep, void *inbuf, int* incount, int* datatype, void *outbuf, MPI_Aint* outcount,
                         MPI_Aint *position, int* ierr);
void mpi_unpack_external_ (char *datarep, void *inbuf, MPI_Aint* insize, MPI_Aint *position, void *outbuf,
                           int* outcount, int* datatype, int* ierr);
void mpi_type_hindexed_ (int* count, int* blocklens, MPI_Aint* indices, int* old_type, int*  newtype, int* ierr) ;
void mpi_type_create_hindexed_ (int* count, int* blocklens, MPI_Aint* indices, int* old_type, int*  newtype, int* ierr);
void mpi_type_create_hindexed_block_ (int* count, int* blocklength, MPI_Aint* indices, int* old_type, int*  newtype,
                                      int* ierr) ;
void mpi_type_indexed_ (int* count, int* blocklens, int* indices, int* old_type, int*  newtype, int* ierr) ;
void mpi_type_create_indexed_block_ (int* count, int* blocklength, int* indices,  int* old_type,  int*newtype,
                                     int* ierr);
void mpi_type_struct_ (int* count, int* blocklens, MPI_Aint* indices, int*  old_types, int*  newtype, int* ierr) ;
void mpi_type_create_struct_ (int* count, int* blocklens, MPI_Aint* indices, int*  old_types, int*  newtype, int* ierr);
void mpi_ssend_ (void* buf, int* count, int* datatype, int* dest, int* tag, int* comm, int* ierr) ;
void mpi_ssend_init_ (void* buf, int* count, int* datatype, int* dest, int* tag, int* comm, int*  request, int* ierr) ;
void mpi_intercomm_create_ (int* local_comm, int* local_leader, int* peer_comm, int* remote_leader, int* tag,
                            int* comm_out, int* ierr) ;
void mpi_intercomm_merge_ (int* comm, int* high, int*  comm_out, int* ierr) ;
void mpi_bsend_ (void* buf, int* count, int* datatype, int* dest, int* tag, int* comm, int* ierr) ;
void mpi_bsend_init_ (void* buf, int* count, int* datatype, int* dest, int* tag, int* comm, int*  request, int* ierr) ;
void mpi_ibsend_ (void* buf, int* count, int* datatype, int* dest, int* tag, int* comm, int*  request, int* ierr) ;
void mpi_comm_remote_group_ (int* comm, int*  group, int* ierr) ;
void mpi_comm_remote_size_ (int* comm, int* size, int* ierr) ;
void mpi_issend_ (void* buf, int* count, int* datatype, int* dest, int* tag, int* comm, int*  request, int* ierr) ;
void mpi_probe_ (int* source, int* tag, int* comm, MPI_Status* status, int* ierr) ;
void mpi_attr_delete_ (int* comm, int* keyval, int* ierr) ;
void mpi_attr_put_ (int* comm, int* keyval, void* attr_value, int* ierr) ;
void mpi_rsend_init_ (void* buf, int* count, int* datatype, int* dest, int* tag, int* comm, int*  request, int* ierr) ;
void mpi_keyval_create_ (void* copy_fn, void* delete_fn, int* keyval, void* extra_state, int* ierr) ;
void mpi_keyval_free_ (int* keyval, int* ierr) ;
void mpi_test_cancelled_ (MPI_Status* status, int* flag, int* ierr) ;
void mpi_pack_ (void* inbuf, int* incount, int* type, void* outbuf, int* outcount, int* position, int* comm, int* ierr);
void mpi_get_elements_ (MPI_Status* status, int* datatype, int* elements, int* ierr) ;
void mpi_dims_create_ (int* nnodes, int* ndims, int* dims, int* ierr) ;
void mpi_iprobe_ (int* source, int* tag, int* comm, int* flag, MPI_Status* status, int* ierr) ;
void mpi_type_get_envelope_ ( int* datatype, int *num_integers, int *num_addresses, int *num_datatypes, int *combiner,
                              int* ierr);
void mpi_type_get_contents_ (int* datatype, int* max_integers, int* max_addresses, int* max_datatypes,
                             int* array_of_integers, MPI_Aint* array_of_addresses,
 int*array_of_datatypes, int* ierr);
void mpi_type_create_darray_ (int* size, int* rank, int* ndims, int* array_of_gsizes, int* array_of_distribs,
                              int* array_of_dargs, int* array_of_psizes,
 int* order, int* oldtype, int*newtype, int* ierr) ;
void mpi_type_create_resized_ (int* oldtype,MPI_Aint* lb, MPI_Aint* extent, int*newtype, int* ierr);
void mpi_type_create_subarray_ (int* ndims,int *array_of_sizes, int *array_of_subsizes, int *array_of_starts,
                                int* order, int* oldtype, int*newtype, int* ierr);
void mpi_type_match_size_ (int* typeclass,int* size,int*datatype, int* ierr);
void mpi_alltoallw_ ( void *sendbuf, int *sendcnts, int *sdispls, int*sendtypes, void *recvbuf, int *recvcnts,
                      int *rdispls, int*recvtypes, int* comm, int* ierr);
void mpi_exscan_ (void *sendbuf, void *recvbuf, int* count, int* datatype, int* op, int* comm, int* ierr);
void mpi_comm_set_name_ (int* comm, char* name, int* ierr, int size);
void mpi_comm_dup_with_info_ (int* comm, int* info, int* newcomm, int* ierr);
void mpi_comm_split_type_ (int* comm, int* split_type, int* key, int* info, int*newcomm, int* ierr);
void mpi_comm_set_info_ (int* comm, int* info, int* ierr);
void mpi_comm_get_info_ (int* comm, int* info, int* ierr);
void mpi_info_get_ (int* info,char *key,int* valuelen, char *value, int *flag, int* ierr, unsigned int keylen);
void mpi_comm_create_errhandler_ ( void *function, void *errhandler, int* ierr);
void mpi_add_error_class_ ( int *errorclass, int* ierr);
void mpi_add_error_code_ (  int* errorclass, int *errorcode, int* ierr);
void mpi_add_error_string_ ( int* errorcode, char *string, int* ierr);
void mpi_comm_call_errhandler_ (int* comm,int* errorcode, int* ierr);
void mpi_info_dup_ (int* info, int* newinfo, int* ierr);
void mpi_info_get_valuelen_ ( int* info, char *key, int *valuelen, int *flag, int* ierr, unsigned int keylen);
void mpi_info_delete_ (int* info, char *key, int* ierr, unsigned int keylen);
void mpi_info_get_nkeys_ ( int* info, int *nkeys, int* ierr);
void mpi_info_get_nthkey_ ( int* info, int* n, char *key, int* ierr, unsigned int keylen);
void mpi_get_version_ (int *version,int *subversion, int* ierr);
void mpi_get_library_version_ (char *version,int *len, int* ierr);
void mpi_request_get_status_ ( int* request, int *flag, MPI_Status* status, int* ierr);
void mpi_grequest_start_ ( void *query_fn, void *free_fn, void *cancel_fn, void *extra_state, int*request, int* ierr);
void mpi_grequest_complete_ ( int* request, int* ierr);
void mpi_status_set_cancelled_ (MPI_Status* status,int* flag, int* ierr);
void mpi_status_set_elements_ ( MPI_Status* status, int* datatype, int* count, int* ierr);
void mpi_comm_connect_ ( char *port_name, int* info, int* root, int* comm, int* newcomm, int* ierr);
void mpi_publish_name_ ( char *service_name, int* info, char *port_name, int* ierr);
void mpi_unpublish_name_ ( char *service_name, int* info, char *port_name, int* ierr);
void mpi_lookup_name_ ( char *service_name, int* info, char *port_name, int* ierr);
void mpi_comm_join_ ( int* fd, int*intercomm, int* ierr);
void mpi_open_port_ ( int* info, char *port_name, int* ierr);
void mpi_close_port_ ( char *port_name, int* ierr);
void mpi_comm_accept_ ( char *port_name, int* info, int* root, int* comm, int* newcomm, int* ierr);
void mpi_comm_spawn_ ( char *command, char *argv, int* maxprocs, int* info, int* root, int* comm, int*intercomm,
                       int* array_of_errcodes, int* ierr);
void mpi_comm_spawn_multiple_ ( int* count, char *array_of_commands, char** array_of_argv, int* array_of_maxprocs,
                       int* array_of_info, int* root, int* comm, int*intercomm, int* array_of_errcodes, int* ierr);
void mpi_comm_get_parent_ ( int*parent, int* ierr);

/********** Tracing **********/
/* from instr_smpi.c */
XBT_PRIVATE void TRACE_internal_smpi_set_category (const char *category);
XBT_PRIVATE const char *TRACE_internal_smpi_get_category (void);
XBT_PRIVATE void TRACE_smpi_collective_in(int rank, int root, const char *operation, instr_extra_data extra);
XBT_PRIVATE void TRACE_smpi_collective_out(int rank, int root, const char *operation);
XBT_PRIVATE void TRACE_smpi_computing_init(int rank);
XBT_PRIVATE void TRACE_smpi_computing_out(int rank);
XBT_PRIVATE void TRACE_smpi_computing_in(int rank, instr_extra_data extra);
XBT_PRIVATE void TRACE_smpi_sleeping_init(int rank);
XBT_PRIVATE void TRACE_smpi_sleeping_out(int rank);
XBT_PRIVATE void TRACE_smpi_sleeping_in(int rank, instr_extra_data extra);
XBT_PRIVATE void TRACE_smpi_testing_out(int rank);
XBT_PRIVATE void TRACE_smpi_testing_in(int rank, instr_extra_data extra);
XBT_PRIVATE void TRACE_smpi_alloc(void);
XBT_PRIVATE void TRACE_smpi_release(void);
XBT_PRIVATE void TRACE_smpi_ptp_in(int rank, int src, int dst, const char *operation,  instr_extra_data extra);
XBT_PRIVATE void TRACE_smpi_ptp_out(int rank, int src, int dst, const char *operation);
XBT_PRIVATE void TRACE_smpi_send(int rank, int src, int dst, int size);
XBT_PRIVATE void TRACE_smpi_recv(int rank, int src, int dst);
XBT_PRIVATE void TRACE_smpi_init(int rank);
XBT_PRIVATE void TRACE_smpi_finalize(int rank);

XBT_PRIVATE const char* encode_datatype(MPI_Datatype datatype, int* known);

// TODO, make this static and expose it more cleanly

typedef struct s_smpi_privatisation_region {
  void* address;
  int file_descriptor;
} s_smpi_privatisation_region_t, *smpi_privatisation_region_t;

extern XBT_PRIVATE smpi_privatisation_region_t smpi_privatisation_regions;
extern XBT_PRIVATE int smpi_loaded_page;
extern XBT_PRIVATE int smpi_universe_size;

XBT_PRIVATE int SIMIX_process_get_PID(smx_process_t self);

static inline __attribute__ ((always_inline))
int smpi_process_index_of_smx_process(smx_process_t process) {
  return SIMIX_process_get_PID(process) -1;
}

SG_END_DECL()
#endif
