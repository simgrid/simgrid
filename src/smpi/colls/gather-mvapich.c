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


#define MPIR_Gather_MV2_Direct smpi_coll_tuned_gather_ompi_basic_linear
#define MPIR_Gather_MV2_two_level_Direct smpi_coll_tuned_gather_ompi_basic_linear
#define MPIR_Gather_intra smpi_coll_tuned_gather_mpich
typedef int (*MV2_Gather_function_ptr) (void *sendbuf,
    int sendcnt,
    MPI_Datatype sendtype,
    void *recvbuf,
    int recvcnt,
    MPI_Datatype recvtype,
    int root, MPI_Comm comm);
    
extern MV2_Gather_function_ptr MV2_Gather_inter_leader_function;
extern MV2_Gather_function_ptr MV2_Gather_intra_node_function;

#define TEMP_BUF_HAS_NO_DATA (0)
#define TEMP_BUF_HAS_DATA (1)

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
static int MPIR_pt_pt_intra_gather( void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                            void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                            int root, int rank, 
                            void *tmp_buf, int nbytes,
                            int is_data_avail,
                            MPI_Comm comm,  
                            MV2_Gather_function_ptr intra_node_fn_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint recvtype_extent = 0;  /* Datatype extent */
    MPI_Aint true_lb, sendtype_true_extent, recvtype_true_extent;


    if (sendtype != MPI_DATATYPE_NULL) {
        smpi_datatype_extent(sendtype, &true_lb,
                                       &sendtype_true_extent);
    }
    if (recvtype != MPI_DATATYPE_NULL) {
        recvtype_extent=smpi_datatype_get_extent(recvtype);
        smpi_datatype_extent(recvtype, &true_lb,
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


int smpi_coll_tuned_gather_mvapich2_two_level(void *sendbuf,
                                            int sendcnt,
                                            MPI_Datatype sendtype,
                                            void *recvbuf,
                                            int recvcnt,
                                            MPI_Datatype recvtype,
                                            int root,
                                            MPI_Comm comm)
{
    void *leader_gather_buf = NULL;
    int comm_size, rank;
    int local_rank, local_size;
    int leader_comm_rank = -1, leader_comm_size = 0;
    int mpi_errno = MPI_SUCCESS;
    int recvtype_size = 0, sendtype_size = 0, nbytes=0;
    int leader_root, leader_of_root;
    MPI_Status status;
    MPI_Aint sendtype_extent = 0, recvtype_extent = 0;  /* Datatype extent */
    MPI_Aint true_lb, sendtype_true_extent, recvtype_true_extent;
    MPI_Comm shmem_comm, leader_comm;
    void* tmp_buf;
    

    //if not set (use of the algo directly, without mvapich2 selector)
    if(MV2_Gather_intra_node_function==NULL)
      MV2_Gather_intra_node_function=smpi_coll_tuned_gather_mpich;
    
    if(smpi_comm_get_leaders_comm(comm)==MPI_COMM_NULL){
      smpi_comm_init_smp(comm);
    }
    comm_size = smpi_comm_size(comm);
    rank = smpi_comm_rank(comm);

    if (((rank == root) && (recvcnt == 0)) ||
        ((rank != root) && (sendcnt == 0))) {
        return MPI_SUCCESS;
    }

    if (sendtype != MPI_DATATYPE_NULL) {
        sendtype_extent=smpi_datatype_get_extent(sendtype);
        sendtype_size=smpi_datatype_size(sendtype);
        smpi_datatype_extent(sendtype, &true_lb,
                                       &sendtype_true_extent);
    }
    if (recvtype != MPI_DATATYPE_NULL) {
        recvtype_extent=smpi_datatype_get_extent(recvtype);
        recvtype_size=smpi_datatype_size(recvtype);
        smpi_datatype_extent(recvtype, &true_lb,
                                       &recvtype_true_extent);
    }

    /* extract the rank,size information for the intra-node
     * communicator */
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
        leader_comm_rank = smpi_comm_rank(leader_comm);
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
                tmp_buf = xbt_malloc(recvcnt * MAX(recvtype_extent,
                            recvtype_true_extent) * local_size);
            } else {
                tmp_buf = xbt_malloc(sendcnt * MAX(sendtype_extent,
                            sendtype_true_extent) *
                        local_size);
            }
            if (tmp_buf == NULL) {
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
    leader_comm = smpi_comm_get_leaders_comm(comm);
    int* leaders_map = smpi_comm_get_leaders_map(comm);
    leader_of_root = leaders_map[root];
    leader_root = smpi_group_rank(smpi_comm_group(leader_comm),leader_of_root);
    /* leader_root is the rank of the leader of the root in leader_comm. 
     * leader_root is to be used as the root of the inter-leader gather ops 
     */
    if (!smpi_comm_is_uniform(comm)) {
        if (local_rank == 0) {
            int *displs = NULL;
            int *recvcnts = NULL;
            int *node_sizes;
            int i = 0;
            /* Node leaders have all the data. But, different nodes can have
             * different number of processes. Do a Gather first to get the 
             * buffer lengths at each leader, followed by a Gatherv to move
             * the actual data */

            if (leader_comm_rank == leader_root && root != leader_of_root) {
                /* The root of the Gather operation is not a node-level 
                 * leader and this process's rank in the leader_comm 
                 * is the same as leader_root */
                if(rank == root) { 
                    leader_gather_buf = xbt_malloc(recvcnt *
                                                MAX(recvtype_extent,
                                                recvtype_true_extent) *
                                                comm_size);
                } else { 
                    leader_gather_buf = xbt_malloc(sendcnt *
                                                MAX(sendtype_extent,
                                                sendtype_true_extent) *
                                                comm_size);
                } 
                if (leader_gather_buf == NULL) {
                    mpi_errno =  MPI_ERR_OTHER;
                    return mpi_errno;
                }
            }

            node_sizes = smpi_comm_get_non_uniform_map(comm);

            if (leader_comm_rank == leader_root) {
                displs = xbt_malloc(sizeof (int) * leader_comm_size);
                recvcnts = xbt_malloc(sizeof (int) * leader_comm_size);
                if (!displs || !recvcnts) {
                    mpi_errno = MPI_ERR_OTHER;
                    return mpi_errno;
                }
            }

            if (root == leader_of_root) {
                /* The root of the gather operation is also the node 
                 * leader. Receive into recvbuf and we are done */
                if (leader_comm_rank == leader_root) {
                    recvcnts[0] = node_sizes[0] * recvcnt;
                    displs[0] = 0;

                    for (i = 1; i < leader_comm_size; i++) {
                        displs[i] = displs[i - 1] + node_sizes[i - 1] * recvcnt;
                        recvcnts[i] = node_sizes[i] * recvcnt;
                    }
                } 
                smpi_mpi_gatherv(tmp_buf,
                                         local_size * nbytes,
                                         MPI_BYTE, recvbuf, recvcnts,
                                         displs, recvtype,
                                         leader_root, leader_comm);
            } else {
                /* The root of the gather operation is not the node leader. 
                 * Receive into leader_gather_buf and then send 
                 * to the root */
                if (leader_comm_rank == leader_root) {
                    recvcnts[0] = node_sizes[0] * nbytes;
                    displs[0] = 0;

                    for (i = 1; i < leader_comm_size; i++) {
                        displs[i] = displs[i - 1] + node_sizes[i - 1] * nbytes;
                        recvcnts[i] = node_sizes[i] * nbytes;
                    }
                } 
                smpi_mpi_gatherv(tmp_buf, local_size * nbytes,
                                         MPI_BYTE, leader_gather_buf,
                                         recvcnts, displs, MPI_BYTE,
                                         leader_root, leader_comm);
            }
            if (leader_comm_rank == leader_root) {
                xbt_free(displs);
                xbt_free(recvcnts);
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
                leader_gather_buf = xbt_malloc(nbytes * comm_size);
                if (leader_gather_buf == NULL) {
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
        smpi_mpi_send(leader_gather_buf,
                                 nbytes * comm_size, MPI_BYTE,
                                 root, COLL_TAG_GATHER, comm);
    }

    if (rank == root && local_rank != 0) {
        /* The root of the gather operation is not the node leader. Receive
         y* data from the node leader */
        smpi_mpi_recv(recvbuf, recvcnt * comm_size, recvtype,
                                 leader_of_root, COLL_TAG_GATHER, comm,
                                 &status);
    }

    /* check if multiple threads are calling this collective function */
    if (local_rank == 0 ) {
        if (tmp_buf != NULL) {
            xbt_free(tmp_buf);
        }
        if (leader_gather_buf != NULL) {
            xbt_free(leader_gather_buf);
        }
    }

    return (mpi_errno);
}

#if defined(_SMP_LIMIC_)

static int MPIR_Limic_Gather_Scheme_PT_PT(
                                         const void *sendbuf,int sendcnt, MPI_Datatype sendtype,
                                         void *recvbuf, int recvcnt,MPI_Datatype recvtype,
                                         int root, MPI_Comm comm, 
                                         MV2_Gather_function_ptr intra_node_fn_ptr, 
                                         int *errflag) 
{

    void *intra_tmp_buf = NULL;
    int rank;
    int local_size;
    int mpi_errno = MPI_SUCCESS;
    int recvtype_size = 0, sendtype_size = 0, nbytes=0;
    int sendtype_iscontig;
    int intra_sock_rank=0, intra_sock_comm_size=0;
    int intra_node_leader_rank=0, intra_node_leader_comm_size=0;
    MPI_Aint sendtype_extent = 0, recvtype_extent = 0;  /* Datatype extent */
    MPI_Aint true_lb, sendtype_true_extent, recvtype_true_extent;
    MPI_Comm shmem_comm;
    MPID_Comm *shmem_commptr;
    MPID_Comm *intra_sock_commptr = NULL, *intra_node_leader_commptr=NULL;

    rank = smpi_comm_rank(comm);

    if (((rank == root) && (recvcnt == 0)) ||
            ((rank != root) && (sendcnt == 0))) {
        return MPI_SUCCESS;
    }

    if (sendtype != MPI_DATATYPE_NULL) {
        MPIR_Datatype_iscontig(sendtype, &sendtype_iscontig);
        sendtype_extent=smpi_datatype_extent(sendtype);
        sendtype_size=smpi_datatype_size(sendtype);
        smpi_datatype_extent(sendtype, &true_lb,
                &sendtype_true_extent);
    }
    if (recvtype != MPI_DATATYPE_NULL) {
        recvtype_extent=smpi_datatype_extent(recvtype);
        recvtype_size=smpi_datatype_size(recvtype);
        smpi_datatype_extent(recvtype, &true_lb,
                &recvtype_true_extent);
    }

    /* extract the rank,size information for the intra-node
     * communicator */
    shmem_comm = comm_ptr->ch.shmem_comm;
    MPID_Comm_get_ptr(shmem_comm, shmem_commptr);
    local_size = shmem_commptr->local_size;


    if (rank == root) {
        nbytes = recvcnt * recvtype_size;

    } else {
        nbytes = sendcnt * sendtype_size;
    }

    if(shmem_commptr->ch.use_intra_sock_comm == 1) { 
        MPID_Comm_get_ptr(shmem_commptr->ch.intra_sock_comm, intra_sock_commptr);
        MPID_Comm_get_ptr(shmem_commptr->ch.intra_sock_leader_comm, intra_node_leader_commptr);

        intra_sock_rank = intra_sock_commptr->rank;
        intra_sock_comm_size = intra_sock_commptr->local_size;
        if(intra_sock_rank == 0) { 
            intra_node_leader_rank = intra_node_leader_commptr->rank;
            intra_node_leader_comm_size = intra_node_leader_commptr->local_size;
        }
    }
    if (intra_sock_rank == 0) {
        if (intra_node_leader_rank == 0) {
            /* Node leaders, allocate large buffers which is used to gather
             * data for the entire node. The same buffer is used for inter-node
             * gather as well. This saves us a memcpy operation*/
            if (rank == root) {
                intra_tmp_buf = malloc(recvcnt * MPIR_MAX(recvtype_extent,
                            recvtype_true_extent) * local_size);
            } else {
                intra_tmp_buf = malloc(sendcnt * MPIR_MAX(sendtype_extent,
                            sendtype_true_extent) * local_size);
            }
        } else {

            /* Socket leader, allocate tmp_buffer */
            if (rank == root) {
                intra_tmp_buf = malloc(recvcnt * MPIR_MAX(recvtype_extent,
                            recvtype_true_extent) * intra_sock_comm_size);
            } else {
                intra_tmp_buf = malloc(sendcnt * MPIR_MAX(sendtype_extent,
                            sendtype_true_extent) * intra_sock_comm_size);
            }
        }
        if (intra_tmp_buf == NULL) {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                    MPIR_ERR_RECOVERABLE,
                    FCNAME, __LINE__, MPI_ERR_OTHER,
                    "**nomem", 0);
            return mpi_errno;
        }

    }

    /*Intra socket gather*/
    /*We are gathering the data into intra_tmp_buf and the output
     * will be of MPI_BYTE datatype. Since the tmp_buf has no
     * local data, we pass is_data_avail = TEMP_BUF_HAS_NO_DATA*/
    mpi_errno  = MPIR_pt_pt_intra_gather(sendbuf, sendcnt, sendtype,
                                         recvbuf, recvcnt, recvtype,
                                         root, rank, 
                                         intra_tmp_buf, nbytes,
                                         TEMP_BUF_HAS_NO_DATA,
                                         intra_sock_commptr, 
                                         intra_node_fn_ptr,
                                         errflag);
    if (mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

    /*Inter socket gather*/
    if(intra_sock_rank == 0) {
        /*When data in each socket is different*/
        if (shmem_commptr->ch.is_socket_uniform != 1) {

            int *displs = NULL;
            int *recvcnts = NULL;
            int *socket_sizes;
            int i = 0;
            socket_sizes = shmem_commptr->ch.socket_size;

            if (intra_node_leader_rank == 0) {
                tmp_buf = intra_tmp_buf;

                displs = malloc(sizeof (int) * intra_node_leader_comm_size);
                recvcnts = malloc(sizeof (int) * intra_node_leader_comm_size);
                if (!displs || !recvcnts) {
                    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                            MPIR_ERR_RECOVERABLE,
                            FCNAME, __LINE__,
                            MPI_ERR_OTHER,
                            "**nomem", 0);
                    return mpi_errno;
                }

                recvcnts[0] = socket_sizes[0] * nbytes;
                displs[0] = 0;

                for (i = 1; i < intra_node_leader_comm_size; i++) {
                    displs[i] = displs[i - 1] + socket_sizes[i - 1] * nbytes;
                    recvcnts[i] = socket_sizes[i] * nbytes;
                }

                mpi_errno = MPIR_Gatherv(MPI_IN_PLACE,
                                         intra_sock_comm_size * nbytes,
                                         MPI_BYTE, tmp_buf, recvcnts,
                                         displs, MPI_BYTE,
                                         0, intra_node_leader_commptr,
                                         errflag);

                /*Free the displacement and recvcnts buffer*/
                free(displs);
                free(recvcnts);
            } else {
                mpi_errno = MPIR_Gatherv(intra_tmp_buf,
                                         intra_sock_comm_size * nbytes,
                                         MPI_BYTE, tmp_buf, recvcnts,
                                         displs, MPI_BYTE,
                                         0, intra_node_leader_commptr, 
                                         errflag);

            }

        } else {

            if (intra_node_leader_rank == 0) {
                tmp_buf = intra_tmp_buf;

                /*We have now completed the intra_sock gather and all the 
                 * socket level leaders have data in their tmp_buf. So we 
                 * set sendbuf = MPI_IN_PLACE and also explicity set the
                 * is_data_avail= TEMP_BUF_HAS_DATA*/
                mpi_errno  = MPIR_pt_pt_intra_gather(MPI_IN_PLACE, 
                                                     (nbytes*intra_sock_comm_size), 
                                                     MPI_BYTE,
                                                     recvbuf, recvcnt, recvtype,
                                                     root, rank, 
                                                     tmp_buf,  
                                                     (nbytes*intra_sock_comm_size),
                                                     TEMP_BUF_HAS_DATA, 
                                                     intra_node_leader_commptr,              
                                                     intra_node_fn_ptr, 
                                                     errflag);
            } else {

                /*After the intra_sock gather, all the node level leaders
                 * have the data in intra_tmp_buf(sendbuf) and this is gathered into
                 * tmp_buf. Since the tmp_buf(in non-root processes) does not have 
                 * the data in tmp_buf is_data_avail = TEMP_BUF_HAS_NO_DATA*/
                mpi_errno  = MPIR_pt_pt_intra_gather(intra_tmp_buf, 
                                                    (nbytes*intra_sock_comm_size), 
                                                    MPI_BYTE,
                                                    recvbuf, recvcnt, recvtype,
                                                    root, rank,
                                                    tmp_buf,
                                                    (nbytes*intra_sock_comm_size),
                                                    TEMP_BUF_HAS_NO_DATA,
                                                    intra_node_leader_commptr,
                                                    intra_node_fn_ptr,
                                                    errflag);
            }

        }

        if (mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }
    }
fn_fail:
    /*Free the intra socket leader buffers*/
    if (intra_sock_rank == 0) {
        if ((intra_node_leader_rank != 0) && (intra_tmp_buf != NULL)) {
            free(intra_tmp_buf);
        }
    }

    return (mpi_errno);
}

static int MPIR_Limic_Gather_Scheme_PT_LINEAR(
                                         const void *sendbuf,int sendcnt, MPI_Datatype sendtype,
                                         void *recvbuf, int recvcnt,MPI_Datatype recvtype,
                                         int root, MPI_Comm comm, 
                                         MV2_Gather_function_ptr intra_node_fn_ptr, 
                                         int *errflag) 
{
    void *intra_tmp_buf = NULL;
    void *local_sendbuf=NULL;
    int rank;
    int local_rank, local_size;
    int mpi_errno = MPI_SUCCESS;
    int recvtype_size = 0, nbytes=0;
    int sendtype_iscontig;
    int intra_sock_rank=0, intra_sock_comm_size=0;
    int intra_node_leader_rank=0, intra_node_leader_comm_size=0;
    int send_nbytes=0;
    MPI_Aint recvtype_extent = 0;  /* Datatype extent */
    MPI_Aint true_lb, sendtype_true_extent, recvtype_true_extent;
    MPI_Comm shmem_comm;
    MPID_Comm *shmem_commptr;
    MPID_Comm *intra_sock_commptr = NULL, *intra_node_leader_commptr=NULL;
    MPI_Aint position = 0;
    MPI_Aint sendtype_size = 0;

    rank = smpi_comm_rank(comm);

    if (((rank == root) && (recvcnt == 0)) ||
            ((rank != root) && (sendcnt == 0))) {
        return MPI_SUCCESS;
    }

    if (sendtype != MPI_DATATYPE_NULL) {
        //MPIR_Datatype_iscontig(sendtype, &sendtype_iscontig);
        sendtype_size=smpi_datatype_size(sendtype);
        smpi_datatype_extent(sendtype, &true_lb,
                &sendtype_true_extent);
    }
    if (recvtype != MPI_DATATYPE_NULL) {
        recvtype_extent=smpi_datatype_extent(recvtype);
        recvtype_size=smpi_datatype_size(recvtype);
        smpi_datatype_extent(recvtype, &true_lb,
                &recvtype_true_extent);
    }

    /* extract the rank,size information for the intra-node
     * communicator */
    shmem_comm = comm_ptr->ch.shmem_comm;
    MPID_Comm_get_ptr(shmem_comm, shmem_commptr);
    local_rank = shmem_commptr->rank;
    local_size = shmem_commptr->local_size;


    if (rank == root) {
        nbytes = recvcnt * recvtype_size;

    } else {
        nbytes = sendcnt * sendtype_size;
    }

    if(shmem_commptr->ch.use_intra_sock_comm == 1) { 
        MPID_Comm_get_ptr(shmem_commptr->ch.intra_sock_comm, intra_sock_commptr);
        MPID_Comm_get_ptr(shmem_commptr->ch.intra_sock_leader_comm, intra_node_leader_commptr);

        intra_sock_rank = intra_sock_commptr->rank;
        intra_sock_comm_size = intra_sock_commptr->local_size;
        if(intra_sock_rank == 0) { 
            intra_node_leader_rank = intra_node_leader_commptr->rank;
            intra_node_leader_comm_size = intra_node_leader_commptr->local_size;
        }    
    }
    /*Pack data for non-contiguous buffer*/
   /* if ((!sendtype_iscontig) && (sendbuf != MPI_IN_PLACE)) {
        MPIR_Pack_size_impl(1, sendtype, &sendtype_size);
        send_nbytes= sendcnt * sendtype_size;
        MPIU_CHKLMEM_MALLOC(local_sendbuf, void *, send_nbytes, mpi_errno, "local_sendbuf");
        MPIR_Pack_impl(sendbuf, sendcnt, sendtype, local_sendbuf, send_nbytes, &position);
    } else {*/
        local_sendbuf = (void *)sendbuf;
        send_nbytes=nbytes;
   // }


    if (intra_sock_rank == 0) {
        if (intra_node_leader_rank == 0) {
            /* Node leaders, allocate large buffers which is used to gather
             * data for the entire node. The same buffer is used for inter-node
             * gather as well. This saves us a memcpy operation*/
            if (rank == root) {
                intra_tmp_buf = malloc(recvcnt * MPIR_MAX(recvtype_extent,
                            recvtype_true_extent) * local_size);
            } else {
                intra_tmp_buf = malloc(send_nbytes * local_size);
            }

        } else {

            /* Socket leader, allocate tmp_buffer */
            if (rank == root) {
                intra_tmp_buf = malloc(recvcnt * MPIR_MAX(recvtype_extent,
                            recvtype_true_extent) * intra_sock_comm_size);
            } else {
                intra_tmp_buf = malloc(send_nbytes * intra_sock_comm_size);
            }

        }

        if (intra_tmp_buf == NULL) {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                    MPIR_ERR_RECOVERABLE,
                    FCNAME, __LINE__, MPI_ERR_OTHER,
                    "**nomem", 0);
            return mpi_errno;
        }

        /*Local copy of buffer*/
        if(sendbuf != MPI_IN_PLACE) {
            memcpy(intra_tmp_buf, local_sendbuf, send_nbytes);
        } else {
            MPIR_Localcopy(((char *) recvbuf +rank * recvcnt * recvtype_extent),
                           recvcnt, recvtype,
                           intra_tmp_buf, send_nbytes, MPI_BYTE);
       }
    }

    if(local_rank !=0 && sendbuf == MPI_IN_PLACE) {
        mpi_errno = MPIR_Limic_Gather_OSU(intra_tmp_buf, 
                                          (intra_sock_comm_size * send_nbytes), 
                                          (recvbuf + (rank*nbytes)), nbytes,
                                          intra_sock_commptr );
    } else {
        mpi_errno = MPIR_Limic_Gather_OSU(intra_tmp_buf, 
                                          (intra_sock_comm_size * send_nbytes), 
                                          local_sendbuf, send_nbytes, 
                                          intra_sock_commptr );
    }
    if (mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

    /*Inter socket gather*/
    if(intra_sock_rank == 0) {
        /*When data in each socket is different*/
        if (shmem_commptr->ch.is_socket_uniform != 1) {

            int *displs = NULL;
            int *recvcnts = NULL;
            int *socket_sizes;
            int i = 0;
            socket_sizes = shmem_commptr->ch.socket_size;

            if (intra_node_leader_rank == 0) {
                tmp_buf = intra_tmp_buf;

                displs = malloc(sizeof (int) * intra_node_leader_comm_size);
                recvcnts = malloc(sizeof (int) * intra_node_leader_comm_size);
                if (!displs || !recvcnts) {
                    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                            MPIR_ERR_RECOVERABLE,
                            FCNAME, __LINE__,
                            MPI_ERR_OTHER,
                            "**nomem", 0);
                    return mpi_errno;
                }

                recvcnts[0] = socket_sizes[0] * nbytes;
                displs[0] = 0;

                for (i = 1; i < intra_node_leader_comm_size; i++) {
                    displs[i] = displs[i - 1] + socket_sizes[i - 1] * nbytes;
                    recvcnts[i] = socket_sizes[i] * nbytes;
                }


                mpi_errno = MPIR_Gatherv(MPI_IN_PLACE,
                                         intra_sock_comm_size * nbytes,
                                         MPI_BYTE, tmp_buf, recvcnts,
                                         displs, MPI_BYTE,
                                         0, intra_node_leader_commptr, 
                                         errflag);

                /*Free the displacement and recvcnts buffer*/
                free(displs);
                free(recvcnts);

            } else {
                mpi_errno = MPIR_Gatherv(intra_tmp_buf,
                                         intra_sock_comm_size * nbytes,
                                         MPI_BYTE, tmp_buf, recvcnts,
                                         displs, MPI_BYTE,
                                         0, intra_node_leader_commptr, 
                                         errflag);

            }
        } else {

            if (intra_node_leader_rank == 0) {
                tmp_buf = intra_tmp_buf;

                /*We have now completed the intra_sock gather and all the 
                 * socket level leaders have data in their tmp_buf. So we 
                 * set sendbuf = MPI_IN_PLACE and also explicity set the
                 * is_data_avail= TEMP_BUF_HAS_DATA*/
                mpi_errno  = MPIR_pt_pt_intra_gather(MPI_IN_PLACE, 
                                                     (send_nbytes*intra_sock_comm_size), 
                                                     MPI_BYTE,
                                                     recvbuf, recvcnt, recvtype,
                                                     root, rank, 
                                                     tmp_buf, 
                                                     (send_nbytes*intra_sock_comm_size),
                                                     TEMP_BUF_HAS_DATA, 
                                                     intra_node_leader_commptr,
                                                     intra_node_fn_ptr,
                                                     errflag);
            } else {

                /*After the intra_sock gather, all the node level leaders
                 * have the data in intra_tmp_buf(sendbuf) and this is gathered into
                 * tmp_buf. Since the tmp_buf(in non-root processes) does not have 
                 * the data in tmp_buf is_data_avail = TEMP_BUF_HAS_NO_DATA*/
                mpi_errno  = MPIR_pt_pt_intra_gather(intra_tmp_buf, 
                                                     (send_nbytes*intra_sock_comm_size),
                                                     MPI_BYTE,
                                                     recvbuf, recvcnt, recvtype,
                                                     root, rank, 
                                                     tmp_buf, 
                                                     (send_nbytes*intra_sock_comm_size),
                                                     TEMP_BUF_HAS_NO_DATA, 
                                                     intra_node_leader_commptr,
                                                     intra_node_fn_ptr,
                                                     errflag);
            }
        }

        if (mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }
    }
fn_fail:
    /*Free the intra socket leader buffers*/
    if (intra_sock_rank == 0) { 
        if ((intra_node_leader_rank != 0) && (intra_tmp_buf != NULL)) {
            free(intra_tmp_buf);
        }
    }
    MPIU_CHKLMEM_FREEALL();
    return (mpi_errno);
}

static int MPIR_Limic_Gather_Scheme_LINEAR_PT(
                                         const void *sendbuf,int sendcnt, MPI_Datatype sendtype,
                                         void *recvbuf, int recvcnt,MPI_Datatype recvtype,
                                         int root, MPI_Comm comm, 
                                         MV2_Gather_function_ptr intra_node_fn_ptr, 
                                         int *errflag) 
{
    void *intra_tmp_buf = NULL;
    int rank;
    int local_size;
    int mpi_errno = MPI_SUCCESS;
    int recvtype_size = 0, sendtype_size = 0, nbytes=0;
    int sendtype_iscontig;
    int intra_sock_rank=0, intra_sock_comm_size=0;
    int intra_node_leader_rank=0;
    MPI_Aint sendtype_extent = 0, recvtype_extent = 0;  /* Datatype extent */
    MPI_Aint true_lb, sendtype_true_extent, recvtype_true_extent;
    MPI_Comm shmem_comm;
    MPID_Comm *shmem_commptr;
    MPID_Comm *intra_sock_commptr = NULL, *intra_node_leader_commptr=NULL;

    rank = smpi_comm_rank(comm);

    if (((rank == root) && (recvcnt == 0)) ||
            ((rank != root) && (sendcnt == 0))) {
        return MPI_SUCCESS;
    }

    if (sendtype != MPI_DATATYPE_NULL) {
        //MPIR_Datatype_iscontig(sendtype, &sendtype_iscontig);
        sendtype_extent=smpi_datatype_extent(sendtype);
        sendtype_size=smpi_datatype_size(sendtype);
        smpi_datatype_extent(sendtype, &true_lb,
                &sendtype_true_extent);
    }
    if (recvtype != MPI_DATATYPE_NULL) {
        recvtype_extent=smpi_datatype_extent(recvtype);
        recvtype_size=smpi_datatype_size(recvtype);
        smpi_datatype_extent(recvtype, &true_lb,
                &recvtype_true_extent);
    }

    /* extract the rank,size information for the intra-node
     * communicator */
    shmem_comm = comm_ptr->ch.shmem_comm;
    MPID_Comm_get_ptr(shmem_comm, shmem_commptr);
    local_size = shmem_commptr->local_size;


    if (rank == root) {
        nbytes = recvcnt * recvtype_size;

    } else {
        nbytes = sendcnt * sendtype_size;
    }

    if(shmem_commptr->ch.use_intra_sock_comm == 1) { 
        MPID_Comm_get_ptr(shmem_commptr->ch.intra_sock_comm, intra_sock_commptr);
        MPID_Comm_get_ptr(shmem_commptr->ch.intra_sock_leader_comm, intra_node_leader_commptr);

        intra_sock_rank = intra_sock_commptr->rank;
        intra_sock_comm_size = intra_sock_commptr->local_size;
        if(intra_sock_rank == 0) { 
            intra_node_leader_rank = intra_node_leader_commptr->rank;
        }    
    }

    if (intra_sock_rank == 0) {
        if (intra_node_leader_rank == 0) {
            /* Node leaders, allocate large buffers which is used to gather
             * data for the entire node. The same buffer is used for inter-node
             * gather as well. This saves us a memcpy operation*/
            if (rank == root) {
                intra_tmp_buf = malloc(recvcnt * MPIR_MAX(recvtype_extent,
                            recvtype_true_extent) * local_size);
            } else {
                intra_tmp_buf = malloc(sendcnt * MPIR_MAX(sendtype_extent,
                            sendtype_true_extent) * local_size);
            } 
        } else {

            /* Socket leader, allocate tmp_buffer */
            if (rank == root) {
                intra_tmp_buf = malloc(recvcnt * MPIR_MAX(recvtype_extent,
                            recvtype_true_extent) * intra_sock_comm_size);
            } else {
                intra_tmp_buf = malloc(sendcnt * MPIR_MAX(sendtype_extent,
                            sendtype_true_extent) * intra_sock_comm_size);
            }
        }
        if (intra_tmp_buf == NULL) {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                    MPIR_ERR_RECOVERABLE,
                    FCNAME, __LINE__, MPI_ERR_OTHER,
                    "**nomem", 0);
            return mpi_errno;
        }
    }

    /*Intra socket gather*/
    /*We are gathering the data into intra_tmp_buf and the output
     * will be of MPI_BYTE datatype. Since the tmp_buf has no
     * local data, we pass is_data_avail = TEMP_BUF_HAS_NO_DATA*/
    mpi_errno  = MPIR_pt_pt_intra_gather(sendbuf, sendcnt, sendtype,
                                         recvbuf, recvcnt, recvtype,
                                         root, rank, 
                                         intra_tmp_buf, nbytes, 
                                         TEMP_BUF_HAS_NO_DATA,
                                         intra_sock_commptr,
                                         intra_node_fn_ptr,
                                         errflag);

    if (mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

    /*Inter socket gather*/
    if(intra_sock_rank == 0) {
        if (intra_node_leader_rank == 0) {
            tmp_buf = intra_tmp_buf;
        }
        mpi_errno = MPIR_Limic_Gather_OSU(tmp_buf, (local_size * nbytes), 
                                          intra_tmp_buf, 
                                          (intra_sock_comm_size * nbytes), 
                                          intra_node_leader_commptr);
    }

    if (mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }
fn_fail:
    /*Free the intra socket leader buffers*/
    if (intra_sock_rank == 0) { 
        if ((intra_node_leader_rank != 0) && (intra_tmp_buf != NULL)) {
            free(intra_tmp_buf);
        }
    }

    return (mpi_errno);
}

#undef FUNCNAME
#define FUNCNAME MPIR_Limic_Gather_Scheme_LINEAR_LINEAR
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int MPIR_Limic_Gather_Scheme_LINEAR_LINEAR(
                                         const void *sendbuf,int sendcnt, MPI_Datatype sendtype,
                                         void *recvbuf, int recvcnt,MPI_Datatype recvtype,
                                         int root, MPI_Comm comm, 
                                         int *errflag) 
{
    void *intra_tmp_buf = NULL;
    void *local_sendbuf=NULL;
    int rank;
    int local_rank, local_size;
    int mpi_errno = MPI_SUCCESS;
    int recvtype_size = 0, nbytes=0;
    int sendtype_iscontig;
    int intra_sock_rank=0, intra_sock_comm_size=0;
    int intra_node_leader_rank=0;
    int send_nbytes=0;
    MPI_Aint recvtype_extent = 0;  /* Datatype extent */
    MPI_Aint true_lb, sendtype_true_extent, recvtype_true_extent;
    MPI_Comm shmem_comm;
    MPID_Comm *shmem_commptr;
    MPID_Comm *intra_sock_commptr = NULL, *intra_node_leader_commptr=NULL;
    MPI_Aint sendtype_size = 0;
    MPI_Aint position = 0;
    MPIU_CHKLMEM_DECL(1);

    rank = smpi_comm_rank(comm);

    if (((rank == root) && (recvcnt == 0)) ||
            ((rank != root) && (sendcnt == 0))) {
        return MPI_SUCCESS;
    }

    if (sendtype != MPI_DATATYPE_NULL) {
        MPIR_Datatype_iscontig(sendtype, &sendtype_iscontig);
        sendtype_size=smpi_datatype_size(sendtype);
        smpi_datatype_extent(sendtype, &true_lb,
                &sendtype_true_extent);
    }
    if (recvtype != MPI_DATATYPE_NULL) {
        recvtype_extent=smpi_datatype_extent(recvtype);
        recvtype_size=smpi_datatype_size(recvtype);
        smpi_datatype_extent(recvtype, &true_lb,
                &recvtype_true_extent);
    }

    /* extract the rank,size information for the intra-node
     * communicator */
    shmem_comm = comm_ptr->ch.shmem_comm;
    MPID_Comm_get_ptr(shmem_comm, shmem_commptr);
    local_rank = shmem_commptr->rank;
    local_size = shmem_commptr->local_size;


    if (rank == root) {
        nbytes = recvcnt * recvtype_size;

    } else {
        nbytes = sendcnt * sendtype_size;
    }

    if(shmem_commptr->ch.use_intra_sock_comm == 1) { 
        MPID_Comm_get_ptr(shmem_commptr->ch.intra_sock_comm, intra_sock_commptr);
        MPID_Comm_get_ptr(shmem_commptr->ch.intra_sock_leader_comm, intra_node_leader_commptr);

        intra_sock_rank = intra_sock_commptr->rank;
        intra_sock_comm_size = intra_sock_commptr->local_size;
        if(intra_sock_rank == 0) { 
            intra_node_leader_rank = intra_node_leader_commptr->rank;
        }    
    }
    
    /*Pack data for non-contiguous buffer*/
   /* if ((!sendtype_iscontig) && (sendbuf != MPI_IN_PLACE)) {

        MPIR_Pack_size_impl(1, sendtype, &sendtype_size);
        send_nbytes= sendcnt * sendtype_size;
        MPIU_CHKLMEM_MALLOC(local_sendbuf, void *, send_nbytes, mpi_errno, "local_sendbuf");
        MPIR_Pack_impl(sendbuf, sendcnt, sendtype, local_sendbuf, send_nbytes, &position);
    }
    else {*/
        local_sendbuf = (void *)sendbuf;
        send_nbytes = nbytes;
   // }

    if (intra_sock_rank == 0) {
        if (intra_node_leader_rank == 0) {
            /* Node leaders, allocate large buffers which is used to gather
             * data for the entire node. The same buffer is used for inter-node
             * gather as well. This saves us a memcpy operation*/
            if (rank == root) {
                intra_tmp_buf = malloc(recvcnt * MPIR_MAX(recvtype_extent,
                            recvtype_true_extent) * local_size);
            } else {
                intra_tmp_buf = malloc(send_nbytes * local_size);
            }

        } else {

            /* Socket leader, allocate tmp_buffer */
            if (rank == root) {
                intra_tmp_buf = malloc(recvcnt * MPIR_MAX(recvtype_extent,
                            recvtype_true_extent) * intra_sock_comm_size);
            } else {
                intra_tmp_buf = malloc(send_nbytes * intra_sock_comm_size);
            }

        }
        if (intra_tmp_buf == NULL) {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                    MPIR_ERR_RECOVERABLE,
                    FCNAME, __LINE__, MPI_ERR_OTHER,
                    "**nomem", 0);
            return mpi_errno;
        }

        /*Local copy of buffer*/
        if(sendbuf != MPI_IN_PLACE) {
            memcpy(intra_tmp_buf, local_sendbuf, send_nbytes);
        } else {
            MPIR_Localcopy(((char *) recvbuf +rank * recvcnt * recvtype_extent),
                           recvcnt, recvtype,
                           intra_tmp_buf, send_nbytes, MPI_BYTE);
        }
    }


    if(local_rank !=0 && sendbuf == MPI_IN_PLACE) {
        mpi_errno = MPIR_Limic_Gather_OSU(intra_tmp_buf,  
                                         (intra_sock_comm_size * send_nbytes), 
                                         (recvbuf + (rank*nbytes)), nbytes,
                                         intra_sock_commptr);
    } else {
        mpi_errno = MPIR_Limic_Gather_OSU(intra_tmp_buf, 
                                          (intra_sock_comm_size * send_nbytes), 
                                          local_sendbuf, send_nbytes, 
                                          intra_sock_commptr );
    }
    if (mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }
    
    /*Inter socket gather*/
    if(intra_sock_rank == 0) {
        if (intra_node_leader_rank == 0) {
            tmp_buf = intra_tmp_buf;
        }
        mpi_errno = MPIR_Limic_Gather_OSU(tmp_buf, (local_size * send_nbytes), 
                                          intra_tmp_buf, 
                                          (intra_sock_comm_size * send_nbytes), 
                                          intra_node_leader_commptr );
    }

    if (mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }
fn_fail:
    /*Free the intra socket leader buffers*/
    if (intra_sock_rank == 0) { 
        if ((intra_node_leader_rank != 0) && (intra_tmp_buf != NULL)) {
            free(intra_tmp_buf);
        }
    }
    
    MPIU_CHKLMEM_FREEALL(); 
    return (mpi_errno);
}

#undef FUNCNAME
#define FUNCNAME MPIR_Limic_Gather_Scheme_SINGLE_LEADER
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int MPIR_Limic_Gather_Scheme_SINGLE_LEADER(
                                         const void *sendbuf,int sendcnt, MPI_Datatype sendtype,
                                         void *recvbuf, int recvcnt,MPI_Datatype recvtype,
                                         int root, MPI_Comm comm, 
                                         int *errflag) 
{
    void *local_sendbuf=NULL;
    int rank;
    int local_rank, local_size;
    int mpi_errno = MPI_SUCCESS;
    int recvtype_size = 0, nbytes=0;
    int sendtype_iscontig;
    int send_nbytes=0; 
    MPI_Aint recvtype_extent = 0;  /* Datatype extent */
    MPI_Aint true_lb, sendtype_true_extent, recvtype_true_extent;
    MPI_Comm shmem_comm;
    MPID_Comm *shmem_commptr;
    MPI_Aint sendtype_size = 0;
    MPI_Aint position = 0;
    MPIU_CHKLMEM_DECL(1); 

    rank = smpi_comm_rank(comm);

    if (((rank == root) && (recvcnt == 0)) ||
            ((rank != root) && (sendcnt == 0))) {
        return MPI_SUCCESS;
    }

    if (sendtype != MPI_DATATYPE_NULL) {
        MPIR_Datatype_iscontig(sendtype, &sendtype_iscontig);
        sendtype_size=smpi_datatype_size(sendtype);
        smpi_datatype_extent(sendtype, &true_lb,
                &sendtype_true_extent);
    }
    if (recvtype != MPI_DATATYPE_NULL) {
        recvtype_extent=smpi_datatype_extent(recvtype);
        recvtype_size=smpi_datatype_size(recvtype);
        smpi_datatype_extent(recvtype, &true_lb,
                &recvtype_true_extent);
    }

    /* extract the rank,size information for the intra-node
     * communicator */
    shmem_comm = comm_ptr->ch.shmem_comm;
    MPID_Comm_get_ptr(shmem_comm, shmem_commptr);
    local_rank = shmem_commptr->rank;
    local_size = shmem_commptr->local_size;


    if (rank == root) {
        nbytes = recvcnt * recvtype_size;

    } else {
        nbytes = sendcnt * sendtype_size;
    }

    /*Pack data for non-contiguous buffer*/
  /*  if ((!sendtype_iscontig) && (sendbuf != MPI_IN_PLACE)) {

        MPIR_Pack_size_impl(1, sendtype, &sendtype_size);
        send_nbytes= sendcnt * sendtype_size;
        MPIU_CHKLMEM_MALLOC(local_sendbuf, void *, send_nbytes, mpi_errno, "local_sendbuf");
        MPIR_Pack_impl(sendbuf, sendcnt, sendtype, local_sendbuf, send_nbytes, &position);
    }*/
    else {
        local_sendbuf = (void *)sendbuf;
        send_nbytes = nbytes; 
    //}

    if (local_rank == 0) {
        /* Node leader, allocate tmp_buffer */
        if (rank == root) {
            tmp_buf = malloc(recvcnt * MPIR_MAX(recvtype_extent,
                        recvtype_true_extent) * local_size);
        } else {
            tmp_buf = malloc( send_nbytes * local_size);
        }
        if (tmp_buf == NULL) {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                    MPIR_ERR_RECOVERABLE,
                    FCNAME, __LINE__, MPI_ERR_OTHER,
                    "**nomem", 0);
            return mpi_errno;
        }

        /*Local copy of buffer*/
        if(sendbuf != MPI_IN_PLACE) {
            memcpy(tmp_buf, local_sendbuf, send_nbytes);
        } else {
            MPIR_Localcopy(((char *) recvbuf +rank * recvcnt * recvtype_extent),
                           recvcnt, recvtype,
                           tmp_buf, send_nbytes, MPI_BYTE);
        } 
    } 

    if(local_rank !=0 && sendbuf == MPI_IN_PLACE) {
        mpi_errno = MPIR_Limic_Gather_OSU(tmp_buf, (local_size * send_nbytes), 
                                          (recvbuf + (rank*nbytes)), 
                                          nbytes, shmem_commptr );
    } else {   
        mpi_errno = MPIR_Limic_Gather_OSU(tmp_buf, (local_size * send_nbytes), 
                                          local_sendbuf, nbytes, 
                                          shmem_commptr );
    }

    if (mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

fn_fail:
    MPIU_CHKLMEM_FREEALL();
    return (mpi_errno);
}

#undef FUNCNAME
#define FUNCNAME MPIR_Intra_node_LIMIC_Gather_MV2
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Intra_node_LIMIC_Gather_MV2(
                                     const void *sendbuf,int sendcnt, MPI_Datatype sendtype,
                                     void *recvbuf, int recvcnt,MPI_Datatype recvtype,
                                     int root, MPI_Comm comm, int *errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Comm shmem_comm;
    MPID_Comm *shmem_commptr;
    
    /* extract the rank,size information for the intra-node
     * communicator */
	shmem_comm = comm_ptr->ch.shmem_comm;
	MPID_Comm_get_ptr(shmem_comm, shmem_commptr);

    /*This case uses the PT-PT scheme with binomial
     * algorithm */
    if((shmem_commptr->ch.use_intra_sock_comm == 1) 
            && (num_scheme ==  USE_GATHER_PT_PT_BINOMIAL)) {

        mpi_errno = MPIR_Limic_Gather_Scheme_PT_PT(sendbuf, sendcnt, sendtype,
                                                   recvbuf, recvcnt, recvtype,
                                                   root, comm_ptr, 
                                                   MPIR_Gather_intra,
                                                   errflag);
        if (mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }
    } 
    /*This case uses the PT-PT scheme with DIRECT
     * algorithm */
    else if((shmem_commptr->ch.use_intra_sock_comm == 1) 
            && (num_scheme == USE_GATHER_PT_PT_DIRECT)) {

        mpi_errno = MPIR_Limic_Gather_Scheme_PT_PT(sendbuf, sendcnt, sendtype,
                                                   recvbuf, recvcnt, recvtype,
                                                   root, comm_ptr, 
                                                   MPIR_Gather_MV2_Direct,
                                                   errflag);
        if (mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }
    } 
    /*This case uses the PT-LINEAR scheme with binomial
     * algorithm */
    else if((shmem_commptr->ch.use_intra_sock_comm == 1) 
            && (num_scheme == USE_GATHER_PT_LINEAR_BINOMIAL)) {
        
        mpi_errno = MPIR_Limic_Gather_Scheme_PT_LINEAR(sendbuf, sendcnt, sendtype,
                                                       recvbuf, recvcnt, recvtype,
                                                       root, comm_ptr, 
                                                       MPIR_Gather_intra,
                                                       errflag);
        if (mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }

    } 
    /*This case uses the PT-LINEAR scheme with DIRECT
     * algorithm */
    else if((shmem_commptr->ch.use_intra_sock_comm == 1) 
            && (num_scheme == USE_GATHER_PT_LINEAR_DIRECT)) {
        
        mpi_errno = MPIR_Limic_Gather_Scheme_PT_LINEAR(sendbuf, sendcnt, sendtype,
                                                       recvbuf, recvcnt, recvtype,
                                                       root, comm_ptr, 
                                                       MPIR_Gather_MV2_Direct,
                                                       errflag);
        if (mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }

    } 
    /*This case uses the LINEAR-PT scheme with binomial
     * algorithm */
    else if((shmem_commptr->ch.use_intra_sock_comm == 1) 
              && (num_scheme == USE_GATHER_LINEAR_PT_BINOMIAL)) {
        
        mpi_errno = MPIR_Limic_Gather_Scheme_LINEAR_PT(sendbuf, sendcnt, sendtype,
                                                       recvbuf, recvcnt, recvtype,
                                                       root, comm_ptr, 
                                                       MPIR_Gather_intra,
                                                       errflag);
        if (mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }

    } 
    /*This case uses the LINEAR-PT scheme with DIRECT
     * algorithm */
    else if((shmem_commptr->ch.use_intra_sock_comm == 1) 
              && (num_scheme == USE_GATHER_LINEAR_PT_DIRECT)) {
        
        mpi_errno = MPIR_Limic_Gather_Scheme_LINEAR_PT(sendbuf, sendcnt, sendtype,
                                                       recvbuf, recvcnt, recvtype,
                                                       root, comm_ptr, 
                                                       MPIR_Gather_MV2_Direct,
                                                       errflag);
        if (mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }

    } else if((shmem_commptr->ch.use_intra_sock_comm == 1) 
             && (num_scheme == USE_GATHER_LINEAR_LINEAR)) {

        mpi_errno = MPIR_Limic_Gather_Scheme_LINEAR_LINEAR(sendbuf, sendcnt, sendtype,
                                                       recvbuf, recvcnt, recvtype,
                                                       root, comm_ptr,
                                                       errflag);
        if (mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }
      
    } else if(((comm_ptr->ch.shmem_coll_ok == 1) || 
              (shmem_commptr->ch.use_intra_sock_comm == 1))
             && (num_scheme == USE_GATHER_SINGLE_LEADER)) {

        mpi_errno = MPIR_Limic_Gather_Scheme_SINGLE_LEADER(sendbuf, sendcnt, sendtype,
                                                           recvbuf, recvcnt, recvtype,
                                                           root, comm_ptr,
                                                           errflag);
        if (mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }
    } else {
        /*This is a invalid case, if we are in LIMIC Gather
         * the code flow should be in one of the if-else case*/
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                         MPIR_ERR_RECOVERABLE,
                                         FCNAME, __LINE__, MPI_ERR_OTHER,
                                         "**badcase", 0);
    }


  fn_fail:
    return (mpi_errno);
}

#endif    /*#if defined(_SMP_LIMIC_) */
