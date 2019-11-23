/* High level handling of collective algorithms */
/* Copyright (c) 2009-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_COLL_HPP
#define SMPI_COLL_HPP

#include "private.hpp"
#include "xbt/base.h"

/** @brief MPI collective description */

#define COLL_DEFS(cat, ret, args, args2)                                                                               \
  extern int(*cat) args;

#define COLL_SIG(cat, ret, args, args2) int cat args;

#define COLL_UNPAREN(...)  __VA_ARGS__
#define COLL_APPLY(action, sig, name) action(sig, name)

#define COLL_GATHER_SIG gather, int,                                                                                   \
    (const void *send_buff, int send_count, MPI_Datatype send_type,                                                    \
           void *recv_buff, int recv_count, MPI_Datatype recv_type,  int root, MPI_Comm comm)
#define COLL_ALLGATHER_SIG allgather, int,                                                                             \
    (const void *send_buff, int send_count, MPI_Datatype send_type,                                                    \
            void *recv_buff, int recv_count, MPI_Datatype recv_type,  MPI_Comm comm)
#define COLL_ALLGATHERV_SIG allgatherv, int,                                                                           \
    (const void *send_buff, int send_count, MPI_Datatype send_type,                                                    \
           void *recv_buff, const int *recv_count, const int *recv_disps,  MPI_Datatype recv_type, MPI_Comm comm)
#define COLL_ALLTOALL_SIG alltoall, int,                                                                               \
    (const void *send_buff, int send_count, MPI_Datatype send_type,                                                    \
           void *recv_buff, int recv_count, MPI_Datatype recv_type,  MPI_Comm comm)
#define COLL_ALLTOALLV_SIG alltoallv, int,                                                                             \
    (const void *send_buff, const int *send_counts, const int *send_disps, MPI_Datatype send_type,                     \
           void *recv_buff, const int *recv_counts, const int *recv_disps, MPI_Datatype recv_type, MPI_Comm comm)
