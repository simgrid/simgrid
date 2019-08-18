/*High level handling of collective algorithms*/
/* Copyright (c) 2009-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_COLL_HPP
#define SMPI_COLL_HPP

#include "private.hpp"
#include "xbt/base.h"

/** @brief MPI collective description */

#define COLL_DEFS(cat, ret, args, args2)                                                                               \
  static void _XBT_CONCAT(set_, cat)(const std::string& name);                                                         \
  static s_mpi_coll_description_t _XBT_CONCAT3(mpi_coll_, cat, _description)[];                                        \
  static int(*cat) args;

#define COLL_SIG(cat, ret, args, args2)\
    static int cat args;

#define COLL_DESCRIPTION(cat, ret, args, name)                                                                         \
  {                                                                                                                    \
    _XBT_STRINGIFY(name)                                                                                               \
    , _XBT_STRINGIFY(cat) " " _XBT_STRINGIFY(name) " collective", (void*)_XBT_CONCAT4(Coll_, cat, _, name)::cat        \
  }

#define COLL_PROTO(cat, ret, args, name)                                                                               \
  class _XBT_CONCAT4(Coll_, cat, _, name) : public Coll {                                                              \
  public:                                                                                                              \
    static ret cat(COLL_UNPAREN args);                                                                                 \
  };

#define COLL_UNPAREN(...)  __VA_ARGS__

#define COLL_APPLY(action, sig, name) action(sig, name)
#define COLL_COMMA ,
#define COLL_NOsep
#define COLL_NOTHING(...)

#define COLL_GATHER_SIG gather, int, \
                      (const void *send_buff, int send_count, MPI_Datatype send_type, \
                       void *recv_buff, int recv_count, MPI_Datatype recv_type, \
                           int root, MPI_Comm comm)
#define COLL_ALLGATHER_SIG allgather, int, \
                      (const void *send_buff, int send_count, MPI_Datatype send_type, \
                       void *recv_buff, int recv_count, MPI_Datatype recv_type, \
                           MPI_Comm comm)
#define COLL_ALLGATHERV_SIG allgatherv, int, \
                      (const void *send_buff, int send_count, MPI_Datatype send_type, \
                       void *recv_buff, const int *recv_count, const int *recv_disps, \
               MPI_Datatype recv_type, MPI_Comm comm)
#define COLL_ALLTOALL_SIG alltoall, int, \
                     (const void *send_buff, int send_count, MPI_Datatype send_type, \
                      void *recv_buff, int recv_count, MPI_Datatype recv_type, \
                          MPI_Comm comm)
#define COLL_ALLTOALLV_SIG alltoallv, int, \
                     (const void *send_buff, const int *send_counts, const int *send_disps, MPI_Datatype send_type, \
                      void *recv_buff, const int *recv_counts, const int *recv_disps, MPI_Datatype recv_type, \
                          MPI_Comm comm)
#define COLL_BCAST_SIG bcast, int, \
                  (void *buf, int count, MPI_Datatype datatype, \
                   int root, MPI_Comm comm)
#define COLL_REDUCE_SIG reduce, int, \
                   (const void *buf, void *rbuf, int count, MPI_Datatype datatype, \
                        MPI_Op op, int root, MPI_Comm comm)
#define COLL_ALLREDUCE_SIG allreduce, int, \
                      (const void *sbuf, void *rbuf, int rcount, \
                           MPI_Datatype dtype, MPI_Op op, MPI_Comm comm)
#define COLL_REDUCE_SCATTER_SIG reduce_scatter, int, \
                      (const void *sbuf, void *rbuf, const int *rcounts,\
                    MPI_Datatype dtype,MPI_Op  op,MPI_Comm  comm)
#define COLL_SCATTER_SIG scatter, int, \
                (const void *sendbuf, int sendcount, MPI_Datatype sendtype,\
                void *recvbuf, int recvcount, MPI_Datatype recvtype,\
                int root, MPI_Comm comm)
#define COLL_BARRIER_SIG barrier, int, \
                (MPI_Comm comm)

