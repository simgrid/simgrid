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
#include <algorithm>

#define MPIR_Gather_MV2_Direct gather__ompi_basic_linear
#define MPIR_Gather_MV2_two_level_Direct gather__ompi_basic_linear
#define MPIR_Gather_intra gather__mpich
typedef int (*MV2_Gather_function_ptr) (const void *sendbuf,
    int sendcnt,
    MPI_Datatype sendtype,
    void *recvbuf,
    int recvcnt,
    MPI_Datatype recvtype,
    int root, MPI_Comm comm);

extern MV2_Gather_function_ptr MV2_Gather_inter_leader_function;
extern MV2_Gather_function_ptr MV2_Gather_intra_node_function;

#define TEMP_BUF_HAS_NO_DATA (false)
#define TEMP_BUF_HAS_DATA (true)

namespace simgrid{
namespace smpi{

/* sendbuf           - (in) sender's buffer
 * sendcnt           - (in) sender's element count
 * sendtype          - (in) sender's data type
 * recvbuf           - (in) receiver's buffer
 * recvcnt           - (in) receiver's element count
 * recvtype          - (in) receiver's data type
 * root              - (in)root for the gather operation
 * rank              - (in) global rank(rank in the global comm)
 * tmp_buf           - (out/in) tmp_buf into which intra node
 *                     data is gathered
 * is_data_avail     - (in) based on this, tmp_buf acts
 *                     as in/out parameter.
 *                     1 - tmp_buf acts as in parameter
 *                     0 - tmp_buf acts as out parameter
 * comm_ptr          - (in) pointer to the communicator
 *                     (shmem_comm or intra_sock_comm or
 *                     inter-sock_leader_comm)
 * intra_node_fn_ptr - (in) Function ptr to choose the
 *                      intra node gather function
 * errflag           - (out) to record errors
 */
static int MPIR_pt_pt_intra_gather(const void* sendbuf, int sendcnt, MPI_Datatype sendtype, void* recvbuf, int recvcnt,
                                   MPI_Datatype recvtype, int root, int rank, void* tmp_buf, int nbytes,
                                   bool is_data_avail, MPI_Comm comm, MV2_Gather_function_ptr intra_node_fn_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint recvtype_extent = 0;  /* Datatype extent */
    MPI_Aint true_lb, sendtype_true_extent, recvtype_true_extent;


    if (sendtype != MPI_DATATYPE_NULL) {
        sendtype->extent(&true_lb,
                                       &sendtype_true_extent);
    }
    if (recvtype != MPI_DATATYPE_NULL) {
        recvtype_extent=recvtype->get_extent();
        recvtype->extent(&true_lb,
                                       &recvtype_true_extent);
    }

    /* Special case, when tmp_buf itself has data */
    if (rank == root && sendbuf == MPI_IN_PLACE && is_data_avail) {

         mpi_errno = intra_node_fn_ptr(MPI_IN_PLACE,
                                       sendcnt, sendtype, tmp_buf, nbytes,
                                       MPI_BYTE, 0, comm);

    } else if (rank == root && sendbuf == MPI_IN_PLACE) {
         mpi_errno = intra_node_fn_ptr((char*)recvbuf +
                                       rank * recvcnt * recvtype_extent,
                                       recvcnt, recvtype, tmp_buf, nbytes,
                                       MPI_BYTE, 0, comm);
    } else {
        mpi_errno = intra_node_fn_ptr(sendbuf, sendcnt, sendtype,
                                      tmp_buf, nbytes, MPI_BYTE,
                                      0, comm);
    }

    return mpi_errno;

}



int gather__mvapich2_two_level(const void *sendbuf,
                               int sendcnt,
                               MPI_Datatype sendtype,
                               void *recvbuf,
                               int recvcnt,
                               MPI_Datatype recvtype,
                               int root,
                               MPI_Comm comm)
{
  unsigned char* leader_gather_buf = nullptr;
  int comm_size, rank;
  int local_rank, local_size;
  int leader_comm_rank = -1, leader_comm_size = 0;
  int mpi_errno     = MPI_SUCCESS;
  int recvtype_size = 0, sendtype_size = 0, nbytes = 0;
  int leader_root, leader_of_root;
  MPI_Status status;
  MPI_Aint sendtype_extent = 0, recvtype_extent = 0; /* Datatype extent */
  MPI_Aint true_lb = 0, sendtype_true_extent = 0, recvtype_true_extent = 0;
  MPI_Comm shmem_comm, leader_comm;
  unsigned char* tmp_buf = nullptr;

  // if not set (use of the algo directly, without mvapich2 selector)
  if (MV2_Gather_intra_node_function == nullptr)
    MV2_Gather_intra_node_function = gather__mpich;

  if (comm->get_leaders_comm() == MPI_COMM_NULL) {
    comm->init_smp();
    }
    comm_size = comm->size();
    rank = comm->rank();

    if (((rank == root) && (recvcnt == 0)) ||
        ((rank != root) && (sendcnt == 0))) {
        return MPI_SUCCESS;
    }

    if (sendtype != MPI_DATATYPE_NULL) {
        sendtype_extent=sendtype->get_extent();
        sendtype_size=sendtype->size();
        sendtype->extent(&true_lb,
                                       &sendtype_true_extent);
    }
    if (recvtype != MPI_DATATYPE_NULL) {
        recvtype_extent=recvtype->get_extent();
        recvtype_size=recvtype->size();
        recvtype->extent(&true_lb,
                                       &recvtype_true_extent);
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
        if(leader_comm==MPI_COMM_NULL){
          leader_comm = MPI_COMM_WORLD;
        }
        leader_comm_size = leader_comm->size();
        leader_comm_rank = leader_comm->size();
    }

    if (rank == root) {
        nbytes = recvcnt * recvtype_size;

    } else {
        nbytes = sendcnt * sendtype_size;
    }

#if defined(_SMP_LIMIC_)
     if((g_use_limic2_coll) && (shmem_commptr->ch.use_intra_sock_comm == 1)
         && (use_limic_gather)
         &&((num_scheme == USE_GATHER_PT_PT_BINOMIAL)
            || (num_scheme == USE_GATHER_PT_PT_DIRECT)
            ||(num_scheme == USE_GATHER_PT_LINEAR_BINOMIAL)
            || (num_scheme == USE_GATHER_PT_LINEAR_DIRECT)
            || (num_scheme == USE_GATHER_LINEAR_PT_BINOMIAL)
            || (num_scheme == USE_GATHER_LINEAR_PT_DIRECT)
            || (num_scheme == USE_GATHER_LINEAR_LINEAR)
            || (num_scheme == USE_GATHER_SINGLE_LEADER))) {

            mpi_errno = MV2_Gather_intra_node_function(sendbuf, sendcnt, sendtype,
                                                    recvbuf, recvcnt,recvtype,
                                                    root, comm);
     } else

#endif/*#if defined(_SMP_LIMIC_)*/
    {
        if (local_rank == 0) {
            /* Node leader, allocate tmp_buffer */
            if (rank == root) {
              tmp_buf = smpi_get_tmp_recvbuffer(recvcnt * std::max(recvtype_extent, recvtype_true_extent) * local_size);
            } else {
              tmp_buf = smpi_get_tmp_sendbuffer(sendcnt * std::max(sendtype_extent, sendtype_true_extent) * local_size);
            }
            if (tmp_buf == nullptr) {
              mpi_errno = MPI_ERR_OTHER;
              return mpi_errno;
            }
        }
         /*while testing mpich2 gather test, we see that
         * which basically splits the comm, and we come to
         * a point, where use_intra_sock_comm == 0, but if the
         * intra node function is MPIR_Intra_node_LIMIC_Gather_MV2,
         * it would use the intra sock comm. In such cases, we
         * fallback to binomial as a default case.*/
#if defined(_SMP_LIMIC_)
        if(*MV2_Gather_intra_node_function == MPIR_Intra_node_LIMIC_Gather_MV2) {

            mpi_errno  = MPIR_pt_pt_intra_gather(sendbuf,sendcnt, sendtype,
                                                 recvbuf, recvcnt, recvtype,
                                                 root, rank,
                                                 tmp_buf, nbytes,
                                                 TEMP_BUF_HAS_NO_DATA,
                                                 shmem_commptr,
                                                 MPIR_Gather_intra);
        } else
#endif
        {
            /*We are gathering the data into tmp_buf and the output
             * will be of MPI_BYTE datatype. Since the tmp_buf has no
             * local data, we pass is_data_avail = TEMP_BUF_HAS_NO_DATA*/
            mpi_errno  = MPIR_pt_pt_intra_gather(sendbuf,sendcnt, sendtype,
                                                 recvbuf, recvcnt, recvtype,
                                                 root, rank,
                                                 tmp_buf, nbytes,
                                                 TEMP_BUF_HAS_NO_DATA,
                                                 shmem_comm,
                                                 MV2_Gather_intra_node_function
                                                 );
        }
    }
    leader_comm = comm->get_leaders_comm();
    int* leaders_map = comm->get_leaders_map();
    leader_of_root = comm->group()->rank(leaders_map[root]);
    leader_root = leader_comm->group()->rank(leaders_map[root]);
    /* leader_root is the rank of the leader of the root in leader_comm.
     * leader_root is to be used as the root of the inter-leader gather ops
     */
    if (not comm->is_uniform()) {
      if (local_rank == 0) {
        int* displs   = nullptr;
        int* recvcnts = nullptr;
        int* node_sizes;
        int i = 0;
        /* Node leaders have all the data. But, different nodes can have
         * different number of processes. Do a Gather first to get the
         * buffer lengths at each leader, followed by a Gatherv to move
         * the actual data */

        if (leader_comm_rank == leader_root && root != leader_of_root) {
          /* The root of the Gather operation is not a node-level
           * leader and this process's rank in the leader_comm
           * is the same as leader_root */
          if (rank == root) {
            leader_gather_buf =
                smpi_get_tmp_recvbuffer(recvcnt * std::max(recvtype_extent, recvtype_true_extent) * comm_size);
          } else {
            leader_gather_buf =
                smpi_get_tmp_sendbuffer(sendcnt * std::max(sendtype_extent, sendtype_true_extent) * comm_size);
          }
          if (leader_gather_buf == nullptr) {
            mpi_errno = MPI_ERR_OTHER;
            return mpi_errno;
          }
        }

        node_sizes = comm->get_non_uniform_map();

        if (leader_comm_rank == leader_root) {
          displs   = new int[leader_comm_size];
          recvcnts = new int[leader_comm_size];
        }

        if (root == leader_of_root) {
          /* The root of the gather operation is also the node
           * leader. Receive into recvbuf and we are done */
          if (leader_comm_rank == leader_root) {
            recvcnts[0] = node_sizes[0] * recvcnt;
            displs[0]   = 0;

            for (i = 1; i < leader_comm_size; i++) {
              displs[i]   = displs[i - 1] + node_sizes[i - 1] * recvcnt;
              recvcnts[i] = node_sizes[i] * recvcnt;
            }
          }
          colls::gatherv(tmp_buf, local_size * nbytes, MPI_BYTE, recvbuf, recvcnts, displs, recvtype, leader_root,
                         leader_comm);
        } else {
          /* The root of the gather operation is not the node leader.
           * Receive into leader_gather_buf and then send
           * to the root */
          if (leader_comm_rank == leader_root) {
            recvcnts[0] = node_sizes[0] * nbytes;
            displs[0]   = 0;

            for (i = 1; i < leader_comm_size; i++) {
              displs[i]   = displs[i - 1] + node_sizes[i - 1] * nbytes;
              recvcnts[i] = node_sizes[i] * nbytes;
            }
          }
          colls::gatherv(tmp_buf, local_size * nbytes, MPI_BYTE, leader_gather_buf, recvcnts, displs, MPI_BYTE,
                         leader_root, leader_comm);
        }
        if (leader_comm_rank == leader_root) {
          delete[] displs;
          delete[] recvcnts;
        }
      }
    } else {
        /* All nodes have the same number of processes.
         * Just do one Gather to get all
         * the data at the leader of the root process */
        if (local_rank == 0) {
            if (leader_comm_rank == leader_root && root != leader_of_root) {
                /* The root of the Gather operation is not a node-level leader
                 */
                leader_gather_buf = smpi_get_tmp_sendbuffer(nbytes * comm_size);
                if (leader_gather_buf == nullptr) {
                  mpi_errno = MPI_ERR_OTHER;
                  return mpi_errno;
                }
            }
            if (root == leader_of_root) {
                mpi_errno = MPIR_Gather_MV2_Direct(tmp_buf,
                                                   nbytes * local_size,
                                                   MPI_BYTE, recvbuf,
                                                   recvcnt * local_size,
                                                   recvtype, leader_root,
                                                   leader_comm);

            } else {
                mpi_errno = MPIR_Gather_MV2_Direct(tmp_buf, nbytes * local_size,
                                                   MPI_BYTE, leader_gather_buf,
                                                   nbytes * local_size,
                                                   MPI_BYTE, leader_root,
                                                   leader_comm);
            }
        }
    }
    if ((local_rank == 0) && (root != rank)
        && (leader_of_root == rank)) {
        Request::send(leader_gather_buf,
                                 nbytes * comm_size, MPI_BYTE,
                                 root, COLL_TAG_GATHER, comm);
    }

    if (rank == root && local_rank != 0) {
        /* The root of the gather operation is not the node leader. Receive
         y* data from the node leader */
        Request::recv(recvbuf, recvcnt * comm_size, recvtype,
                                 leader_of_root, COLL_TAG_GATHER, comm,
                                 &status);
    }

    /* check if multiple threads are calling this collective function */
    if (local_rank == 0 ) {
      if (tmp_buf != nullptr) {
        smpi_free_tmp_buffer(tmp_buf);
      }
      if (leader_gather_buf != nullptr) {
        smpi_free_tmp_buffer(leader_gather_buf);
      }
    }

    return (mpi_errno);
}
}
}

