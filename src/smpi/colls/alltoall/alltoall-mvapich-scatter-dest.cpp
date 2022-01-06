/* Copyright (c) 2013-2022. The SimGrid Team.
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

//correct on stampede
#define MV2_ALLTOALL_THROTTLE_FACTOR         4

#include "../colls_private.hpp"
namespace simgrid{
namespace smpi{
int alltoall__mvapich2_scatter_dest(const void *sendbuf,
                                    int sendcount,
                                    MPI_Datatype sendtype,
                                    void *recvbuf,
                                    int recvcount,
                                    MPI_Datatype recvtype,
                                    MPI_Comm comm)
{
    int          comm_size, i, j;
    MPI_Aint     sendtype_extent = 0, recvtype_extent = 0;
    int mpi_errno=MPI_SUCCESS;
    int dst, rank;

    if (recvcount == 0) return MPI_SUCCESS;

    comm_size =  comm->size();
    rank = comm->rank();

    /* Get extent of send and recv types */
    recvtype_extent = recvtype->get_extent();
    sendtype_extent = sendtype->get_extent();

    /* Medium-size message. Use isend/irecv with scattered
     destinations. Use Tony Ladd's modification to post only
     a small number of isends/irecvs at a time. */
    /* FIXME: This converts the Alltoall to a set of blocking phases.
     Two alternatives should be considered:
     1) the choice of communication pattern could try to avoid
     contending routes in each phase
     2) rather than wait for all communication to finish (waitall),
     we could maintain constant queue size by using waitsome
     and posting new isend/irecv as others complete.  This avoids
     synchronization delays at the end of each block (when
     there are only a few isend/irecvs left)
     */
    int ii, ss, bblock;

    //Stampede is configured with
    bblock = MV2_ALLTOALL_THROTTLE_FACTOR;//mv2_coll_param.alltoall_throttle_factor;

    if (bblock >= comm_size) bblock = comm_size;
    /* If throttle_factor is n, each process posts n pairs of isend/irecv
     in each iteration. */

    /* FIXME: This should use the memory macros (there are storage
     leaks here if there is an error, for example) */
    auto* reqarray = new MPI_Request[2 * bblock];

    auto* starray = new MPI_Status[2 * bblock];

    for (ii=0; ii<comm_size; ii+=bblock) {
        ss = comm_size-ii < bblock ? comm_size-ii : bblock;
        /* do the communication -- post ss sends and receives: */
        for ( i=0; i<ss; i++ ) {
            dst = (rank+i+ii) % comm_size;
            reqarray[i]=Request::irecv((char *)recvbuf +
                                      dst*recvcount*recvtype_extent,
                                      recvcount, recvtype, dst,
                                      COLL_TAG_ALLTOALL, comm);

        }
        for ( i=0; i<ss; i++ ) {
            dst = (rank-i-ii+comm_size) % comm_size;
            reqarray[i+ss]=Request::isend((char *)sendbuf +
                                          dst*sendcount*sendtype_extent,
                                          sendcount, sendtype, dst,
                                          COLL_TAG_ALLTOALL, comm);

        }

        /* ... then wait for them to finish: */
        Request::waitall(2*ss,reqarray,starray);


        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno == MPI_ERR_IN_STATUS) {
                for (j=0; j<2*ss; j++) {
                     if (starray[j].MPI_ERROR != MPI_SUCCESS) {
                         mpi_errno = starray[j].MPI_ERROR;
                     }
                }
        }
    }
    /* --END ERROR HANDLING-- */
    delete[] starray;
    delete[] reqarray;
    return (mpi_errno);

}
}
}
