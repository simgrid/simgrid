/* Copyright (c) 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_PRIVATE_H
#define SMPI_PRIVATE_H

#include "xbt.h"
#include "xbt/xbt_os_time.h"
#include "simgrid/simix.h"
#include "smpi/smpi.h"
#include "smpi/smpif.h"
#include "smpi/smpi_cocci.h"
#include "instr/instr_private.h"

struct s_smpi_process_data;
typedef struct s_smpi_process_data *smpi_process_data_t;

#define PERSISTENT     0x1
#define NON_PERSISTENT 0x2
#define SEND           0x4
#define RECV           0x8


// this struct is here to handle the problem of non-contignous data
// for each such structure these function should be implemented (vector
// index hvector hindex struct)
typedef struct s_smpi_subtype{
  void (*serialize)(const void * input, void *output, size_t count, void* subtype);
  void (*unserialize)(const void * input, void *output, size_t count, void* subtype);
  void (*subtype_free)(MPI_Datatype* type);
} s_smpi_subtype_t;

typedef struct s_smpi_mpi_datatype{
  size_t size;
  /* this let us know if a serialization is required*/
  size_t has_subtype;
  MPI_Aint lb;
  MPI_Aint ub;
  int flags;
  /* this let us know how to serialize and unserialize*/
  void *substruct;
} s_smpi_mpi_datatype_t;

//*****************************************************************************************

typedef struct s_smpi_mpi_request {
  void *buf;
  /* in the case of non-contignous memory the user address shoud be keep
   * to unserialize the data inside the user memory*/
  void *old_buf;
  /* this let us know how tounserialize at the end of
   * the communication*/
  MPI_Datatype old_type;
  size_t size;
  int src;
  int dst;
  int tag;
  MPI_Comm comm;
  smx_action_t action;
  unsigned flags;
  int detached;
#ifdef HAVE_TRACING
  int send;
  int recv;
#endif
} s_smpi_mpi_request_t;

void smpi_process_init(int *argc, char ***argv);
void smpi_process_destroy(void);
void smpi_process_finalize(void);

smpi_process_data_t smpi_process_data(void);
smpi_process_data_t smpi_process_remote_data(int index);
void smpi_process_set_user_data(void *);
void* smpi_process_get_user_data(void);
int smpi_process_count(void);
smx_rdv_t smpi_process_mailbox(void);
smx_rdv_t smpi_process_remote_mailbox(int index);
smx_rdv_t smpi_process_mailbox_small(void);
smx_rdv_t smpi_process_remote_mailbox_small(int index);
xbt_os_timer_t smpi_process_timer(void);
void smpi_process_simulated_start(void);
double smpi_process_simulated_elapsed(void);

void print_request(const char *message, MPI_Request request);

void smpi_global_init(void);
void smpi_global_destroy(void);

size_t smpi_datatype_size(MPI_Datatype datatype);
MPI_Aint smpi_datatype_lb(MPI_Datatype datatype);
MPI_Aint smpi_datatype_ub(MPI_Datatype datatype);
int smpi_datatype_extent(MPI_Datatype datatype, MPI_Aint * lb,
                         MPI_Aint * extent);
int smpi_datatype_copy(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                       void *recvbuf, int recvcount,
                       MPI_Datatype recvtype);
int smpi_datatype_contiguous(int count, MPI_Datatype old_type,
                       MPI_Datatype* new_type);
int smpi_datatype_vector(int count, int blocklen, int stride,
                      MPI_Datatype old_type, MPI_Datatype* new_type);

int smpi_datatype_hvector(int count, int blocklen, MPI_Aint stride,
                      MPI_Datatype old_type, MPI_Datatype* new_type);
int smpi_datatype_indexed(int count, int* blocklens, int* indices,
                     MPI_Datatype old_type, MPI_Datatype* new_type);
int smpi_datatype_hindexed(int count, int* blocklens, MPI_Aint* indices,
                     MPI_Datatype old_type, MPI_Datatype* new_type);
int smpi_datatype_struct(int count, int* blocklens, MPI_Aint* indices,
                    MPI_Datatype* old_types, MPI_Datatype* new_type);

void smpi_datatype_create(MPI_Datatype* new_type, int size, int has_subtype, void *struct_type, int flags);


void smpi_datatype_free(MPI_Datatype* type);
void smpi_datatype_commit(MPI_Datatype* datatype);

void smpi_empty_status(MPI_Status * status);
MPI_Op smpi_op_new(MPI_User_function * function, int commute);
void smpi_op_destroy(MPI_Op op);
void smpi_op_apply(MPI_Op op, void *invec, void *inoutvec, int *len,
                   MPI_Datatype * datatype);

