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
#define MV2_INTRA_SHMEM_REDUCE_MSG 2048

#define mv2_g_shmem_coll_max_msg_size (1 << 17)
#define SHMEM_COLL_BLOCK_SIZE (local_size * mv2_g_shmem_coll_max_msg_size)
#define mv2_use_knomial_reduce 1

#define MPIR_Reduce_inter_knomial_wrapper_MV2 smpi_coll_tuned_reduce_mvapich2_knomial
#define MPIR_Reduce_intra_knomial_wrapper_MV2 smpi_coll_tuned_reduce_mvapich2_knomial
#define MPIR_Reduce_binomial_MV2 smpi_coll_tuned_reduce_binomial
#define MPIR_Reduce_redscat_gather_MV2 smpi_coll_tuned_reduce_scatter_gather
#define MPIR_Reduce_shmem_MV2 smpi_coll_tuned_reduce_ompi_basic_linear

extern int (*MV2_Reduce_function)( void *sendbuf,
    void *recvbuf,
    int count,
    MPI_Datatype datatype,
    MPI_Op op,
    int root,
    MPI_Comm  comm_ptr);

extern int (*MV2_Reduce_intra_function)( void *sendbuf,
    void *recvbuf,
    int count,
    MPI_Datatype datatype,
    MPI_Op op,
    int root,
    MPI_Comm  comm_ptr);


/*Fn pointers for collectives */
static int (*reduce_fn)(void *sendbuf,
                             void *recvbuf,
                             int count,
                             MPI_Datatype datatype,
                             MPI_Op op, int root, MPI_Comm  comm);

