/* Copyright (c) 2013-2022. The SimGrid Team.
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

#define MPIR_Scatter_MV2_Binomial scatter__ompi_binomial
#define MPIR_Scatter_MV2_Direct scatter__ompi_basic_linear

extern int (*MV2_Scatter_intra_function) (const void *sendbuf, int sendcount, MPI_Datatype sendtype,
    void *recvbuf, int recvcount, MPI_Datatype recvtype,
    int root, MPI_Comm comm);

namespace simgrid{
namespace smpi{

int scatter__mvapich2_two_level_direct(const void *sendbuf,
                                       int sendcnt,
                                       MPI_Datatype sendtype,
                                       void *recvbuf,
                                       int recvcnt,
                                       MPI_Datatype recvtype,
                                       int root, MPI_Comm  comm)
{
    int comm_size, rank;
    int local_rank, local_size;
    int leader_comm_rank = -1, leader_comm_size = -1;
    int mpi_errno = MPI_SUCCESS;
    int recvtype_size, sendtype_size, nbytes;
    unsigned char* tmp_buf            = nullptr;
    unsigned char* leader_scatter_buf = nullptr;
    MPI_Status status;
    int leader_root, leader_of_root = -1;
    MPI_Comm shmem_comm, leader_comm;
    //if not set (use of the algo directly, without mvapich2 selector)
    if (MV2_Scatter_intra_function == nullptr)
      MV2_Scatter_intra_function = scatter__mpich;

    if(comm->get_leaders_comm()==MPI_COMM_NULL){
      comm->init_smp();
    }
    comm_size = comm->size();
    rank = comm->rank();

    if (((rank == root) && (recvcnt == 0))
        || ((rank != root) && (sendcnt == 0))) {
        return MPI_SUCCESS;
    }

    /* extract the rank,size information for the intra-node
     * communicator */
    shmem_comm = comm->get_intra_comm();
    local_rank = shmem_comm->rank();
    local_size = shmem_comm->size();

    if (local_rank == 0) {
        /* Node leader. Extract the rank, size information for the leader
         * communicator */
        leader_comm = comm->get_leaders_comm();
        leader_comm_size = leader_comm->size();
        leader_comm_rank = leader_comm->rank();
    }

    if (local_size == comm_size) {
        /* purely intra-node scatter. Just use the direct algorithm and we are done */
        mpi_errno = MPIR_Scatter_MV2_Direct(sendbuf, sendcnt, sendtype,
                                            recvbuf, recvcnt, recvtype,
                                            root, comm);

    } else {
        recvtype_size=recvtype->size();
        sendtype_size=sendtype->size();

        if (rank == root) {
            nbytes = sendcnt * sendtype_size;
        } else {
            nbytes = recvcnt * recvtype_size;
        }

        if (local_rank == 0) {
            /* Node leader, allocate tmp_buffer */
            tmp_buf = smpi_get_tmp_sendbuffer(nbytes * local_size);
        }

        leader_comm = comm->get_leaders_comm();
        int* leaders_map = comm->get_leaders_map();
        leader_of_root = comm->group()->rank(leaders_map[root]);
        leader_root = leader_comm->group()->rank(leaders_map[root]);
        /* leader_root is the rank of the leader of the root in leader_comm.
         * leader_root is to be used as the root of the inter-leader gather ops
         */

        if ((local_rank == 0) && (root != rank)
            && (leader_of_root == rank)) {
            /* The root of the scatter operation is not the node leader. Recv
             * data from the node leader */
            leader_scatter_buf = smpi_get_tmp_sendbuffer(nbytes * comm_size);
            Request::recv(leader_scatter_buf, nbytes * comm_size, MPI_BYTE,
                             root, COLL_TAG_SCATTER, comm, &status);

        }

        if (rank == root && local_rank != 0) {
            /* The root of the scatter operation is not the node leader. Send
             * data to the node leader */
            Request::send(sendbuf, sendcnt * comm_size, sendtype,
                                     leader_of_root, COLL_TAG_SCATTER, comm
                                     );
        }

        if (leader_comm_size > 1 && local_rank == 0) {
          if (not comm->is_uniform()) {
            int* displs   = nullptr;
            int* sendcnts = nullptr;
            int* node_sizes;
            int i      = 0;
            node_sizes = comm->get_non_uniform_map();

            if (root != leader_of_root) {
              if (leader_comm_rank == leader_root) {
                displs      = new int[leader_comm_size];
                sendcnts    = new int[leader_comm_size];
                sendcnts[0] = node_sizes[0] * nbytes;
                displs[0]   = 0;

                for (i = 1; i < leader_comm_size; i++) {
                  displs[i]   = displs[i - 1] + node_sizes[i - 1] * nbytes;
                  sendcnts[i] = node_sizes[i] * nbytes;
                }
              }
              colls::scatterv(leader_scatter_buf, sendcnts, displs, MPI_BYTE, tmp_buf, nbytes * local_size, MPI_BYTE,
                              leader_root, leader_comm);
            } else {
              if (leader_comm_rank == leader_root) {
                displs      = new int[leader_comm_size];
                sendcnts    = new int[leader_comm_size];
                sendcnts[0] = node_sizes[0] * sendcnt;
                displs[0]   = 0;

                for (i = 1; i < leader_comm_size; i++) {
                  displs[i]   = displs[i - 1] + node_sizes[i - 1] * sendcnt;
                  sendcnts[i] = node_sizes[i] * sendcnt;
                }
              }
              colls::scatterv(sendbuf, sendcnts, displs, sendtype, tmp_buf, nbytes * local_size, MPI_BYTE, leader_root,
                              leader_comm);
            }
            if (leader_comm_rank == leader_root) {
              delete[] displs;
              delete[] sendcnts;
            }
            } else {
                if (leader_of_root != root) {
                    mpi_errno =
                        MPIR_Scatter_MV2_Direct(leader_scatter_buf,
                                                nbytes * local_size, MPI_BYTE,
                                                tmp_buf, nbytes * local_size,
                                                MPI_BYTE, leader_root,
                                                leader_comm);
                } else {
                    mpi_errno =
                        MPIR_Scatter_MV2_Direct(sendbuf, sendcnt * local_size,
                                                sendtype, tmp_buf,
                                                nbytes * local_size, MPI_BYTE,
                                                leader_root, leader_comm);

                }
            }
        }
        /* The leaders are now done with the inter-leader part. Scatter the data within the nodes */

        if (rank == root && recvbuf == MPI_IN_PLACE) {
            mpi_errno = MV2_Scatter_intra_function(tmp_buf, nbytes, MPI_BYTE,
                                                (void *)sendbuf, sendcnt, sendtype,
                                                0, shmem_comm);
        } else {
            mpi_errno = MV2_Scatter_intra_function(tmp_buf, nbytes, MPI_BYTE,
                                                recvbuf, recvcnt, recvtype,
                                                0, shmem_comm);
        }
    }

    /* check if multiple threads are calling this collective function */
    if (comm_size != local_size && local_rank == 0) {
        smpi_free_tmp_buffer(tmp_buf);
        if (leader_of_root == rank && root != rank) {
            smpi_free_tmp_buffer(leader_scatter_buf);
        }
    }
    return (mpi_errno);
}