MPI_Group smpi_group_new(int size);
void smpi_group_destroy(MPI_Group group);
void smpi_group_set_mapping(MPI_Group group, int index, int rank);
int smpi_group_index(MPI_Group group, int rank);
int smpi_group_rank(MPI_Group group, int index);
int smpi_group_use(MPI_Group group);
int smpi_group_unuse(MPI_Group group);
int smpi_group_size(MPI_Group group);
int smpi_group_compare(MPI_Group group1, MPI_Group group2);

MPI_Comm smpi_comm_new(MPI_Group group);
void smpi_comm_destroy(MPI_Comm comm);
MPI_Group smpi_comm_group(MPI_Comm comm);
int smpi_comm_size(MPI_Comm comm);
void smpi_comm_get_name(MPI_Comm comm, char* name, int* len);
int smpi_comm_rank(MPI_Comm comm);
MPI_Comm smpi_comm_split(MPI_Comm comm, int color, int key);

MPI_Request smpi_mpi_send_init(void *buf, int count, MPI_Datatype datatype,
                               int dst, int tag, MPI_Comm comm);
MPI_Request smpi_mpi_recv_init(void *buf, int count, MPI_Datatype datatype,
                               int src, int tag, MPI_Comm comm);
void smpi_mpi_start(MPI_Request request);
void smpi_mpi_startall(int count, MPI_Request * requests);
void smpi_mpi_request_free(MPI_Request * request);
MPI_Request smpi_isend_init(void *buf, int count, MPI_Datatype datatype,
                            int dst, int tag, MPI_Comm comm);
MPI_Request smpi_mpi_isend(void *buf, int count, MPI_Datatype datatype,
                           int dst, int tag, MPI_Comm comm);
MPI_Request smpi_irecv_init(void *buf, int count, MPI_Datatype datatype,
                            int src, int tag, MPI_Comm comm);
MPI_Request smpi_mpi_irecv(void *buf, int count, MPI_Datatype datatype,
                           int src, int tag, MPI_Comm comm);
void smpi_mpi_recv(void *buf, int count, MPI_Datatype datatype, int src,
                   int tag, MPI_Comm comm, MPI_Status * status);
void smpi_mpi_send(void *buf, int count, MPI_Datatype datatype, int dst,
                   int tag, MPI_Comm comm);
void smpi_mpi_sendrecv(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                       int dst, int sendtag, void *recvbuf, int recvcount,
                       MPI_Datatype recvtype, int src, int recvtag,
                       MPI_Comm comm, MPI_Status * status);
int smpi_mpi_test(MPI_Request * request, MPI_Status * status);
int smpi_mpi_testany(int count, MPI_Request requests[], int *index,
                     MPI_Status * status);
int smpi_mpi_testall(int count, MPI_Request requests[],
                     MPI_Status status[]);
void smpi_mpi_probe(int source, int tag, MPI_Comm comm, MPI_Status* status);
void smpi_mpi_iprobe(int source, int tag, MPI_Comm comm, int* flag,
                    MPI_Status* status);
int smpi_mpi_get_count(MPI_Status * status, MPI_Datatype datatype);
void smpi_mpi_wait(MPI_Request * request, MPI_Status * status);
int smpi_mpi_waitany(int count, MPI_Request requests[],
                     MPI_Status * status);
void smpi_mpi_waitall(int count, MPI_Request requests[],
                      MPI_Status status[]);
int smpi_mpi_waitsome(int incount, MPI_Request requests[], int *indices,
                      MPI_Status status[]);
int smpi_mpi_testsome(int incount, MPI_Request requests[], int *indices,
                      MPI_Status status[]);
void smpi_mpi_bcast(void *buf, int count, MPI_Datatype datatype, int root,
                    MPI_Comm comm);
void smpi_mpi_barrier(MPI_Comm comm);
void smpi_mpi_gather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                     void *recvbuf, int recvcount, MPI_Datatype recvtype,
                     int root, MPI_Comm comm);
void smpi_mpi_gatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                      void *recvbuf, int *recvcounts, int *displs,
                      MPI_Datatype recvtype, int root, MPI_Comm comm);
void smpi_mpi_allgather(void *sendbuf, int sendcount,
                        MPI_Datatype sendtype, void *recvbuf,
                        int recvcount, MPI_Datatype recvtype,
                        MPI_Comm comm);
void smpi_mpi_allgatherv(void *sendbuf, int sendcount,
                         MPI_Datatype sendtype, void *recvbuf,
                         int *recvcounts, int *displs,
                         MPI_Datatype recvtype, MPI_Comm comm);
void smpi_mpi_scatter(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                      void *recvbuf, int recvcount, MPI_Datatype recvtype,
                      int root, MPI_Comm comm);
void smpi_mpi_scatterv(void *sendbuf, int *sendcounts, int *displs,
                       MPI_Datatype sendtype, void *recvbuf, int recvcount,
                       MPI_Datatype recvtype, int root, MPI_Comm comm);