int smpi_coll_tuned_reduce_mvapich2_two_level( void *sendbuf,
                                     void *recvbuf,
                                     int count,
                                     MPI_Datatype datatype,
                                     MPI_Op op,
                                     int root,
                                     MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    int my_rank, total_size, local_rank, local_size;
    int leader_comm_rank = -1, leader_comm_size = 0;
    MPI_Comm shmem_comm, leader_comm;
    int leader_root, leader_of_root;
    void *in_buf = NULL, *out_buf = NULL, *tmp_buf = NULL;
    MPI_Aint true_lb, true_extent, extent;
    int is_commutative = 0, stride = 0;
    int intra_node_root=0; 
    
    //if not set (use of the algo directly, without mvapich2 selector)
    if(MV2_Reduce_function==NULL)
      MV2_Reduce_function=smpi_coll_tuned_reduce_mpich;
    if(MV2_Reduce_intra_function==NULL)
      MV2_Reduce_intra_function=smpi_coll_tuned_reduce_mpich;

    if(smpi_comm_get_leaders_comm(comm)==MPI_COMM_NULL){
      smpi_comm_init_smp(comm);
    }
  
    my_rank = smpi_comm_rank(comm);
    total_size = smpi_comm_size(comm);
    shmem_comm = smpi_comm_get_intra_comm(comm);
    local_rank = smpi_comm_rank(shmem_comm);
    local_size = smpi_comm_size(shmem_comm);
    
    leader_comm = smpi_comm_get_leaders_comm(comm);
    int* leaders_map = smpi_comm_get_leaders_map(comm);
    leader_of_root = smpi_group_rank(smpi_comm_group(comm),leaders_map[root]);
    leader_root = smpi_group_rank(smpi_comm_group(leader_comm),leaders_map[root]);

    is_commutative=smpi_op_is_commute(op);

    smpi_datatype_extent(datatype, &true_lb,
                                       &true_extent);
    extent =smpi_datatype_get_extent(datatype);
    stride = count * MAX(extent, true_extent);

    if (local_size == total_size) {
        /* First handle the case where there is only one node */
        if (stride <= MV2_INTRA_SHMEM_REDUCE_MSG &&
            is_commutative == 1) {
            if (local_rank == 0 ) {
                tmp_buf=(void *)xbt_malloc( count *
                                    (MAX(extent, true_extent)));
                tmp_buf = (void *) ((char *) tmp_buf - true_lb);
            }

            if (sendbuf != MPI_IN_PLACE) {
                in_buf = (void *)sendbuf;
            } else {
                in_buf = recvbuf;
            }

            if (local_rank == 0) { 
                 if( my_rank != root) {
                     out_buf = tmp_buf;
                 } else { 
                     out_buf = recvbuf; 
                     if(in_buf == out_buf) { 
                        in_buf = MPI_IN_PLACE; 
                        out_buf = recvbuf; 
                     } 
                 } 
            } else {
                in_buf  = (void *)sendbuf; 
                out_buf = NULL;
            }

	    if (count * (MAX(extent, true_extent)) < SHMEM_COLL_BLOCK_SIZE) {
		mpi_errno = MPIR_Reduce_shmem_MV2(in_buf, out_buf, count,
						  datatype, op,
						  0, shmem_comm);
	    }
	    else {
		mpi_errno = MPIR_Reduce_intra_knomial_wrapper_MV2(in_buf, out_buf, count,
								  datatype, op,
								  0, shmem_comm);
	    }
	    
            if (local_rank == 0 && root != my_rank) {
                smpi_mpi_send(out_buf, count, datatype, root,
                                         COLL_TAG_REDUCE+1, comm);
            }
            if ((local_rank != 0) && (root == my_rank)) {
                smpi_mpi_recv(recvbuf, count, datatype,
                                         leader_of_root, COLL_TAG_REDUCE+1, comm,
                                         MPI_STATUS_IGNORE);
            }
        } else {
            if(mv2_use_knomial_reduce == 1) { 
                reduce_fn = &MPIR_Reduce_intra_knomial_wrapper_MV2; 
            } else { 
                reduce_fn = &MPIR_Reduce_binomial_MV2; 
            } 
            mpi_errno = reduce_fn(sendbuf, recvbuf, count,
                                  datatype, op,
                                  root, comm);
        }
        /* We are done */
        goto fn_exit;
    }
    

    if (local_rank == 0) {
        leader_comm = smpi_comm_get_leaders_comm(comm);
        if(leader_comm==MPI_COMM_NULL){
          leader_comm = MPI_COMM_WORLD;
        }
        leader_comm_size = smpi_comm_size(leader_comm);
        leader_comm_rank = smpi_comm_rank(leader_comm);
        tmp_buf=(void *)xbt_malloc(count *
                            (MAX(extent, true_extent)));
        tmp_buf = (void *) ((char *) tmp_buf - true_lb);
    }
    if (sendbuf != MPI_IN_PLACE) {
        in_buf = (void *)sendbuf;
    } else {
        in_buf = recvbuf;
    }
    if (local_rank == 0) {
        out_buf = tmp_buf;
    } else {
        out_buf = NULL;
    }


    if(local_size > 1) { 
        /* Lets do the intra-node reduce operations, if we have more than one
         * process in the node */

        /*Fix the input and outbuf buffers for the intra-node reduce.
         *Node leaders will have the reduced data in tmp_buf after 
         *this step*/
        if (MV2_Reduce_intra_function == & MPIR_Reduce_shmem_MV2)
        {
            if (is_commutative == 1
		&& (count * (MAX(extent, true_extent)) < SHMEM_COLL_BLOCK_SIZE)) {
                    mpi_errno = MV2_Reduce_intra_function(in_buf, out_buf, count,
                                      datatype, op,
                                      intra_node_root, shmem_comm);
            } else {
                    mpi_errno = MPIR_Reduce_intra_knomial_wrapper_MV2(in_buf, out_buf, count,
                                      datatype, op,
                                      intra_node_root, shmem_comm);
            }
        } else {

            mpi_errno = MV2_Reduce_intra_function(in_buf, out_buf, count,
                                      datatype, op,
                                      intra_node_root, shmem_comm);
        }
    } else { 
        tmp_buf = in_buf; 
    } 

    /* Now work on the inter-leader phase. Data is in tmp_buf */
    if (local_rank == 0 && leader_comm_size > 1) {
        /*The leader of root will have the global reduced data in tmp_buf 
           or recv_buf
           at the end of the reduce */
        if (leader_comm_rank == leader_root) {
            if (my_rank == root) {
                /* I am the root of the leader-comm, and the 
                 * root of the reduce op. So, I will write the 
                 * final result directly into my recvbuf */
                if(tmp_buf != recvbuf) { 
                    in_buf = tmp_buf;
                    out_buf = recvbuf;
                } else { 

                     in_buf = (char *)xbt_malloc(count*
                                       smpi_datatype_get_extent(datatype));
                     smpi_datatype_copy(tmp_buf, count, datatype,
                                        in_buf, count, datatype);
                    //in_buf = MPI_IN_PLACE; 
                    out_buf = recvbuf; 
                } 
            } else {
                in_buf = (char *)xbt_malloc(count*
                                       smpi_datatype_get_extent(datatype));
                smpi_datatype_copy(tmp_buf, count, datatype,
                                        in_buf, count, datatype);
                //in_buf = MPI_IN_PLACE;
                out_buf = tmp_buf;
            }
        } else {
            in_buf = tmp_buf;
            out_buf = NULL;
        }

        /* inter-leader communication  */
        mpi_errno = MV2_Reduce_function(in_buf, out_buf, count,
                              datatype, op,
                              leader_root, leader_comm);

    }

    if (local_size > 1) {
        /* Send the message to the root if the leader is not the
         * root of the reduce operation. The reduced data is in tmp_buf */
        if ((local_rank == 0) && (root != my_rank)
            && (leader_root == leader_comm_rank)) {
            smpi_mpi_send(tmp_buf, count, datatype, root,
                                     COLL_TAG_REDUCE+1, comm);
        }

        if ((local_rank != 0) && (root == my_rank)) {
            smpi_mpi_recv(recvbuf, count, datatype,
                                     leader_of_root,
                                     COLL_TAG_REDUCE+1, comm,
                                     MPI_STATUS_IGNORE);
        }
    }

  fn_exit:
    return mpi_errno;
}