namespace simgrid{
namespace smpi{

struct s_mpi_coll_description_t {
  std::string name;
  std::string description;
  void *coll;
};

class Colls{
public:
  static XBT_PUBLIC void coll_help(const char* category, s_mpi_coll_description_t* table);
  static XBT_PUBLIC int find_coll_description(s_mpi_coll_description_t* table, const std::string& name,
                                              const char* desc);
  static void set_collectives();

  // for each collective type, create the set_* prototype, the description array and the function pointer
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
  static int gatherv(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, const int* recvcounts, const int* displs,
                     MPI_Datatype recvtype, int root, MPI_Comm comm);
  static int scatterv(const void* sendbuf, const int* sendcounts, const int* displs, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                      MPI_Datatype recvtype, int root, MPI_Comm comm);
  static int scan(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
  static int exscan(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
  static int alltoallw
         (const void* sendbuf, const int* sendcounts, const int* senddisps, const MPI_Datatype* sendtypes, void* recvbuf, const int* recvcounts,
          const int* recvdisps, const MPI_Datatype* recvtypes, MPI_Comm comm);

  //async collectives
  static int ibarrier(MPI_Comm comm, MPI_Request* request, int external=1);
  static int ibcast(void *buf, int count, MPI_Datatype datatype, 
                   int root, MPI_Comm comm, MPI_Request* request, int external=1);
  static int igather (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                                      MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request, int external=1);
  static int igatherv (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
                                       const int* recvcounts, const int* displs, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request, int external=1);
  static int iallgather (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
                                         int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request, int external=1);
  static int iallgatherv (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
                                          const int* recvcounts, const int* displs, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request, int external=1);
  static int iscatter (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
                                       int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request, int external=1);
  static int iscatterv (const void* sendbuf, const int* sendcounts, const int* displs, MPI_Datatype sendtype,
                                        void* recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request, int external=1);
  static int ireduce
         (const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request *request, int external=1);
  static int iallreduce
         (const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request, int external=1);
  static int iscan
         (const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request, int external=1);
  static int iexscan
         (const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request, int external=1);
  static int ireduce_scatter
         (const void* sendbuf, void* recvbuf, const int* recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request, int external=1);
  static int ireduce_scatter_block
         (const void* sendbuf, void* recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request, int external=1);
  static int ialltoall (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
                                        int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request, int external=1);
  static int ialltoallv
         (const void* sendbuf, const int* sendcounts, const int* senddisps, MPI_Datatype sendtype, void* recvbuf, const int* recvcounts,
          const int* recvdisps, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request, int external=1);
  static int ialltoallw
         (const void* sendbuf, const int* sendcounts, const int* senddisps, const MPI_Datatype* sendtypes, void* recvbuf, const int* recvcounts,
          const int* recvdisps, const MPI_Datatype* recvtypes, MPI_Comm comm, MPI_Request *request, int external=1);


  static void (*smpi_coll_cleanup_callback)();
};

class Coll {
public:
  // for each collective type, create a function member
  COLL_APPLY(COLL_SIG, COLL_GATHER_SIG, "")
  COLL_APPLY(COLL_SIG, COLL_ALLGATHER_SIG, "")
  COLL_APPLY(COLL_SIG, COLL_ALLGATHERV_SIG, "")
  COLL_APPLY(COLL_SIG, COLL_REDUCE_SIG, "")
  COLL_APPLY(COLL_SIG, COLL_ALLREDUCE_SIG, "")
  COLL_APPLY(COLL_SIG, COLL_REDUCE_SCATTER_SIG, "")
  COLL_APPLY(COLL_SIG, COLL_SCATTER_SIG, "")
  COLL_APPLY(COLL_SIG, COLL_BARRIER_SIG, "")
  COLL_APPLY(COLL_SIG, COLL_BCAST_SIG, "")
  COLL_APPLY(COLL_SIG, COLL_ALLTOALL_SIG, "")
  COLL_APPLY(COLL_SIG, COLL_ALLTOALLV_SIG, "")
};

/*************
 * GATHER *
 *************/

#define COLL_GATHERS(action, COLL_sep) \
COLL_APPLY(action, COLL_GATHER_SIG, default) COLL_sep \
COLL_APPLY(action, COLL_GATHER_SIG, ompi) COLL_sep \
COLL_APPLY(action, COLL_GATHER_SIG, ompi_basic_linear) COLL_sep \
COLL_APPLY(action, COLL_GATHER_SIG, ompi_binomial) COLL_sep \
COLL_APPLY(action, COLL_GATHER_SIG, ompi_linear_sync) COLL_sep \
COLL_APPLY(action, COLL_GATHER_SIG, mpich) COLL_sep \
COLL_APPLY(action, COLL_GATHER_SIG, mvapich2) COLL_sep \
COLL_APPLY(action, COLL_GATHER_SIG, mvapich2_two_level) COLL_sep \
COLL_APPLY(action, COLL_GATHER_SIG, impi) COLL_sep \
COLL_APPLY(action, COLL_GATHER_SIG, automatic)

COLL_GATHERS(COLL_PROTO, COLL_NOsep)

/*************
 * ALLGATHER *
 *************/

#define COLL_ALLGATHERS(action, COLL_sep) \
COLL_APPLY(action, COLL_ALLGATHER_SIG, default) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, 2dmesh) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, 3dmesh) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, bruck) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, GB) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, loosely_lr) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, NTSLR) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, NTSLR_NB) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, pair) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, rdb) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, rhv) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, ring) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, SMP_NTS) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, smp_simple) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, spreading_simple) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, ompi) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, ompi_neighborexchange) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, mvapich2) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, mvapich2_smp) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, mpich) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, impi) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, automatic)