int scatter__mvapich2_two_level_binomial(const void *sendbuf,
                                         int sendcnt,
                                         MPI_Datatype sendtype,
                                         void *recvbuf,
                                         int recvcnt,
                                         MPI_Datatype recvtype,
                                         int root, MPI_Comm comm)
{
    int comm_size, rank;
    int local_rank, local_size;
    int leader_comm_rank = -1, leader_comm_size = -1;
    int mpi_errno = MPI_SUCCESS;
    int recvtype_size, sendtype_size, nbytes;
    unsigned char* tmp_buf            = nullptr;
    unsigned char* leader_scatter_buf = nullptr;
    MPI_Status status;
    int leader_root = -1, leader_of_root = -1;
    MPI_Comm shmem_comm, leader_comm;


    //if not set (use of the algo directly, without mvapich2 selector)
    if (MV2_Scatter_intra_function == nullptr)
      MV2_Scatter_intra_function = scatter__mpich;

    if(comm->get_leaders_comm()==MPI_COMM_NULL){
      comm->init_smp();
    }
    comm_size = comm->size();
    rank = comm->rank();

    if (((rank == root) && (recvcnt == 0))
        || ((rank != root) && (sendcnt == 0))) {
        return MPI_SUCCESS;
    }

    /* extract the rank,size information for the intra-node
     * communicator */
    shmem_comm = comm->get_intra_comm();
    local_rank = shmem_comm->rank();
    local_size = shmem_comm->size();

    if (local_rank == 0) {
        /* Node leader. Extract the rank, size information for the leader
         * communicator */
        leader_comm = comm->get_leaders_comm();
        leader_comm_size = leader_comm->size();
        leader_comm_rank = leader_comm->rank();
    }

    if (local_size == comm_size) {
        /* purely intra-node scatter. Just use the direct algorithm and we are done */
        mpi_errno = MPIR_Scatter_MV2_Direct(sendbuf, sendcnt, sendtype,
                                            recvbuf, recvcnt, recvtype,
                                            root, comm);

    } else {
        recvtype_size=recvtype->size();
        sendtype_size=sendtype->size();

        if (rank == root) {
            nbytes = sendcnt * sendtype_size;
        } else {
            nbytes = recvcnt * recvtype_size;
        }

        if (local_rank == 0) {
            /* Node leader, allocate tmp_buffer */
            tmp_buf = smpi_get_tmp_sendbuffer(nbytes * local_size);
        }
        leader_comm = comm->get_leaders_comm();
        int* leaders_map = comm->get_leaders_map();
        leader_of_root = comm->group()->rank(leaders_map[root]);
        leader_root = leader_comm->group()->rank(leaders_map[root]);
        /* leader_root is the rank of the leader of the root in leader_comm.
         * leader_root is to be used as the root of the inter-leader gather ops
         */

        if ((local_rank == 0) && (root != rank)
            && (leader_of_root == rank)) {
            /* The root of the scatter operation is not the node leader. Recv
             * data from the node leader */
            leader_scatter_buf = smpi_get_tmp_sendbuffer(nbytes * comm_size);
            Request::recv(leader_scatter_buf, nbytes * comm_size, MPI_BYTE,
                             root, COLL_TAG_SCATTER, comm, &status);
        }

        if (rank == root && local_rank != 0) {
            /* The root of the scatter operation is not the node leader. Send
             * data to the node leader */
            Request::send(sendbuf, sendcnt * comm_size, sendtype,
                                     leader_of_root, COLL_TAG_SCATTER, comm);
        }

        if (leader_comm_size > 1 && local_rank == 0) {
          if (not comm->is_uniform()) {
            int* displs   = nullptr;
            int* sendcnts = nullptr;
            int* node_sizes;
            int i      = 0;
            node_sizes = comm->get_non_uniform_map();

            if (root != leader_of_root) {
              if (leader_comm_rank == leader_root) {
                displs      = new int[leader_comm_size];
                sendcnts    = new int[leader_comm_size];
                sendcnts[0] = node_sizes[0] * nbytes;
                displs[0]   = 0;

                for (i = 1; i < leader_comm_size; i++) {
                  displs[i]   = displs[i - 1] + node_sizes[i - 1] * nbytes;
                  sendcnts[i] = node_sizes[i] * nbytes;
                }
              }
              colls::scatterv(leader_scatter_buf, sendcnts, displs, MPI_BYTE, tmp_buf, nbytes * local_size, MPI_BYTE,
                              leader_root, leader_comm);
            } else {
              if (leader_comm_rank == leader_root) {
                displs      = new int[leader_comm_size];
                sendcnts    = new int[leader_comm_size];
                sendcnts[0] = node_sizes[0] * sendcnt;
                displs[0]   = 0;

                for (i = 1; i < leader_comm_size; i++) {
                  displs[i]   = displs[i - 1] + node_sizes[i - 1] * sendcnt;
                  sendcnts[i] = node_sizes[i] * sendcnt;
                }
              }
              colls::scatterv(sendbuf, sendcnts, displs, sendtype, tmp_buf, nbytes * local_size, MPI_BYTE, leader_root,
                              leader_comm);
            }
            if (leader_comm_rank == leader_root) {
              delete[] displs;
              delete[] sendcnts;
            }
            } else {
                if (leader_of_root != root) {
                    mpi_errno =
                        MPIR_Scatter_MV2_Binomial(leader_scatter_buf,
                                                  nbytes * local_size, MPI_BYTE,
                                                  tmp_buf, nbytes * local_size,
                                                  MPI_BYTE, leader_root,
                                                  leader_comm);
                } else {
                    mpi_errno =
                        MPIR_Scatter_MV2_Binomial(sendbuf, sendcnt * local_size,
                                                  sendtype, tmp_buf,
                                                  nbytes * local_size, MPI_BYTE,
                                                  leader_root, leader_comm);

                }
            }
        }
        /* The leaders are now done with the inter-leader part. Scatter the data within the nodes */

        if (rank == root && recvbuf == MPI_IN_PLACE) {
            mpi_errno = MV2_Scatter_intra_function(tmp_buf, nbytes, MPI_BYTE,
                                                (void *)sendbuf, sendcnt, sendtype,
                                                0, shmem_comm);
        } else {
            mpi_errno = MV2_Scatter_intra_function(tmp_buf, nbytes, MPI_BYTE,
                                                recvbuf, recvcnt, recvtype,
                                                0, shmem_comm);
        }

    }


    /* check if multiple threads are calling this collective function */
    if (comm_size != local_size && local_rank == 0) {
        smpi_free_tmp_buffer(tmp_buf);
        if (leader_of_root == rank && root != rank) {
            smpi_free_tmp_buffer(leader_scatter_buf);
        }
    }

    return (mpi_errno);
}

}
}

