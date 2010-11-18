/* Copyright (c) 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_PRIVATE_H
#define SMPI_PRIVATE_H

#include "xbt.h"
#include "xbt/xbt_os_time.h"
#include "simix/simix.h"
#include "smpi/smpi.h"
#include "instr/private.h"

struct s_smpi_process_data;
typedef struct s_smpi_process_data *smpi_process_data_t;

#define PERSISTENT     0x0
#define NON_PERSISTENT 0x1
#define SEND           0x0
#define RECV           0x2

typedef struct s_smpi_mpi_request {
  void *buf;
  size_t size;
  int src;
  int dst;
  int tag;
  MPI_Comm comm;
  smx_rdv_t rdv;
  smx_comm_t pair;
  int complete;
  MPI_Request match;
  unsigned flags;
#ifdef HAVE_TRACING
  int send;
  int recv;
#endif
} s_smpi_mpi_request_t;

void smpi_process_init(int *argc, char ***argv);
void smpi_process_destroy(void);

smpi_process_data_t smpi_process_data(void);
smpi_process_data_t smpi_process_remote_data(int index);
int smpi_process_count(void);
int smpi_process_index(void);
xbt_os_timer_t smpi_process_timer(void);
void smpi_process_simulated_start(void);
double smpi_process_simulated_elapsed(void);
void print_request(const char *message, MPI_Request request);
void smpi_process_post_send(MPI_Comm comm, MPI_Request request);
void smpi_process_post_recv(MPI_Request request);

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
int smpi_mpi_get_count(MPI_Status * status, MPI_Datatype datatype);
void smpi_mpi_wait(MPI_Request * request, MPI_Status * status);
int smpi_mpi_waitany(int count, MPI_Request requests[],
                     MPI_Status * status);
void smpi_mpi_waitall(int count, MPI_Request requests[],
                      MPI_Status status[]);
int smpi_mpi_waitsome(int incount, MPI_Request requests[], int *indices,
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
void smpi_bench_begin(int rank, const char *mpi_call);
void smpi_bench_end(int rank, const char *mpi_call);

#endif
