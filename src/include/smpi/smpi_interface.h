/* Copyright (c) 2012-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_INTERFACE_H
#define SMPI_INTERFACE_H
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
                 
/** \ingroup MPI allgather
 *  \brief The list of all available allgather collectives
 */
XBT_PUBLIC_DATA(s_mpi_coll_description_t) mpi_coll_allgather_description[];

/** \ingroup MPI allgather
 *  \brief The list of all available allgather collectives
 */
XBT_PUBLIC_DATA(s_mpi_coll_description_t) mpi_coll_allgatherv_description[];

/** \ingroup MPI allreduce
 *  \brief The list of all available allgather collectives
 */
XBT_PUBLIC_DATA(s_mpi_coll_description_t) mpi_coll_allreduce_description[];

/** \ingroup MPI alltoall
 *  \brief The list of all available alltoall collectives
 */
XBT_PUBLIC_DATA(s_mpi_coll_description_t) mpi_coll_alltoall_description[];

/** \ingroup MPI alltoallv
 *  \brief The list of all available alltoallv collectives
 */
XBT_PUBLIC_DATA(s_mpi_coll_description_t) mpi_coll_alltoallv_description[];

/** \ingroup MPI bcast
 *  \brief The list of all available bcast collectives
 */
XBT_PUBLIC_DATA(s_mpi_coll_description_t) mpi_coll_bcast_description[];

/** \ingroup MPI reduce
 *  \brief The list of all available reduce collectives
 */
XBT_PUBLIC_DATA(s_mpi_coll_description_t) mpi_coll_reduce_description[];

/** \ingroup MPI reduce_scatter
 *  \brief The list of all available allgather collectives
 */
XBT_PUBLIC_DATA(s_mpi_coll_description_t) mpi_coll_reduce_scatter_description[];

/** \ingroup MPI scatter
 *  \brief The list of all available allgather collectives
 */
XBT_PUBLIC_DATA(s_mpi_coll_description_t) mpi_coll_scatter_description[];

/** \ingroup MPI barrier
 *  \brief The list of all available allgather collectives
 */
XBT_PUBLIC_DATA(s_mpi_coll_description_t) mpi_coll_barrier_description[];
XBT_PUBLIC_DATA(int (*mpi_coll_barrier_fun)(MPI_Comm comm));

XBT_PUBLIC(void) coll_help(const char *category, s_mpi_coll_description_t * table);
XBT_PUBLIC(int) find_coll_description(s_mpi_coll_description_t * table, const char *name, const char *desc);

XBT_PUBLIC_DATA(void) (*smpi_coll_cleanup_callback)();
XBT_PUBLIC(void) smpi_coll_cleanup_mvapich2(void);

SG_END_DECL()

#endif
