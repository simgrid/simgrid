/* Copyright (c) 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2012 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2008      Sun Microsystems, Inc.  All rights reserved.
 * Copyright (c) 2009      University of Houston. All rights reserved.
 *
 * Additional copyrights may follow
 */

/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* Copyright (c) 2001-2014, The Ohio State University. All rights
 * reserved.
 *
 * This file is part of the MVAPICH2 software package developed by the
 * team members of The Ohio State University's Network-Based Computing
 * Laboratory (NBCL), headed by Professor Dhabaleswar K. (DK) Panda.
 *
 * For detailed copyright and licensing information, please refer to the
 * copyright file COPYRIGHT in the top level MVAPICH2 directory.
 *
 */
 
#include "colls_private.h"
extern int mv2_reduce_intra_knomial_factor;
extern int mv2_reduce_inter_knomial_factor;

#define SMPI_DEFAULT_KNOMIAL_FACTOR 4

//        int mv2_reduce_knomial_factor = 2;
        
        
        
static int MPIR_Reduce_knomial_trace(int root, int reduce_knomial_factor,  
        MPI_Comm comm, int *dst, int *expected_send_count,
        int *expected_recv_count, int **src_array)
{
    int mask=0x1, k, comm_size, src, rank, relative_rank, lroot=0;
    int orig_mask=0x1; 
    int recv_iter=0, send_iter=0;
    int *knomial_reduce_src_array=NULL;
    comm_size =  smpi_comm_size(comm);
    rank = smpi_comm_rank(comm);

    lroot = root;
    relative_rank = (rank - lroot + comm_size) % comm_size;

    /* First compute to whom we need to send data */ 
    while (mask < comm_size) {
        if (relative_rank % (reduce_knomial_factor*mask)) {
            *dst = relative_rank/(reduce_knomial_factor*mask)*
                (reduce_knomial_factor*mask)+root;
            if (*dst >= comm_size) {
                *dst -= comm_size;
            }
            send_iter++;
            break;
        }
        mask *= reduce_knomial_factor;
    }
    mask /= reduce_knomial_factor;

    /* Now compute how many children we have in the knomial-tree */ 
    orig_mask = mask; 
    while (mask > 0) {
        for(k=1;k<reduce_knomial_factor;k++) {
            if (relative_rank + mask*k < comm_size) {
                recv_iter++;
            }
        }
        mask /= reduce_knomial_factor;
    }

    /* Finally, fill up the src array */ 
    if(recv_iter > 0) { 
        knomial_reduce_src_array = xbt_malloc(sizeof(int)*recv_iter); 
    } 

    mask = orig_mask; 
    recv_iter=0; 
    while (mask > 0) {
        for(k=1;k<reduce_knomial_factor;k++) {
            if (relative_rank + mask*k < comm_size) {
                src = rank + mask*k;
                if (src >= comm_size) {
                    src -= comm_size;
                }
                knomial_reduce_src_array[recv_iter++] = src;
            }
        }
        mask /= reduce_knomial_factor;
    }

    *expected_recv_count = recv_iter;
    *expected_send_count = send_iter;
    *src_array = knomial_reduce_src_array; 
    return 0; 
}
        
int smpi_coll_tuned_reduce_mvapich2_knomial (
        void *sendbuf,
        void *recvbuf,
        int count,
        MPI_Datatype datatype,
        MPI_Op op,
        int root,
        MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    int rank, is_commutative;
    int src, k;
    MPI_Request send_request;
    int index=0;
    MPI_Aint true_lb, true_extent, extent;
    MPI_Status status; 
    int recv_iter=0, dst=-1, expected_send_count, expected_recv_count;
    int *src_array=NULL;
    void **tmp_buf=NULL;
    MPI_Request *requests=NULL;


    if (count == 0) return MPI_SUCCESS;

    rank = smpi_comm_rank(comm);

    /* Create a temporary buffer */

    smpi_datatype_extent(datatype, &true_lb, &true_extent);
    extent = smpi_datatype_get_extent(datatype);

    is_commutative = smpi_op_is_commute(op);

    if (rank != root) {
        recvbuf=(void *)xbt_malloc(count*(MAX(extent,true_extent)));
        recvbuf = (void *)((char*)recvbuf - true_lb);
    }

    if ((rank != root) || (sendbuf != MPI_IN_PLACE)) {
        mpi_errno = smpi_datatype_copy(sendbuf, count, datatype, recvbuf,
                count, datatype);
    }


    if(mv2_reduce_intra_knomial_factor<0)
      {
        mv2_reduce_intra_knomial_factor = SMPI_DEFAULT_KNOMIAL_FACTOR;
      }
    if(mv2_reduce_inter_knomial_factor<0)
      {
        mv2_reduce_inter_knomial_factor = SMPI_DEFAULT_KNOMIAL_FACTOR;
      }


    MPIR_Reduce_knomial_trace(root, mv2_reduce_intra_knomial_factor, comm, 
           &dst, &expected_send_count, &expected_recv_count, &src_array);

    if(expected_recv_count > 0 ) {
        tmp_buf  = xbt_malloc(sizeof(void *)*expected_recv_count);
        requests = xbt_malloc(sizeof(MPI_Request)*expected_recv_count);
        for(k=0; k < expected_recv_count; k++ ) {
            tmp_buf[k] = xbt_malloc(count*(MAX(extent,true_extent)));
            tmp_buf[k] = (void *)((char*)tmp_buf[k] - true_lb);
        }

        while(recv_iter  < expected_recv_count) {
            src = src_array[expected_recv_count - (recv_iter+1)];

            requests[recv_iter]=smpi_mpi_irecv (tmp_buf[recv_iter], count, datatype ,src,
                    COLL_TAG_REDUCE, comm);
            recv_iter++;

        }

        recv_iter=0;
        while(recv_iter < expected_recv_count) {
            index=smpi_mpi_waitany(expected_recv_count, requests,
                    &status);
            recv_iter++;

            if (is_commutative) {
              smpi_op_apply(op, tmp_buf[index], recvbuf, &count, &datatype);
            }
        }

        for(k=0; k < expected_recv_count; k++ ) {
            xbt_free(tmp_buf[k]);
        }
        xbt_free(tmp_buf);
        xbt_free(requests);
    }

    if(src_array != NULL) { 
        xbt_free(src_array);
    } 

    if(rank != root) {
        send_request=smpi_mpi_isend(recvbuf,count, datatype, dst,
                COLL_TAG_REDUCE,comm);

        smpi_mpi_waitall(1, &send_request, &status);
    }

    /* --END ERROR HANDLING-- */

    return mpi_errno;
}
