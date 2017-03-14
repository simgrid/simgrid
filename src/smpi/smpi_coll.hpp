/*High level handling of collective algorithms*/
/* Copyright (c) 2009-2010, 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_COLL_HPP
#define SMPI_COLL_HPP

#include <xbt/base.h>

#include "private.h"

/** \brief MPI collective description */

struct mpi_coll_description {
  const char *name;
  const char *description;
  void *coll;
};
typedef struct mpi_coll_description  s_mpi_coll_description_t;
typedef struct mpi_coll_description* mpi_coll_description_t;


namespace simgrid{
namespace smpi{


class Colls{
  private:
  public:
    static void set_gather(const char* name);
    static void set_allgather(const char* name);
    static void set_allgatherv(const char* name);
    static void set_alltoall(const char* name);
    static void set_alltoallv(const char* name);
    static void set_allreduce(const char* name);
    static void set_reduce(const char* name);
    static void set_reduce_scatter(const char* name);
    static void set_scatter(const char* name);
    static void set_barrier(const char* name);
    static void set_bcast(const char* name);

    static s_mpi_coll_description_t mpi_coll_gather_description[];
    static s_mpi_coll_description_t mpi_coll_allgather_description[];
    static s_mpi_coll_description_t mpi_coll_allgatherv_description[];
    static s_mpi_coll_description_t mpi_coll_allreduce_description[];
    static s_mpi_coll_description_t mpi_coll_alltoall_description[];
    static s_mpi_coll_description_t mpi_coll_alltoallv_description[];
    static s_mpi_coll_description_t mpi_coll_bcast_description[];
    static s_mpi_coll_description_t mpi_coll_reduce_description[];
    static s_mpi_coll_description_t mpi_coll_reduce_scatter_description[];
    static s_mpi_coll_description_t mpi_coll_scatter_description[];
    static s_mpi_coll_description_t mpi_coll_barrier_description[];


    static XBT_PUBLIC(void) coll_help(const char *category, s_mpi_coll_description_t * table);
    static XBT_PUBLIC(int) find_coll_description(s_mpi_coll_description_t * table, const char *name, const char *desc);
    static void set_collectives();

