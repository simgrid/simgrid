#ifndef SMPI_COLLS_H
#define SMPI_COLLS_H

#include <math.h>
#include "smpi/mpi.h"
#include "smpi/private.h"
#include "xbt.h"

#define COLL_DESCRIPTION(cat, ret, args, name) \
  {# name,\
   # cat " " # name " collective",\
   smpi_coll_tuned_ ## cat ## _ ## name}

#define COLL_PROTO(cat, ret, args, name) \
  ret smpi_coll_tuned_ ## cat ## _ ## name(COLL_UNPAREN args);
#define COLL_UNPAREN(...)  __VA_ARGS__

#define COLL_APPLY(action, sig, name) action(sig, name)
#define COLL_COMMA ,
#define COLL_NOsep 
#define COLL_NOTHING(...) 


/*************
 * ALLGATHER *
 *************/
#define COLL_ALLGATHER_SIG allgather, int, \
	                  (void *send_buff, int send_count, MPI_Datatype send_type, \
	                   void *recv_buff, int recv_count, MPI_Datatype recv_type, \
                           MPI_Comm comm)

#define COLL_ALLGATHERS(action, COLL_sep) \
COLL_NOTHING(COLL_APPLY(action, COLL_ALLGATHER_SIG, 2dmesh) COLL_sep) \
COLL_NOTHING(COLL_APPLY(action, COLL_ALLGATHER_SIG, 3dmesh) COLL_sep) \
COLL_NOTHING(COLL_APPLY(action, COLL_ALLGATHER_SIG, bruck) COLL_sep) \
COLL_APPLY(action, COLL_ALLGATHER_SIG, GB) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, loosely_lr) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, lr) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, NTSLR) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, NTSLR_NB) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, pair) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, rdb) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, rhv) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, ring) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, SMP_NTS) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, smp_simple) COLL_sep \
COLL_APPLY(action, COLL_ALLGATHER_SIG, spreading_simple)

COLL_ALLGATHERS(COLL_PROTO, COLL_NOsep)


/*************
 * ALLREDUCE *
 *************/
#define COLL_ALLREDUCE_SIG allreduce, int, \
	                  (void *sbuf, void *rbuf, int rcount, \
                           MPI_Datatype dtype, MPI_Op op, MPI_Comm comm)

#define COLL_ALLREDUCES(action, COLL_sep) \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, lr) COLL_sep \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, NTS) COLL_sep \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, rab1) COLL_sep \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, rab2) COLL_sep \
COLL_NOTHING(COLL_APPLY(action, COLL_ALLREDUCE_SIG, rab_rdb) COLL_sep) \
COLL_NOTHING(COLL_APPLY(action, COLL_ALLREDUCE_SIG, rab_reduce_scatter) COLL_sep) \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, rab_rsag) COLL_sep \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, rdb) COLL_sep \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, smp_binomial) COLL_sep \
COLL_NOTHING(COLL_APPLY(action, COLL_ALLREDUCE_SIG, smp_binomial_pipeline) COLL_sep) \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, smp_rdb) COLL_sep \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, smp_rsag) COLL_sep \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, smp_rsag_lr) COLL_sep \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, smp_rsag_rab) COLL_sep \
COLL_APPLY(action, COLL_ALLREDUCE_SIG, redbcast)

COLL_ALLREDUCES(COLL_PROTO, COLL_NOsep)


/************
 * ALLTOALL *
 ************/
#define COLL_ALLTOALL_SIG alltoall, int, \
	                 (void *send_buff, int send_count, MPI_Datatype send_type, \
	                  void *recv_buff, int recv_count, MPI_Datatype recv_type, \
                          MPI_Comm com)

#define COLL_ALLTOALLS(action, COLL_sep) \
COLL_APPLY(action, COLL_ALLTOALL_SIG, 2dmesh) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALL_SIG, 3dmesh) COLL_sep \
COLL_NOTHING(COLL_APPLY(action, COLL_ALLTOALL_SIG, bruck) COLL_sep) \
COLL_APPLY(action, COLL_ALLTOALL_SIG, pair) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALL_SIG, pair_light_barrier) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALL_SIG, pair_mpi_barrier) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALL_SIG, pair_one_barrier) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALL_SIG, rdb) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALL_SIG, ring) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALL_SIG, ring_light_barrier) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALL_SIG, ring_mpi_barrier) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALL_SIG, ring_one_barrier) COLL_sep \
COLL_APPLY(action, COLL_ALLTOALL_SIG, simple)

COLL_ALLTOALLS(COLL_PROTO, COLL_NOsep)


/*********
 * BCAST *
 *********/
#define COLL_BCAST_SIG bcast, int, \
	              (void *buf, int count, MPI_Datatype datatype, \
	               int root, MPI_Comm comm)

#define COLL_BCASTS(action, COLL_sep) \
COLL_APPLY(action, COLL_BCAST_SIG, arrival_nb) COLL_sep \
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
COLL_APPLY(action, COLL_BCAST_SIG, SMP_linear)

COLL_BCASTS(COLL_PROTO, COLL_NOsep)


/**********
 * REDUCE *
 **********/
#define COLL_REDUCE_SIG reduce, int, \
	               (void *buf, void *rbuf, int count, MPI_Datatype datatype, \
                        MPI_Op op, int root, MPI_Comm comm)

#define COLL_REDUCES(action, COLL_sep) \
COLL_APPLY(action, COLL_REDUCE_SIG, arrival_pattern_aware) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SIG, binomial) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SIG, flat_tree) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SIG, NTSL) COLL_sep \
COLL_APPLY(action, COLL_REDUCE_SIG, scatter_gather)

COLL_REDUCES(COLL_PROTO, COLL_NOsep)

#endif