COLL_ALLGATHERS(COLL_PROTO, COLL_NOsep)

/**************
 * ALLGATHERV *
 **************/

#define COLL_ALLGATHERVS(action, COLL_sep) \
COLL_APPLY(action, COLL_ALLGATHERV_SIG, default) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHERV_SIG, GB) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHERV_SIG, pair) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHERV_SIG, ring) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHERV_SIG, ompi) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHERV_SIG, ompi_neighborexchange) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHERV_SIG, ompi_bruck) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHERV_SIG, mpich) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHERV_SIG, mpich_rdb) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHERV_SIG, mpich_ring) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHERV_SIG, mvapich2) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHERV_SIG, impi) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHERV_SIG, automatic)

COLL_ALLGATHERVS(COLL_PROTO, COLL_NOsep)

/*************
 * ALLREDUCE *
 *************/

#define COLL_ALLREDUCES(action, COLL_sep) \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, default) COLL_sep \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, lr) COLL_sep \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, rab1) COLL_sep \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, rab2) COLL_sep \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, rab_rdb) COLL_sep \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, rdb) COLL_sep \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, smp_binomial) COLL_sep \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, smp_binomial_pipeline) COLL_sep \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, smp_rdb) COLL_sep \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, smp_rsag) COLL_sep \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, smp_rsag_lr) COLL_sep \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, smp_rsag_rab) COLL_sep \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, redbcast) COLL_sep \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, ompi) COLL_sep \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, ompi_ring_segmented) COLL_sep \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, mpich) COLL_sep \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, mvapich2) COLL_sep \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, mvapich2_rs) COLL_sep \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, mvapich2_two_level) COLL_sep \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, impi) COLL_sep \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, rab) COLL_sep \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, automatic)

COLL_ALLREDUCES(COLL_PROTO, COLL_NOsep)

/************
 * ALLTOALL *
 ************/

#define COLL_ALLTOALLS(action, COLL_sep) \
COLL_APPLY(action, COLL_ALLTOALL_SIG, default) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALL_SIG, 2dmesh) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALL_SIG, 3dmesh) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALL_SIG, basic_linear) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALL_SIG, bruck) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALL_SIG, pair) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALL_SIG, pair_rma) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALL_SIG, pair_light_barrier) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALL_SIG, pair_mpi_barrier) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALL_SIG, pair_one_barrier) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALL_SIG, rdb) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALL_SIG, ring) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALL_SIG, ring_light_barrier) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALL_SIG, ring_mpi_barrier) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALL_SIG, ring_one_barrier) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALL_SIG, mvapich2) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALL_SIG, mvapich2_scatter_dest) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALL_SIG, ompi) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALL_SIG, mpich) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALL_SIG, impi) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALL_SIG, automatic)

