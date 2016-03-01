/* Copyright (c) 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SMPI_INTERFACE_H
#define _SMPI_INTERFACE_H
#include "smpi/smpi.h"

SG_BEGIN_DECL()

/** \brief MPI collective description */

typedef struct mpi_coll_description {
  const char *name;
  const char *description;
  void *coll;
} s_mpi_coll_description_t, *mpi_coll_description_t;

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

extern XBT_PRIVATE double smpi_wtime_sleep;
extern XBT_PRIVATE double smpi_iprobe_sleep;
extern XBT_PRIVATE double smpi_test_sleep;

SG_END_DECL()

#endif                          /* _SMPI_INTERFAC_H */