#define COLL_BCAST_SIG bcast, int,                                                                                     \
    (void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
#define COLL_REDUCE_SIG reduce, int,                                                                                   \
    (const void *buf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
#define COLL_ALLREDUCE_SIG allreduce, int,                                                                             \
    (const void *sbuf, void *rbuf, int rcount,  MPI_Datatype dtype, MPI_Op op, MPI_Comm comm)
#define COLL_REDUCE_SCATTER_SIG reduce_scatter, int,                                                                   \
    (const void *sbuf, void *rbuf, const int *rcounts, MPI_Datatype dtype,MPI_Op  op,MPI_Comm  comm)
#define COLL_SCATTER_SIG scatter, int,                                                                                 \
    (const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
#define COLL_BARRIER_SIG barrier, int,                                                                                 \
    (MPI_Comm comm)

namespace simgrid {
namespace smpi {

struct s_mpi_coll_description_t {
  std::string name;
  std::string description;
  void *coll;
};

namespace colls {
void set_collectives();
XBT_PUBLIC std::vector<s_mpi_coll_description_t>* get_smpi_coll_descriptions(const std::string& name);

void set_gather(const std::string& name);
void set_allgather(const std::string& name);
void set_allgatherv(const std::string& name);
void set_reduce(const std::string& name);
void set_allreduce(const std::string& name);
void set_reduce_scatter(const std::string& name);
void set_scatter(const std::string& name);
void set_barrier(const std::string& name);
void set_bcast(const std::string& name);
void set_alltoall(const std::string& name);
void set_alltoallv(const std::string& name);

// for each collective type, create the set_* prototype, the description array and the function pointer
//  extern int(*gather)(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count,
//                      MPI_Datatype recv_type, int root, MPI_Comm comm);
COLL_APPLY(COLL_DEFS, COLL_GATHER_SIG, "")
COLL_APPLY(COLL_DEFS, COLL_ALLGATHER_SIG, "")
COLL_APPLY(COLL_DEFS, COLL_ALLGATHERV_SIG, "")
COLL_APPLY(COLL_DEFS, COLL_REDUCE_SIG, "")
COLL_APPLY(COLL_DEFS, COLL_ALLREDUCE_SIG, "")
COLL_APPLY(COLL_DEFS, COLL_REDUCE_SCATTER_SIG, "")
COLL_APPLY(COLL_DEFS, COLL_SCATTER_SIG, "")
COLL_APPLY(COLL_DEFS, COLL_BARRIER_SIG, "")
COLL_APPLY(COLL_DEFS, COLL_BCAST_SIG, "")
COLL_APPLY(COLL_DEFS, COLL_ALLTOALL_SIG, "")
COLL_APPLY(COLL_DEFS, COLL_ALLTOALLV_SIG, "")

// These fairly unused collectives only have one implementation in SMPI
int gatherv(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, const int* recvcounts,
            const int* displs, MPI_Datatype recvtype, int root, MPI_Comm comm);
int scatterv(const void* sendbuf, const int* sendcounts, const int* displs, MPI_Datatype sendtype, void* recvbuf,
             int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int scan(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int exscan(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int alltoallw(const void* sendbuf, const int* sendcounts, const int* senddisps, const MPI_Datatype* sendtypes,
              void* recvbuf, const int* recvcounts, const int* recvdisps, const MPI_Datatype* recvtypes, MPI_Comm comm);

// async collectives
int ibarrier(MPI_Comm comm, MPI_Request* request, int external = 1);
int ibcast(void* buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request* request,
           int external = 1);
int igather(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
            MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request* request, int external = 1);
int igatherv(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, const int* recvcounts,
             const int* displs, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request* request, int external = 1);
int iallgather(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
               MPI_Datatype recvtype, MPI_Comm comm, MPI_Request* request, int external = 1);
int iallgatherv(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, const int* recvcounts,
                const int* displs, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request* request, int external = 1);
int iscatter(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
             MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request* request, int external = 1);
int iscatterv(const void* sendbuf, const int* sendcounts, const int* displs, MPI_Datatype sendtype, void* recvbuf,
              int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request* request, int external = 1);
int ireduce(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm,
            MPI_Request* request, int external = 1);
int iallreduce(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
               MPI_Request* request, int external = 1);
int iscan(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
          MPI_Request* request, int external = 1);
int iexscan(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
            MPI_Request* request, int external = 1);
int ireduce_scatter(const void* sendbuf, void* recvbuf, const int* recvcounts, MPI_Datatype datatype, MPI_Op op,
                    MPI_Comm comm, MPI_Request* request, int external = 1);
int ireduce_scatter_block(const void* sendbuf, void* recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op,
                          MPI_Comm comm, MPI_Request* request, int external = 1);
int ialltoall(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
              MPI_Datatype recvtype, MPI_Comm comm, MPI_Request* request, int external = 1);
int ialltoallv(const void* sendbuf, const int* sendcounts, const int* senddisps, MPI_Datatype sendtype, void* recvbuf,
               const int* recvcounts, const int* recvdisps, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request* request,
               int external = 1);
int ialltoallw(const void* sendbuf, const int* sendcounts, const int* senddisps, const MPI_Datatype* sendtypes,
               void* recvbuf, const int* recvcounts, const int* recvdisps, const MPI_Datatype* recvtypes, MPI_Comm comm,
               MPI_Request* request, int external = 1);

extern void (*smpi_coll_cleanup_callback)();
}

/***********************************************
 * Prototypes of each and every implementation *
 ***********************************************/

int gather__default(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, int root, MPI_Comm comm);
int gather__ompi(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, int root, MPI_Comm comm);
int gather__ompi_basic_linear(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, int root, MPI_Comm comm);
int gather__ompi_binomial(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, int root, MPI_Comm comm);
int gather__ompi_linear_sync(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, int root, MPI_Comm comm);
int gather__mpich(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, int root, MPI_Comm comm);
int gather__mvapich2(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, int root, MPI_Comm comm);
int gather__mvapich2_two_level(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, int root, MPI_Comm comm);
int gather__impi(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, int root, MPI_Comm comm);
int gather__automatic(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, int root, MPI_Comm comm);

int allgather__default(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int allgather__2dmesh(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int allgather__3dmesh(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int allgather__bruck(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int allgather__GB(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int allgather__loosely_lr(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int allgather__NTSLR(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int allgather__NTSLR_NB(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int allgather__pair(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int allgather__rdb(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int allgather__rhv(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int allgather__ring(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int allgather__SMP_NTS(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int allgather__smp_simple(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int allgather__spreading_simple(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int allgather__ompi(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int allgather__ompi_neighborexchange (const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int allgather__mvapich2(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int allgather__mvapich2_smp(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int allgather__mpich(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int allgather__impi(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int allgather__automatic(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);

int allgatherv__default(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, const int *recv_count, const int *recv_disps, MPI_Datatype recv_type, MPI_Comm comm);
int allgatherv__GB(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, const int *recv_count, const int *recv_disps, MPI_Datatype recv_type, MPI_Comm comm);
int allgatherv__pair(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, const int *recv_count, const int *recv_disps, MPI_Datatype recv_type, MPI_Comm comm);
int allgatherv__ring(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, const int *recv_count, const int *recv_disps, MPI_Datatype recv_type, MPI_Comm comm);
int allgatherv__ompi(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, const int *recv_count, const int *recv_disps, MPI_Datatype recv_type, MPI_Comm comm);
int allgatherv__ompi_neighborexchange(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, const int *recv_count, const int *recv_disps, MPI_Datatype recv_type, MPI_Comm comm);
int allgatherv__ompi_bruck (const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, const int *recv_count, const int *recv_disps, MPI_Datatype recv_type, MPI_Comm comm);
int allgatherv__mpich(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, const int *recv_count, const int *recv_disps, MPI_Datatype recv_type, MPI_Comm comm);
int allgatherv__mpich_rdb(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, const int *recv_count, const int *recv_disps, MPI_Datatype recv_type, MPI_Comm comm);
int allgatherv__mpich_ring(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, const int *recv_count, const int *recv_disps, MPI_Datatype recv_type, MPI_Comm comm);
int allgatherv__mvapich2(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, const int *recv_count, const int *recv_disps, MPI_Datatype recv_type, MPI_Comm comm);
int allgatherv__impi(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, const int *recv_count, const int *recv_disps, MPI_Datatype recv_type, MPI_Comm comm);
int allgatherv__automatic(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, const int *recv_count, const int *recv_disps, MPI_Datatype recv_type, MPI_Comm comm);

int allreduce__default(const void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm);
int allreduce__lr(const void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm);
int allreduce__rab1(const void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm);
int allreduce__rab2(const void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm);
int allreduce__rab_rdb(const void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm);
int allreduce__rdb(const void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm);
int allreduce__smp_binomial(const void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm);
int allreduce__smp_binomial_pipeline(const void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm);
int allreduce__smp_rdb(const void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm);
int allreduce__smp_rsag(const void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm);
int allreduce__smp_rsag_lr(const void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm);
int allreduce__smp_rsag_rab(const void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm);
int allreduce__redbcast(const void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm);
int allreduce__ompi(const void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm);
int allreduce__ompi_ring_segmented(const void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm);
int allreduce__mpich(const void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm);
int allreduce__mvapich2(const void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm);
int allreduce__mvapich2_rs(const void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm);
int allreduce__mvapich2_two_level(const void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm);
int allreduce__impi(const void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm);
int allreduce__rab(const void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm);
int allreduce__automatic(const void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm);

int alltoall__default(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int alltoall__2dmesh(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int alltoall__3dmesh(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int alltoall__basic_linear(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int alltoall__bruck(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int alltoall__pair(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int alltoall__pair_rma(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int alltoall__pair_light_barrier(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int alltoall__pair_mpi_barrier(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int alltoall__pair_one_barrier(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int alltoall__rdb(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int alltoall__ring(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int alltoall__ring_light_barrier(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int alltoall__ring_mpi_barrier(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int alltoall__ring_one_barrier(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int alltoall__mvapich2(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int alltoall__mvapich2_scatter_dest(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int alltoall__ompi(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int alltoall__mpich(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int alltoall__impi(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);
int alltoall__automatic(const void *send_buff, int send_count, MPI_Datatype send_type, void *recv_buff, int recv_count, MPI_Datatype recv_type, MPI_Comm comm);

int alltoallv__default(const void *send_buff, const int *send_counts, const int *send_disps, MPI_Datatype send_type, void *recv_buff, const int *recv_counts, const int *recv_disps, MPI_Datatype recv_type, MPI_Comm comm);
int alltoallv__bruck(const void *send_buff, const int *send_counts, const int *send_disps, MPI_Datatype send_type, void *recv_buff, const int *recv_counts, const int *recv_disps, MPI_Datatype recv_type, MPI_Comm comm);
int alltoallv__pair(const void *send_buff, const int *send_counts, const int *send_disps, MPI_Datatype send_type, void *recv_buff, const int *recv_counts, const int *recv_disps, MPI_Datatype recv_type, MPI_Comm comm);
int alltoallv__pair_light_barrier(const void *send_buff, const int *send_counts, const int *send_disps, MPI_Datatype send_type, void *recv_buff, const int *recv_counts, const int *recv_disps, MPI_Datatype recv_type, MPI_Comm comm);
int alltoallv__pair_mpi_barrier(const void *send_buff, const int *send_counts, const int *send_disps, MPI_Datatype send_type, void *recv_buff, const int *recv_counts, const int *recv_disps, MPI_Datatype recv_type, MPI_Comm comm);
int alltoallv__pair_one_barrier(const void *send_buff, const int *send_counts, const int *send_disps, MPI_Datatype send_type, void *recv_buff, const int *recv_counts, const int *recv_disps, MPI_Datatype recv_type, MPI_Comm comm);
int alltoallv__ring(const void *send_buff, const int *send_counts, const int *send_disps, MPI_Datatype send_type, void *recv_buff, const int *recv_counts, const int *recv_disps, MPI_Datatype recv_type, MPI_Comm comm);
int alltoallv__ring_light_barrier(const void *send_buff, const int *send_counts, const int *send_disps, MPI_Datatype send_type, void *recv_buff, const int *recv_counts, const int *recv_disps, MPI_Datatype recv_type, MPI_Comm comm);
int alltoallv__ring_mpi_barrier(const void *send_buff, const int *send_counts, const int *send_disps, MPI_Datatype send_type, void *recv_buff, const int *recv_counts, const int *recv_disps, MPI_Datatype recv_type, MPI_Comm comm);
int alltoallv__ring_one_barrier(const void *send_buff, const int *send_counts, const int *send_disps, MPI_Datatype send_type, void *recv_buff, const int *recv_counts, const int *recv_disps, MPI_Datatype recv_type, MPI_Comm comm);
int alltoallv__ompi(const void *send_buff, const int *send_counts, const int *send_disps, MPI_Datatype send_type, void *recv_buff, const int *recv_counts, const int *recv_disps, MPI_Datatype recv_type, MPI_Comm comm);
int alltoallv__mpich(const void *send_buff, const int *send_counts, const int *send_disps, MPI_Datatype send_type, void *recv_buff, const int *recv_counts, const int *recv_disps, MPI_Datatype recv_type, MPI_Comm comm);
int alltoallv__ompi_basic_linear(const void *send_buff, const int *send_counts, const int *send_disps, MPI_Datatype send_type, void *recv_buff, const int *recv_counts, const int *recv_disps, MPI_Datatype recv_type, MPI_Comm comm);
int alltoallv__mvapich2(const void *send_buff, const int *send_counts, const int *send_disps, MPI_Datatype send_type, void *recv_buff, const int *recv_counts, const int *recv_disps, MPI_Datatype recv_type, MPI_Comm comm);
int alltoallv__impi(const void *send_buff, const int *send_counts, const int *send_disps, MPI_Datatype send_type, void *recv_buff, const int *recv_counts, const int *recv_disps, MPI_Datatype recv_type, MPI_Comm comm);
int alltoallv__automatic(const void *send_buff, const int *send_counts, const int *send_disps, MPI_Datatype send_type, void *recv_buff, const int *recv_counts, const int *recv_disps, MPI_Datatype recv_type, MPI_Comm comm);

int bcast__default(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int bcast__arrival_pattern_aware(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int bcast__arrival_pattern_aware_wait(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int bcast__arrival_scatter(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int bcast__binomial_tree(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int bcast__flattree(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int bcast__flattree_pipeline(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int bcast__NTSB(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int bcast__NTSL(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int bcast__NTSL_Isend(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int bcast__scatter_LR_allgather(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int bcast__scatter_rdb_allgather(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int bcast__SMP_binary(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int bcast__SMP_binomial(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int bcast__SMP_linear(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int bcast__ompi(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int bcast__ompi_split_bintree(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int bcast__ompi_pipeline(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int bcast__mpich(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int bcast__mvapich2(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int bcast__mvapich2_inter_node(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int bcast__mvapich2_intra_node(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int bcast__mvapich2_knomial_intra_node(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int bcast__impi(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int bcast__automatic(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);

int reduce__default(const void *buf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int reduce__arrival_pattern_aware(const void *buf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int reduce__binomial(const void *buf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int reduce__flat_tree(const void *buf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int reduce__NTSL(const void *buf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int reduce__scatter_gather(const void *buf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int reduce__ompi(const void *buf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int reduce__ompi_chain(const void *buf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int reduce__ompi_pipeline(const void *buf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int reduce__ompi_basic_linear(const void *buf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int reduce__ompi_in_order_binary(const void *buf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int reduce__ompi_binary(const void *buf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int reduce__ompi_binomial(const void *buf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int reduce__mpich(const void *buf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int reduce__mvapich2(const void *buf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int reduce__mvapich2_knomial(const void *buf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int reduce__mvapich2_two_level(const void *buf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int reduce__impi(const void *buf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int reduce__rab(const void *buf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int reduce__automatic(const void *buf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);

int reduce_scatter__default(const void *sbuf, void *rbuf, const int *rcounts, MPI_Datatype dtype,MPI_Op  op,MPI_Comm  comm);
int reduce_scatter__ompi(const void *sbuf, void *rbuf, const int *rcounts, MPI_Datatype dtype,MPI_Op  op,MPI_Comm  comm);
int reduce_scatter__ompi_basic_recursivehalving(const void *sbuf, void *rbuf, const int *rcounts, MPI_Datatype dtype,MPI_Op  op,MPI_Comm  comm);
int reduce_scatter__ompi_ring(const void *sbuf, void *rbuf, const int *rcounts, MPI_Datatype dtype,MPI_Op  op,MPI_Comm  comm);
int reduce_scatter__mpich(const void *sbuf, void *rbuf, const int *rcounts, MPI_Datatype dtype,MPI_Op  op,MPI_Comm  comm);
int reduce_scatter__mpich_pair(const void *sbuf, void *rbuf, const int *rcounts, MPI_Datatype dtype,MPI_Op  op,MPI_Comm  comm);
int reduce_scatter__mpich_rdb(const void *sbuf, void *rbuf, const int *rcounts, MPI_Datatype dtype,MPI_Op  op,MPI_Comm  comm);
int reduce_scatter__mpich_noncomm(const void *sbuf, void *rbuf, const int *rcounts, MPI_Datatype dtype,MPI_Op  op,MPI_Comm  comm);
int reduce_scatter__mvapich2(const void *sbuf, void *rbuf, const int *rcounts, MPI_Datatype dtype,MPI_Op  op,MPI_Comm  comm);
int reduce_scatter__impi(const void *sbuf, void *rbuf, const int *rcounts, MPI_Datatype dtype,MPI_Op  op,MPI_Comm  comm);
int reduce_scatter__automatic(const void *sbuf, void *rbuf, const int *rcounts, MPI_Datatype dtype,MPI_Op  op,MPI_Comm  comm);

int scatter__default(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int scatter__ompi(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int scatter__ompi_basic_linear(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int scatter__ompi_binomial(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int scatter__mpich(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int scatter__mvapich2(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int scatter__mvapich2_two_level_binomial(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int scatter__mvapich2_two_level_direct(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int scatter__impi (const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int scatter__automatic (const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);

int barrier__default(MPI_Comm comm);
int barrier__ompi(MPI_Comm comm);
int barrier__ompi_basic_linear(MPI_Comm comm);
int barrier__ompi_two_procs(MPI_Comm comm);
int barrier__ompi_tree(MPI_Comm comm);
int barrier__ompi_bruck(MPI_Comm comm);
int barrier__ompi_recursivedoubling(MPI_Comm comm);
int barrier__ompi_doublering(MPI_Comm comm);
int barrier__mpich_smp(MPI_Comm comm);
int barrier__mpich(MPI_Comm comm);
int barrier__mvapich2_pair(MPI_Comm comm);
int barrier__mvapich2 (MPI_Comm comm);
int barrier__impi(MPI_Comm comm);
int barrier__automatic(MPI_Comm comm);

}
}
#endif
