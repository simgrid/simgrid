/* Copyright (c) 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SMPI_INTERFACE_H
#define _SMPI_INTERFACE_H
#include "smpi/smpi.h"

SG_BEGIN_DECL()

/** \brief MPI collective description */

struct mpi_coll_description {
  const char *name;
  const char *description;
  void *coll;
};
typedef struct mpi_coll_description  s_mpi_coll_description_t;
typedef struct mpi_coll_description* mpi_coll_description_t;

/** \ingroup MPI gather
 *  \brief The list of all available allgather collectives
 */
XBT_PUBLIC_DATA(s_mpi_coll_description_t) mpi_coll_gather_description[];
XBT_PUBLIC_DATA(int (*mpi_coll_gather_fun)(void *, int, MPI_Datatype, void *, int, MPI_Datatype, int, MPI_Comm));
                 
/** \ingroup MPI allgather
 *  \brief The list of all available allgather collectives
 */
XBT_PUBLIC_DATA(s_mpi_coll_description_t) mpi_coll_allgather_description[];
XBT_PUBLIC_DATA(int (*mpi_coll_allgather_fun) (void *, int, MPI_Datatype, void *, int, MPI_Datatype, MPI_Comm));

/** \ingroup MPI allgather
 *  \brief The list of all available allgather collectives
 */
XBT_PUBLIC_DATA(s_mpi_coll_description_t) mpi_coll_allgatherv_description[];
XBT_PUBLIC_DATA(int (*mpi_coll_allgatherv_fun) (void *, int, MPI_Datatype, void *, int*, int*, MPI_Datatype, MPI_Comm));

/** \ingroup MPI allreduce
 *  \brief The list of all available allgather collectives
 */
XBT_PUBLIC_DATA(s_mpi_coll_description_t) mpi_coll_allreduce_description[];
XBT_PUBLIC_DATA(int (*mpi_coll_allreduce_fun)(void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype,MPI_Op op,
                MPI_Comm comm));

/** \ingroup MPI alltoall
 *  \brief The list of all available alltoall collectives
 */
XBT_PUBLIC_DATA(s_mpi_coll_description_t) mpi_coll_alltoall_description[];
XBT_PUBLIC_DATA(int (*mpi_coll_alltoall_fun)(void *, int, MPI_Datatype, void *, int, MPI_Datatype, MPI_Comm));

/** \ingroup MPI alltoallv
 *  \brief The list of all available alltoallv collectives
 */
XBT_PUBLIC_DATA(s_mpi_coll_description_t) mpi_coll_alltoallv_description[];
XBT_PUBLIC_DATA(int (*mpi_coll_alltoallv_fun)(void *, int*, int*, MPI_Datatype, void *, int*, int*, MPI_Datatype,
                MPI_Comm));

/** \ingroup MPI bcast
 *  \brief The list of all available bcast collectives
 */
XBT_PUBLIC_DATA(s_mpi_coll_description_t) mpi_coll_bcast_description[];
XBT_PUBLIC_DATA(int (*mpi_coll_bcast_fun)(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm com));

/** \ingroup MPI reduce
 *  \brief The list of all available reduce collectives
 */
XBT_PUBLIC_DATA(s_mpi_coll_description_t) mpi_coll_reduce_description[];
XBT_PUBLIC_DATA(int (*mpi_coll_reduce_fun)(void *buf, void *rbuf, int count, MPI_Datatype datatype,
                MPI_Op op, int root, MPI_Comm comm));

/** \ingroup MPI reduce_scatter
 *  \brief The list of all available allgather collectives
 */
XBT_PUBLIC_DATA(s_mpi_coll_description_t) mpi_coll_reduce_scatter_description[];
XBT_PUBLIC_DATA(int (*mpi_coll_reduce_scatter_fun) (void *sbuf, void *rbuf, int *rcounts,
                 MPI_Datatype dtype, MPI_Op op,MPI_Comm comm));

/** \ingroup MPI scatter
 *  \brief The list of all available allgather collectives
 */
XBT_PUBLIC_DATA(s_mpi_coll_description_t) mpi_coll_scatter_description[];
XBT_PUBLIC_DATA(int (*mpi_coll_scatter_fun)(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm));

/** \ingroup MPI barrier
 *  \brief The list of all available allgather collectives
 */
XBT_PUBLIC_DATA(s_mpi_coll_description_t) mpi_coll_barrier_description[];
XBT_PUBLIC_DATA(int (*mpi_coll_barrier_fun)(MPI_Comm comm));