COLL_ALLTOALLS(COLL_PROTO, COLL_NOsep)

/*************
 * ALLTOALLV *
 *************/

#define COLL_ALLTOALLVS(action, COLL_sep) \
COLL_APPLY(action, COLL_ALLTOALLV_SIG, default) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALLV_SIG, bruck) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALLV_SIG, pair) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALLV_SIG, pair_light_barrier) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALLV_SIG, pair_mpi_barrier) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALLV_SIG, pair_one_barrier) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALLV_SIG, ring) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALLV_SIG, ring_light_barrier) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALLV_SIG, ring_mpi_barrier) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALLV_SIG, ring_one_barrier) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALLV_SIG, ompi) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALLV_SIG, mpich) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALLV_SIG, ompi_basic_linear) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALLV_SIG, mvapich2) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALLV_SIG, impi) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALLV_SIG, automatic)

COLL_ALLTOALLVS(COLL_PROTO, COLL_NOsep)

/*********
 * BCAST *
 *********/

#define COLL_BCASTS(action, COLL_sep) \
COLL_APPLY(action, COLL_BCAST_SIG, default) COLL_sep \
COLL_APPLY(action, COLL_BCAST_SIG, arrival_pattern_aware) COLL_sep \
COLL_APPLY(action, COLL_BCAST_SIG, arrival_pattern_aware_wait) COLL_sep \
COLL_APPLY(action, COLL_BCAST_SIG, arrival_scatter) COLL_sep \
COLL_APPLY(action, COLL_BCAST_SIG, binomial_tree) COLL_sep \
COLL_APPLY(action, COLL_BCAST_SIG, flattree) COLL_sep \
COLL_APPLY(action, COLL_BCAST_SIG, flattree_pipeline) COLL_sep \
COLL_APPLY(action, COLL_BCAST_SIG, NTSB) COLL_sep \
COLL_APPLY(action, COLL_BCAST_SIG, NTSL) COLL_sep \
COLL_APPLY(action, COLL_BCAST_SIG, NTSL_Isend) COLL_sep \
COLL_APPLY(action, COLL_BCAST_SIG, scatter_LR_allgather) COLL_sep \
COLL_APPLY(action, COLL_BCAST_SIG, scatter_rdb_allgather) COLL_sep \
COLL_APPLY(action, COLL_BCAST_SIG, SMP_binary) COLL_sep \
COLL_APPLY(action, COLL_BCAST_SIG, SMP_binomial) COLL_sep \
COLL_APPLY(action, COLL_BCAST_SIG, SMP_linear) COLL_sep \
COLL_APPLY(action, COLL_BCAST_SIG, ompi) COLL_sep \
COLL_APPLY(action, COLL_BCAST_SIG, ompi_split_bintree) COLL_sep \
COLL_APPLY(action, COLL_BCAST_SIG, ompi_pipeline) COLL_sep \
COLL_APPLY(action, COLL_BCAST_SIG, mpich) COLL_sep \
COLL_APPLY(action, COLL_BCAST_SIG, mvapich2)   COLL_sep \
COLL_APPLY(action, COLL_BCAST_SIG, mvapich2_inter_node)   COLL_sep \
COLL_APPLY(action, COLL_BCAST_SIG, mvapich2_intra_node)   COLL_sep \
COLL_APPLY(action, COLL_BCAST_SIG, mvapich2_knomial_intra_node)   COLL_sep \
COLL_APPLY(action, COLL_BCAST_SIG, impi)   COLL_sep \
COLL_APPLY(action, COLL_BCAST_SIG, automatic)

COLL_BCASTS(COLL_PROTO, COLL_NOsep)

/**********
 * REDUCE *
 **********/