void smpi_mpi_reduce(void *sendbuf, void *recvbuf, int count,
                     MPI_Datatype datatype, MPI_Op op, int root,
                     MPI_Comm comm);
void smpi_mpi_allreduce(void *sendbuf, void *recvbuf, int count,
                        MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
void smpi_mpi_scan(void *sendbuf, void *recvbuf, int count,
                   MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

void nary_tree_bcast(void *buf, int count, MPI_Datatype datatype, int root,
                     MPI_Comm comm, int arity);
void nary_tree_barrier(MPI_Comm comm, int arity);

int smpi_coll_tuned_alltoall_bruck(void *sendbuf, int sendcount,
                                   MPI_Datatype sendtype, void *recvbuf,
                                   int recvcount, MPI_Datatype recvtype,
                                   MPI_Comm comm);
int smpi_coll_tuned_alltoall_basic_linear(void *sendbuf, int sendcount,
                                          MPI_Datatype sendtype,
                                          void *recvbuf, int recvcount,
                                          MPI_Datatype recvtype,
                                          MPI_Comm comm);
int smpi_coll_tuned_alltoall_pairwise(void *sendbuf, int sendcount,
                                      MPI_Datatype sendtype, void *recvbuf,
                                      int recvcount, MPI_Datatype recvtype,
                                      MPI_Comm comm);
int smpi_coll_basic_alltoallv(void *sendbuf, int *sendcounts,
                              int *senddisps, MPI_Datatype sendtype,
                              void *recvbuf, int *recvcounts,
                              int *recvdisps, MPI_Datatype recvtype,
                              MPI_Comm comm);

// utilities
void smpi_bench_destroy(void);
void smpi_bench_begin(void);
void smpi_bench_end(void);
void smpi_execute_flops(double flops);

// f77 wrappers
void mpi_init__(int*);
void mpi_finalize__(int*);
void mpi_abort__(int* comm, int* errorcode, int* ierr);
void mpi_comm_rank__(int* comm, int* rank, int* ierr);
void mpi_comm_size__(int* comm, int* size, int* ierr);
double mpi_wtime__(void);

void mpi_comm_dup__(int* comm, int* newcomm, int* ierr);
void mpi_comm_split__(int* comm, int* color, int* key, int* comm_out, int* ierr);

void mpi_send_init__(void *buf, int* count, int* datatype, int* dst, int* tag,
                     int* comm, int* request, int* ierr);
void mpi_isend__(void *buf, int* count, int* datatype, int* dst,
                 int* tag, int* comm, int* request, int* ierr);
void mpi_send__(void* buf, int* count, int* datatype, int* dst,
                int* tag, int* comm, int* ierr);
void mpi_recv_init__(void *buf, int* count, int* datatype, int* src, int* tag,
                     int* comm, int* request, int* ierr);
void mpi_irecv__(void *buf, int* count, int* datatype, int* src, int* tag,
                 int* comm, int* request, int* ierr);
void mpi_recv__(void* buf, int* count, int* datatype, int* src,
                int* tag, int* comm, MPI_Status* status, int* ierr);
void mpi_start__(int* request, int* ierr);
void mpi_startall__(int* count, int* requests, int* ierr);
void mpi_wait__(int* request, MPI_Status* status, int* ierr);
void mpi_waitany__(int* count, int* requests, int* index, MPI_Status* status, int* ierr);
void mpi_waitall__(int* count, int* requests, MPI_Status* status, int* ierr);

void mpi_barrier__(int* comm, int* ierr);
void mpi_bcast__(void* buf, int* count, int* datatype, int* root, int* comm, int* ierr);
void mpi_reduce__(void* sendbuf, void* recvbuf, int* count,
                  int* datatype, int* op, int* root, int* comm, int* ierr);
void mpi_allreduce__(void* sendbuf, void* recvbuf, int* count, int* datatype,
                     int* op, int* comm, int* ierr);
void mpi_scatter__(void* sendbuf, int* sendcount, int* sendtype,
                   void* recvbuf, int* recvcount, int* recvtype,
                   int* root, int* comm, int* ierr);
void mpi_gather__(void* sendbuf, int* sendcount, int* sendtype,
                  void* recvbuf, int* recvcount, int* recvtype,
                  int* root, int* comm, int* ierr);
void mpi_allgather__(void* sendbuf, int* sendcount, int* sendtype,
                     void* recvbuf, int* recvcount, int* recvtype,
                     int* comm, int* ierr);
void mpi_scan__(void* sendbuf, void* recvbuf, int* count, int* datatype,
                int* op, int* comm, int* ierr);
void mpi_alltoall__(void* sendbuf, int* sendcount, int* sendtype,
                    void* recvbuf, int* recvcount, int* recvtype, int* comm, int* ierr);

#endif