XBT_PUBLIC(void) coll_help(const char *category, s_mpi_coll_description_t * table);
XBT_PUBLIC(int) find_coll_description(s_mpi_coll_description_t * table, char *name, const char *desc);

XBT_PUBLIC_DATA(void) (*smpi_coll_cleanup_callback)();
XBT_PUBLIC(void) smpi_coll_cleanup_mvapich2(void);


XBT_PUBLIC (bool) smpi_process_get_replaying(void);

/************************* Tracing********************************************/
XBT_PUBLIC (char *) smpi_container(int rank, char *container, int n);


/************************ Process migration **********************************/
XBT_PUBLIC(void) smpi_replay_process_migrate(smx_actor_t process, sg_host_t new_host, unsigned long size);
XBT_PUBLIC(void) smpi_replay_send_process_data(double data_size, sg_host_t host);
/*****************************************************************************/

/*************************** Actions for SMPI replay *************************/

/*
 * By rktesser: We need  these functions to be public, in order to allow them
 * to be customized or overriden. For this, the custom action should be able to
 * call the original one.  */

XBT_PUBLIC(void) action_init(const char *const *action);
XBT_PUBLIC(void) action_finalize(const char *const *action);
XBT_PUBLIC(void) action_comm_size(const char *const *action);
XBT_PUBLIC(void) action_comm_split(const char *const *action);
XBT_PUBLIC(void) action_comm_dup(const char *const *action);
XBT_PUBLIC(void) action_send(const char *const *action);
XBT_PUBLIC(void) action_Isend(const char *const *action);
XBT_PUBLIC(void) action_Isend(const char *const *action);
XBT_PUBLIC(void) action_recv(const char *const *action);
XBT_PUBLIC(void) action_Irecv(const char *const *action);
XBT_PUBLIC(void) action_Irecv(const char *const *action);
XBT_PUBLIC(void) action_test(const char *const *action);
XBT_PUBLIC(void) action_wait(const char *const *action);
XBT_PUBLIC(void) action_waitall(const char *const *action);
XBT_PUBLIC(void) action_waitall(const char *const *action);
XBT_PUBLIC(void) action_barrier(const char *const *action);
XBT_PUBLIC(void) action_bcast(const char *const *action);
XBT_PUBLIC(void) action_reduce(const char *const *action);
XBT_PUBLIC(void) action_allReduce(const char *const *action);
XBT_PUBLIC(void) action_allReduce(const char *const *action);
XBT_PUBLIC(void) action_allToAll(const char *const *action);
XBT_PUBLIC(void) action_allToAll(const char *const *action);
XBT_PUBLIC(void) action_allToAllv(const char *const *action);
XBT_PUBLIC(void) action_allToAllv(const char *const *action);
XBT_PUBLIC(void) action_gather(const char *const *action);
XBT_PUBLIC(void) action_gatherv(const char *const *action);
XBT_PUBLIC(void) action_gatherv(const char *const *action);
XBT_PUBLIC(void) action_allgather(const char *const *action);
XBT_PUBLIC(void) action_allgather(const char *const *action);
XBT_PUBLIC(void) action_allgatherv(const char *const *action);
XBT_PUBLIC(void) action_allgatherv(const char *const *action);
XBT_PUBLIC(void) action_reducescatter(const char *const *action);
XBT_PUBLIC(void) action_reducescatter(const char *const *action);
XBT_PUBLIC(void) action_compute(const char *const *action);

/* 
 * By rktesser: Armaud wants the LB simulation code to stay outside SimGrid.
 * But we need to add a few new events to the input traces generated by SMPI.
 * Among other things, we need to  implement a few non-standard calls, such as
 * PMPI_Migrate(). For this, we need a few internal smpi calls to be public. 
 */

XBT_PUBLIC(void) smpi_bench_begin();
XBT_PUBLIC(void) smpi_bench_end();
XBT_PUBLIC(void) smpi_mpi_barrier(MPI_Comm comm);
XBT_PUBLIC(double) smpi_process_simulated_elapsed();

/*New function as I can't calll smpi_index_of_smpi_process from outside
 * SimGrid.*/

XBT_PUBLIC(int) smpi_rank_of_smx_process(smx_actor_t process);

SG_END_DECL()

#endif                          /* _SMPI_INTERFAC_H */
