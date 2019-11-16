/* Copyright (c) 2013-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2008      Sun Microsystems, Inc.  All rights reserved.
 *
 * Additional copyrights may follow
 */

 /* -*- Mode: C; c-basic-offset:4 ; -*- */
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

#include "../coll_tuned_topo.hpp"
#include "../colls_private.hpp"
namespace simgrid{
namespace smpi{
int barrier__mvapich2_pair(MPI_Comm comm)
{

    int size, rank;
    int d, dst, src;
    int mpi_errno = MPI_SUCCESS;

    size = comm->size();
    /* Trivial barriers return immediately */
    if (size == 1)
        return MPI_SUCCESS;

    rank =  comm->rank();
    int N2_prev = 1;
    /*  N2_prev = greatest power of two < size of Comm  */
    for( N2_prev = 1; N2_prev <= size; N2_prev <<= 1 );
    N2_prev >>= 1;

    int surfeit = size - N2_prev;

    /* Perform a combine-like operation */
    if (rank < N2_prev) {
        if (rank < surfeit) {
            /* get the fanin letter from the upper "half" process: */
            dst = N2_prev + rank;
            Request::recv(NULL, 0, MPI_BYTE, dst, COLL_TAG_BARRIER,
                                     comm, MPI_STATUS_IGNORE);
        }

        /* combine on embedded N2_prev power-of-two processes */
        for (d = 1; d < N2_prev; d <<= 1) {
            dst = (rank ^ d);
            Request::sendrecv(NULL, 0, MPI_BYTE, dst, COLL_TAG_BARRIER, NULL,
                                 0, MPI_BYTE, dst, COLL_TAG_BARRIER, comm,
                                 MPI_STATUS_IGNORE);
        }

        /* fanout data to nodes above N2_prev... */
        if (rank < surfeit) {
            dst = N2_prev + rank;
            Request::send(NULL, 0, MPI_BYTE, dst, COLL_TAG_BARRIER,
                                     comm);
        }
    } else {
        /* fanin data to power of 2 subset */
        src = rank - N2_prev;
        Request::sendrecv(NULL, 0, MPI_BYTE, src, COLL_TAG_BARRIER,
                                     NULL, 0, MPI_BYTE, src, COLL_TAG_BARRIER,
                                     comm, MPI_STATUS_IGNORE);
    }

    return mpi_errno;

}

}
}