    static int (*gather)(void *, int, MPI_Datatype, void*, int, MPI_Datatype, int root, MPI_Comm);
    static int (*allgather)(void *, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm);
    static int (*allgatherv)(void *, int, MPI_Datatype, void*, int*, int*, MPI_Datatype, MPI_Comm);
    static int (*allreduce)(void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm);
    static int (*alltoall)(void *, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm);
    static int (*alltoallv)(void *, int*, int*, MPI_Datatype, void*, int*, int*, MPI_Datatype, MPI_Comm);
    static int (*bcast)(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm com);
    static int (*reduce)(void *buf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
    static int (*reduce_scatter)(void *sbuf, void *rbuf, int *rcounts,MPI_Datatype dtype,MPI_Op  op,MPI_Comm  comm);
    static int (*scatter)(void *sendbuf, int sendcount, MPI_Datatype sendtype,void *recvbuf, int recvcount, MPI_Datatype recvtype,int root, MPI_Comm comm);
    static int (*barrier)(MPI_Comm comm);

//These fairly unused collectives only have one implementation in SMPI

    static int gatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int *recvcounts, int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm);
    static int scatterv(void *sendbuf, int *sendcounts, int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
    static int scan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
    static int exscan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

    static void (*smpi_coll_cleanup_callback)();
};

class Coll_algo{
  private:
    char* description_;
  public:
    char* description();
};

class Coll_gather : public Coll_algo {
  private:
  public:
    static int gather (void *, int, MPI_Datatype, void*, int, MPI_Datatype, int root, MPI_Comm);
};

class Coll_allgather : public Coll_algo {
  private:
  public:
    static int allgather (void *, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm);
};

class Coll_allgatherv : public Coll_algo {
  private:
  public:
    static int allgatherv (void *, int, MPI_Datatype, void*, int*, int*, MPI_Datatype, MPI_Comm);
};

class Coll_allreduce : public Coll_algo {
  private:
  public:
    static int allreduce (void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm);
};

class Coll_alltoall : public Coll_algo {
  private:
  public:
    static int alltoall (void *, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm);
};

class Coll_alltoallv : public Coll_algo {
  private:
  public:
    static int alltoallv (void *, int*, int*, MPI_Datatype, void*, int*, int*, MPI_Datatype, MPI_Comm);
};

class Coll_bcast : public Coll_algo {
  private:
  public:
    static int bcast (void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm com);
};

class Coll_reduce : public Coll_algo {
  private:
  public:
    static int reduce (void *buf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
};

class Coll_reduce_scatter : public Coll_algo {
  private:
  public:
    static int reduce_scatter (void *sbuf, void *rbuf, int *rcounts,MPI_Datatype dtype,MPI_Op  op,MPI_Comm  comm);
};

class Coll_scatter : public Coll_algo {
  private:
  public:
    static int scatter (void *sendbuf, int sendcount, MPI_Datatype sendtype,void *recvbuf, int recvcount, MPI_Datatype recvtype,int root, MPI_Comm comm);
};

class Coll_barrier : public Coll_algo {
  private:
  public:
    static int barrier (MPI_Comm);
};



#define COLL_DESCRIPTION(cat, ret, args, name) \
  {# name,\
   # cat " " # name " collective",\
   (void*) Coll_ ## cat ## _ ## name::cat }

#define COLL_PROTO(cat, ret, args, name) \
class Coll_ ## cat ## _ ## name : public Coll_ ## cat { \
public: \
static ret cat  (COLL_UNPAREN args); \
};

#define COLL_UNPAREN(...)  __VA_ARGS__

#define COLL_APPLY(action, sig, name) action(sig, name)
#define COLL_COMMA ,
#define COLL_NOsep 
#define COLL_NOTHING(...) 

/*************
 * GATHER *
 *************/

#define COLL_GATHER_SIG gather, int, \
	                  (void *send_buff, int send_count, MPI_Datatype send_type, \
	                   void *recv_buff, int recv_count, MPI_Datatype recv_type, \
                           int root, MPI_Comm comm)

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
#define COLL_ALLGATHER_SIG allgather, int, \
	                  (void *send_buff, int send_count, MPI_Datatype send_type, \
	                   void *recv_buff, int recv_count, MPI_Datatype recv_type, \
                           MPI_Comm comm)

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
#define COLL_ALLGATHERV_SIG allgatherv, int, \
	                  (void *send_buff, int send_count, MPI_Datatype send_type, \
	                   void *recv_buff, int *recv_count, int *recv_disps, \
			   MPI_Datatype recv_type, MPI_Comm comm)

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
#define COLL_ALLREDUCE_SIG allreduce, int, \
	                  (void *sbuf, void *rbuf, int rcount, \
                           MPI_Datatype dtype, MPI_Op op, MPI_Comm comm)

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
#define COLL_ALLTOALL_SIG alltoall, int, \
	                 (void *send_buff, int send_count, MPI_Datatype send_type, \
	                  void *recv_buff, int recv_count, MPI_Datatype recv_type, \
                          MPI_Comm comm)

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
#define COLL_ALLTOALLV_SIG alltoallv, int, \
	                 (void *send_buff, int *send_counts, int *send_disps, MPI_Datatype send_type, \
	                  void *recv_buff, int *recv_counts, int *recv_disps, MPI_Datatype recv_type, \
                          MPI_Comm comm)

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
#define COLL_BCAST_SIG bcast, int, \
	              (void *buf, int count, MPI_Datatype datatype, \
	               int root, MPI_Comm comm)

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
#define COLL_REDUCE_SIG reduce, int, \
	               (void *buf, void *rbuf, int count, MPI_Datatype datatype, \
                        MPI_Op op, int root, MPI_Comm comm)

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
#define COLL_REDUCE_SCATTER_SIG reduce_scatter, int, \
	                  (void *sbuf, void *rbuf, int *rcounts,\
                    MPI_Datatype dtype,MPI_Op  op,MPI_Comm  comm)

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
#define COLL_SCATTER_SIG scatter, int, \
                (void *sendbuf, int sendcount, MPI_Datatype sendtype,\
                void *recvbuf, int recvcount, MPI_Datatype recvtype,\
                int root, MPI_Comm comm)

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
 * SCATTER *
 *************/
#define COLL_BARRIER_SIG barrier, int, \
                (MPI_Comm comm)

#define COLL_BARRIERS(action, COLL_sep) \
COLL_APPLY(action, COLL_BARRIER_SIG, default) COLL_sep \
COLL_APPLY(action, COLL_BARRIER_SIG, ompi) COLL_sep \
COLL_APPLY(action, COLL_BARRIER_SIG, ompi_basic_linear) COLL_sep \
COLL_APPLY(action, COLL_BARRIER_SIG, ompi_two_procs)  COLL_sep \
COLL_APPLY(action, COLL_BARRIER_SIG, ompi_tree)  COLL_sep \
COLL_APPLY(action, COLL_BARRIER_SIG, ompi_bruck)  COLL_sep \
COLL_APPLY(action, COLL_BARRIER_SIG, ompi_recursivedoubling) COLL_sep \
COLL_APPLY(action, COLL_BARRIER_SIG, ompi_doublering) COLL_sep \
COLL_APPLY(action, COLL_BARRIER_SIG, mpich)   COLL_sep \
COLL_APPLY(action, COLL_BARRIER_SIG, mvapich2_pair)   COLL_sep \
COLL_APPLY(action, COLL_BARRIER_SIG, mvapich2)   COLL_sep \
COLL_APPLY(action, COLL_BARRIER_SIG, impi)   COLL_sep \
COLL_APPLY(action, COLL_BARRIER_SIG, automatic)

COLL_BARRIERS(COLL_PROTO, COLL_NOsep)


}
}

#endif
