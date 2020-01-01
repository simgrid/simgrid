/* Copyright (c) 2013-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


#include "../coll_tuned_topo.hpp"
#include "../colls_private.hpp"

namespace simgrid{
namespace smpi{
int barrier__mpich_smp(MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Comm shmem_comm = MPI_COMM_NULL, leader_comm = MPI_COMM_NULL;
    int local_rank = -1;
    
    if(comm->get_leaders_comm()==MPI_COMM_NULL){
      comm->init_smp();
    }

    shmem_comm = comm->get_intra_comm();
    local_rank = shmem_comm->rank();
    /* do the intranode barrier on all nodes */
    if (shmem_comm != NULL) {
        mpi_errno = barrier__mpich(shmem_comm);
        if (mpi_errno) {
          mpi_errno_ret+=mpi_errno;
        }
    }

    leader_comm = comm->get_leaders_comm();
    /* do the barrier across roots of all nodes */
    if (leader_comm != NULL && local_rank == 0) {
        mpi_errno = barrier__mpich(leader_comm);
        if (mpi_errno) {
          mpi_errno_ret+=mpi_errno;
        }
    }

    /* release the local processes on each node with a 1-byte
     * broadcast (0-byte broadcast just returns without doing
     * anything) */
    if (shmem_comm != NULL) {
        int i = 0;
        mpi_errno = bcast__mpich(&i, 1, MPI_BYTE, 0, shmem_comm);
        if (mpi_errno) {
          mpi_errno_ret+=mpi_errno;
        }
    }

    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    return mpi_errno;
}

}
}