#define COLL_REDUCES(action, COLL_sep) \
COLL_APPLY(action, COLL_REDUCE_SIG, default) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SIG, arrival_pattern_aware) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SIG, binomial) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SIG, flat_tree) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SIG, NTSL) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SIG, scatter_gather) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SIG, ompi) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SIG, ompi_chain) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SIG, ompi_pipeline) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SIG, ompi_basic_linear) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SIG, ompi_in_order_binary) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SIG, ompi_binary) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SIG, ompi_binomial) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SIG, mpich) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SIG, mvapich2) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SIG, mvapich2_knomial) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SIG, mvapich2_two_level) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SIG, impi) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SIG, rab) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SIG, automatic)

COLL_REDUCES(COLL_PROTO, COLL_NOsep)

/*************
 * REDUCE_SCATTER *
 *************/

#define COLL_REDUCE_SCATTERS(action, COLL_sep) \
COLL_APPLY(action, COLL_REDUCE_SCATTER_SIG, default) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SCATTER_SIG, ompi) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SCATTER_SIG, ompi_basic_recursivehalving) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SCATTER_SIG, ompi_ring)  COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SCATTER_SIG, mpich) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SCATTER_SIG, mpich_pair) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SCATTER_SIG, mpich_rdb) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SCATTER_SIG, mpich_noncomm) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SCATTER_SIG, mvapich2) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SCATTER_SIG, impi) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SCATTER_SIG, automatic)

COLL_REDUCE_SCATTERS(COLL_PROTO, COLL_NOsep)

/*************
 * SCATTER *
 *************/

#define COLL_SCATTERS(action, COLL_sep) \
COLL_APPLY(action, COLL_SCATTER_SIG, default) COLL_sep \
COLL_APPLY(action, COLL_SCATTER_SIG, ompi) COLL_sep \
COLL_APPLY(action, COLL_SCATTER_SIG, ompi_basic_linear) COLL_sep \
COLL_APPLY(action, COLL_SCATTER_SIG, ompi_binomial)  COLL_sep \
COLL_APPLY(action, COLL_SCATTER_SIG, mpich)   COLL_sep \
COLL_APPLY(action, COLL_SCATTER_SIG, mvapich2)   COLL_sep \
COLL_APPLY(action, COLL_SCATTER_SIG, mvapich2_two_level_binomial)   COLL_sep \
COLL_APPLY(action, COLL_SCATTER_SIG, mvapich2_two_level_direct)   COLL_sep \
COLL_APPLY(action, COLL_SCATTER_SIG, impi)   COLL_sep \
COLL_APPLY(action, COLL_SCATTER_SIG, automatic)

COLL_SCATTERS(COLL_PROTO, COLL_NOsep)

/*************
 * BARRIER *
 *************/

#define COLL_BARRIERS(action, COLL_sep) \
COLL_APPLY(action, COLL_BARRIER_SIG, default) COLL_sep \
COLL_APPLY(action, COLL_BARRIER_SIG, ompi) COLL_sep \
COLL_APPLY(action, COLL_BARRIER_SIG, ompi_basic_linear) COLL_sep \
COLL_APPLY(action, COLL_BARRIER_SIG, ompi_two_procs)  COLL_sep \
COLL_APPLY(action, COLL_BARRIER_SIG, ompi_tree)  COLL_sep \
COLL_APPLY(action, COLL_BARRIER_SIG, ompi_bruck)  COLL_sep \
COLL_APPLY(action, COLL_BARRIER_SIG, ompi_recursivedoubling) COLL_sep \
COLL_APPLY(action, COLL_BARRIER_SIG, ompi_doublering) COLL_sep \
COLL_APPLY(action, COLL_BARRIER_SIG, mpich_smp)   COLL_sep \
COLL_APPLY(action, COLL_BARRIER_SIG, mpich)   COLL_sep \
COLL_APPLY(action, COLL_BARRIER_SIG, mvapich2_pair)   COLL_sep \
COLL_APPLY(action, COLL_BARRIER_SIG, mvapich2)   COLL_sep \
COLL_APPLY(action, COLL_BARRIER_SIG, impi)   COLL_sep \
COLL_APPLY(action, COLL_BARRIER_SIG, automatic)

COLL_BARRIERS(COLL_PROTO, COLL_NOsep)

}
}
#endif
