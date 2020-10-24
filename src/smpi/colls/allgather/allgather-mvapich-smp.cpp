/* Copyright (c) 2013-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2009 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 *
 * Additional copyrights may follow
 */
 /* -*- Mode: C; c-basic-offset:4 ; -*- */
/* Copyright (c) 2001-2014, The Ohio State University. All rights
 * reserved.
 *
 * This file is part of the MVAPICH2 software package developed by the
 * team members of The Ohio State University's Network-Based Computing
 * Laboratory (NBCL), headed by Professor Dhabaleswar K. (DK) Panda.
 *
 * For detailed copyright and licensing information, please refer to the
 * copyright file COPYRIGHT in the top level MVAPICH2 directory.
 */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "../colls_private.hpp"
namespace simgrid{
namespace smpi{

int allgather__mvapich2_smp(const void *sendbuf,int sendcnt, MPI_Datatype sendtype,
                            void *recvbuf, int recvcnt,MPI_Datatype recvtype,
                            MPI_Comm  comm)
{
    int rank, size;
    int local_rank, local_size;
    int leader_comm_size = 0;
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint recvtype_extent = 0;  /* Datatype extent */
    MPI_Comm shmem_comm, leader_comm;

  if(comm->get_leaders_comm()==MPI_COMM_NULL){
    comm->init_smp();
  }

  if (not comm->is_uniform() || not comm->is_blocked())
    throw std::invalid_argument("allgather MVAPICH2 smp algorithm can't be used with irregular deployment. Please "
                                "insure that processes deployed on the same node are contiguous and that each node has "
                                "the same number of processes");

  if (recvcnt == 0) {
    return MPI_SUCCESS;
    }

    rank = comm->rank();
    size = comm->size();

    /* extract the rank,size information for the intra-node communicator */
    recvtype_extent=recvtype->get_extent();

    shmem_comm = comm->get_intra_comm();
    local_rank = shmem_comm->rank();
    local_size = shmem_comm->size();

    if (local_rank == 0) {
        /* Node leader. Extract the rank, size information for the leader communicator */
        leader_comm = comm->get_leaders_comm();
        if(leader_comm==MPI_COMM_NULL){
          leader_comm = MPI_COMM_WORLD;
        }
        leader_comm_size = leader_comm->size();
    }

    /*If there is just one node, after gather itself,
     * root has all the data and it can do bcast*/
    if(local_rank == 0) {
      mpi_errno =
          colls::gather(sendbuf, sendcnt, sendtype, (void*)((char*)recvbuf + (rank * recvcnt * recvtype_extent)),
                        recvcnt, recvtype, 0, shmem_comm);
    } else {
        /*Since in allgather all the processes could have
         * its own data in place*/
        if(sendbuf == MPI_IN_PLACE) {
          mpi_errno = colls::gather((void*)((char*)recvbuf + (rank * recvcnt * recvtype_extent)), recvcnt, recvtype,
                                    recvbuf, recvcnt, recvtype, 0, shmem_comm);
        } else {
          mpi_errno = colls::gather(sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype, 0, shmem_comm);
        }
    }
    /* Exchange the data between the node leaders*/
    if (local_rank == 0 && (leader_comm_size > 1)) {
        /*When data in each socket is different*/
        if (not comm->is_uniform()) {

          int* node_sizes = nullptr;
          int i           = 0;

          node_sizes = comm->get_non_uniform_map();

          int* displs   = new int[leader_comm_size];
          int* recvcnts = new int[leader_comm_size];
          recvcnts[0]   = node_sizes[0] * recvcnt;
          displs[0]     = 0;

          for (i = 1; i < leader_comm_size; i++) {
            displs[i]   = displs[i - 1] + node_sizes[i - 1] * recvcnt;
            recvcnts[i] = node_sizes[i] * recvcnt;
          }

          void* sendtmpbuf = ((char*)recvbuf) + recvtype->get_extent() * displs[leader_comm->rank()];

          mpi_errno = colls::allgatherv(sendtmpbuf, (recvcnt * local_size), recvtype, recvbuf, recvcnts, displs,
                                        recvtype, leader_comm);
          delete[] displs;
          delete[] recvcnts;
        } else {
        void* sendtmpbuf=((char*)recvbuf)+recvtype->get_extent()*(recvcnt*local_size)*leader_comm->rank();



            mpi_errno = allgather__mpich(sendtmpbuf,
                                               (recvcnt*local_size),
                                               recvtype,
                                               recvbuf, (recvcnt*local_size), recvtype,
                                             leader_comm);

        }
    }

    /*Bcast the entire data from node leaders to all other cores*/
    mpi_errno = colls::bcast(recvbuf, recvcnt * size, recvtype, 0, shmem_comm);
    return mpi_errno;
}

}
}
