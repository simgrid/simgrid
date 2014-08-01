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

#define MPIR_Allreduce_pt2pt_rd_MV2 smpi_coll_tuned_allreduce_rdb
#define MPIR_Allreduce_pt2pt_rs_MV2 smpi_coll_tuned_allreduce_mvapich2_rs

extern int (*MV2_Allreduce_function)(void *sendbuf,
    void *recvbuf,
    int count,
    MPI_Datatype datatype,
    MPI_Op op, MPI_Comm comm);


extern int (*MV2_Allreduce_intra_function)( void *sendbuf,
    void *recvbuf,
    int count,
    MPI_Datatype datatype,
    MPI_Op op, MPI_Comm comm);
    
static  int MPIR_Allreduce_reduce_p2p_MV2( void *sendbuf,
    void *recvbuf,
    int count,
    MPI_Datatype datatype,
    MPI_Op op, MPI_Comm  comm)
{
  mpi_coll_reduce_fun(sendbuf,recvbuf,count,datatype,op,0,comm);
  return MPI_SUCCESS;
}

static  int MPIR_Allreduce_reduce_shmem_MV2( void *sendbuf,
    void *recvbuf,
    int count,
    MPI_Datatype datatype,
    MPI_Op op, MPI_Comm  comm)
{
  mpi_coll_reduce_fun(sendbuf,recvbuf,count,datatype,op,0,comm);
  return MPI_SUCCESS;
}
    
    
/* general two level allreduce helper function */
int smpi_coll_tuned_allreduce_mvapich2_two_level(void *sendbuf,
                             void *recvbuf,
                             int count,
                             MPI_Datatype datatype,
                             MPI_Op op, MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    int total_size = 0;
    MPI_Aint true_lb, true_extent;
    MPI_Comm shmem_comm = MPI_COMM_NULL, leader_comm = MPI_COMM_NULL;
    int local_rank = -1, local_size = 0;

    //if not set (use of the algo directly, without mvapich2 selector)
    if(MV2_Allreduce_intra_function==NULL)
      MV2_Allreduce_intra_function = smpi_coll_tuned_allreduce_mpich;
    if(MV2_Allreduce_function==NULL)
      MV2_Allreduce_function = smpi_coll_tuned_allreduce_rdb;
    
    if(smpi_comm_get_leaders_comm(comm)==MPI_COMM_NULL){
      smpi_comm_init_smp(comm);
    }
  
    if (count == 0) {
        return MPI_SUCCESS;
    }
    smpi_datatype_extent(datatype, &true_lb,
                                       &true_extent);

    total_size = smpi_comm_size(comm);
    shmem_comm = smpi_comm_get_intra_comm(comm);
    local_rank = smpi_comm_rank(shmem_comm);
    local_size = smpi_comm_size(shmem_comm);

    leader_comm = smpi_comm_get_leaders_comm(comm);

    if (local_rank == 0) {
        if (sendbuf != MPI_IN_PLACE) {
            smpi_datatype_copy(sendbuf, count, datatype, recvbuf,
                                       count, datatype);
        }
    }

    /* Doing the shared memory gather and reduction by the leader */
    if (local_rank == 0) {
        if ((MV2_Allreduce_intra_function == &MPIR_Allreduce_reduce_shmem_MV2) || 
              (MV2_Allreduce_intra_function == &MPIR_Allreduce_reduce_p2p_MV2) ) {
        mpi_errno =
        MV2_Allreduce_intra_function(sendbuf, recvbuf, count, datatype,
                                     op, comm);
        }
        else {
        mpi_errno =
        MV2_Allreduce_intra_function(sendbuf, recvbuf, count, datatype,
                                     op, shmem_comm);
        }

        if (local_size != total_size) {
        void* sendtmpbuf = (char *)xbt_malloc(count*smpi_datatype_get_extent(datatype));
      smpi_datatype_copy(recvbuf, count, datatype,sendtmpbuf, count, datatype);
            /* inter-node allreduce */
            if(MV2_Allreduce_function == &MPIR_Allreduce_pt2pt_rd_MV2){
                mpi_errno =
                    MPIR_Allreduce_pt2pt_rd_MV2(sendtmpbuf, recvbuf, count, datatype, op,
                                      leader_comm);
            } else {
                mpi_errno =
                    MPIR_Allreduce_pt2pt_rs_MV2(sendtmpbuf, recvbuf, count, datatype, op,
                                      leader_comm);
            }
            xbt_free(sendtmpbuf);
        }
    } else {
        /* insert the first reduce here */
        if ((MV2_Allreduce_intra_function == &MPIR_Allreduce_reduce_shmem_MV2) || 
              (MV2_Allreduce_intra_function == &MPIR_Allreduce_reduce_p2p_MV2) ) {
        mpi_errno =
        MV2_Allreduce_intra_function(sendbuf, recvbuf, count, datatype,
                                     op, comm);
        }
        else {
        mpi_errno =
        MV2_Allreduce_intra_function(sendbuf, recvbuf, count, datatype,
                                     op, shmem_comm);
        }
    }

    /* Broadcasting the mesage from leader to the rest */
    /* Note: shared memory broadcast could improve the performance */
    mpi_errno = mpi_coll_bcast_fun(recvbuf, count, datatype, 0, shmem_comm);

    return (mpi_errno);

}
