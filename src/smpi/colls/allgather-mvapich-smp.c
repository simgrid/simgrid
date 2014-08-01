/* Copyright (c) 2013-2014. The SimGrid Team.
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
 #include "colls_private.h"



int smpi_coll_tuned_allgather_mvapich2_smp(void *sendbuf,int sendcnt, MPI_Datatype sendtype,
                            void *recvbuf, int recvcnt,MPI_Datatype recvtype,
                            MPI_Comm  comm)
{
    int rank, size;
    int local_rank, local_size;
    int leader_comm_size = 0; 
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint recvtype_extent = 0;  /* Datatype extent */
    MPI_Comm shmem_comm, leader_comm;

  if(smpi_comm_get_leaders_comm(comm)==MPI_COMM_NULL){
    smpi_comm_init_smp(comm);
  }
  
    if(!smpi_comm_is_uniform(comm) || !smpi_comm_is_blocked(comm))
    THROWF(arg_error,0, "allgather MVAPICH2 smp algorithm can't be used with irregular deployment. Please insure that processes deployed on the same node are contiguous and that each node has the same number of processes");
  
    if (recvcnt == 0) {
        return MPI_SUCCESS;
    }

    rank = smpi_comm_rank(comm);
    size = smpi_comm_size(comm);

    /* extract the rank,size information for the intra-node
     * communicator */
    recvtype_extent=smpi_datatype_get_extent(recvtype);
    
    shmem_comm = smpi_comm_get_intra_comm(comm);
    local_rank = smpi_comm_rank(shmem_comm);
    local_size = smpi_comm_size(shmem_comm);

    if (local_rank == 0) {
        /* Node leader. Extract the rank, size information for the leader
         * communicator */
        leader_comm = smpi_comm_get_leaders_comm(comm);
        if(leader_comm==MPI_COMM_NULL){
          leader_comm = MPI_COMM_WORLD;
        }
        leader_comm_size = smpi_comm_size(leader_comm);
    }

    /*If there is just one node, after gather itself,
     * root has all the data and it can do bcast*/
    if(local_rank == 0) {
        mpi_errno = mpi_coll_gather_fun(sendbuf, sendcnt,sendtype, 
                                    (void*)((char*)recvbuf + (rank * recvcnt * recvtype_extent)), 
                                     recvcnt, recvtype,
                                     0, shmem_comm);
    } else {
        /*Since in allgather all the processes could have 
         * its own data in place*/
        if(sendbuf == MPI_IN_PLACE) {
            mpi_errno = mpi_coll_gather_fun((void*)((char*)recvbuf + (rank * recvcnt * recvtype_extent)), 
                                         recvcnt , recvtype, 
                                         recvbuf, recvcnt, recvtype,
                                         0, shmem_comm);
        } else {
            mpi_errno = mpi_coll_gather_fun(sendbuf, sendcnt,sendtype, 
                                         recvbuf, recvcnt, recvtype,
                                         0, shmem_comm);
        }
    }
    /* Exchange the data between the node leaders*/
    if (local_rank == 0 && (leader_comm_size > 1)) {
        /*When data in each socket is different*/
        if (smpi_comm_is_uniform(comm) != 1) {

            int *displs = NULL;
            int *recvcnts = NULL;
            int *node_sizes = NULL;
            int i = 0;

            node_sizes = smpi_comm_get_non_uniform_map(comm);

            displs = xbt_malloc(sizeof (int) * leader_comm_size);
            recvcnts = xbt_malloc(sizeof (int) * leader_comm_size);
            if (!displs || !recvcnts) {
                return MPI_ERR_OTHER;
            }
            recvcnts[0] = node_sizes[0] * recvcnt;
            displs[0] = 0;

            for (i = 1; i < leader_comm_size; i++) {
                displs[i] = displs[i - 1] + node_sizes[i - 1] * recvcnt;
                recvcnts[i] = node_sizes[i] * recvcnt;
            }


            void* sendbuf=((char*)recvbuf)+smpi_datatype_get_extent(recvtype)*displs[smpi_comm_rank(leader_comm)];

            mpi_errno = mpi_coll_allgatherv_fun(sendbuf,
                                       (recvcnt*local_size),
                                       recvtype, 
                                       recvbuf, recvcnts,
                                       displs, recvtype,
                                       leader_comm);
            xbt_free(displs);
            xbt_free(recvcnts);
        } else {
        void* sendtmpbuf=((char*)recvbuf)+smpi_datatype_get_extent(recvtype)*(recvcnt*local_size)*smpi_comm_rank(leader_comm);
        
          

            mpi_errno = smpi_coll_tuned_allgather_mpich(sendtmpbuf, 
                                               (recvcnt*local_size),
                                               recvtype,
                                               recvbuf, (recvcnt*local_size), recvtype,
                                             leader_comm);

        }
    }

    /*Bcast the entire data from node leaders to all other cores*/
    mpi_errno = mpi_coll_bcast_fun (recvbuf, recvcnt * size, recvtype, 0, shmem_comm);
    return mpi_errno;
}
