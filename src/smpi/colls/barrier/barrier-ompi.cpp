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

#include "../coll_tuned_topo.hpp"
#include "../colls_private.hpp"

/*
 * Barrier is ment to be a synchronous operation, as some BTLs can mark
 * a request done before its passed to the NIC and progress might not be made
 * elsewhere we cannot allow a process to exit the barrier until its last
 * [round of] sends are completed.
 *
 * It is last round of sends rather than 'last' individual send as each pair of
 * peers can use different channels/devices/btls and the receiver of one of
 * these sends might be forced to wait as the sender
 * leaves the collective and does not make progress until the next mpi call
 *
 */

/*
 * Simple double ring version of barrier
 *
 * synchronous guarantee made by last ring of sends are synchronous
 *
 */
namespace simgrid {
namespace smpi {
int barrier__ompi_doublering(MPI_Comm comm)
{
    int rank, size;
    int left, right;


    rank = comm->rank();
    size = comm->size();

    XBT_DEBUG("ompi_coll_tuned_barrier_ompi_doublering rank %d", rank);

    left = ((rank-1+size)%size);
    right = ((rank+1)%size);

    if (rank > 0) { /* receive message from the left */
        Request::recv((void*)NULL, 0, MPI_BYTE, left,
                                COLL_TAG_BARRIER, comm,
                                MPI_STATUS_IGNORE);
    }

    /* Send message to the right */
    Request::send((void*)NULL, 0, MPI_BYTE, right,
                            COLL_TAG_BARRIER,
                             comm);

    /* root needs to receive from the last node */
    if (rank == 0) {
        Request::recv((void*)NULL, 0, MPI_BYTE, left,
                                COLL_TAG_BARRIER, comm,
                                MPI_STATUS_IGNORE);
    }

    /* Allow nodes to exit */
    if (rank > 0) { /* post Receive from left */
        Request::recv((void*)NULL, 0, MPI_BYTE, left,
                                COLL_TAG_BARRIER, comm,
                                MPI_STATUS_IGNORE);
    }

    /* send message to the right one */
    Request::send((void*)NULL, 0, MPI_BYTE, right,
                            COLL_TAG_BARRIER,
                             comm);

    /* rank 0 post receive from the last node */
    if (rank == 0) {
        Request::recv((void*)NULL, 0, MPI_BYTE, left,
                                COLL_TAG_BARRIER, comm,
                                MPI_STATUS_IGNORE);
    }

    return MPI_SUCCESS;

}


/*
 * To make synchronous, uses sync sends and sync sendrecvs
 */

int barrier__ompi_recursivedoubling(MPI_Comm comm)
{
    int rank, size, adjsize;
    int mask, remote;

    rank = comm->rank();
    size = comm->size();
    XBT_DEBUG(
                 "ompi_coll_tuned_barrier_ompi_recursivedoubling rank %d",
                 rank);

    /* do nearest power of 2 less than size calc */
    for( adjsize = 1; adjsize <= size; adjsize <<= 1 );
    adjsize >>= 1;

    /* if size is not exact power of two, perform an extra step */
    if (adjsize != size) {
        if (rank >= adjsize) {
            /* send message to lower ranked node */
            remote = rank - adjsize;
            Request::sendrecv(NULL, 0, MPI_BYTE, remote,
                                                  COLL_TAG_BARRIER,
                                                  NULL, 0, MPI_BYTE, remote,
                                                  COLL_TAG_BARRIER,
                                                  comm, MPI_STATUS_IGNORE);

        } else if (rank < (size - adjsize)) {

            /* receive message from high level rank */
            Request::recv((void*)NULL, 0, MPI_BYTE, rank+adjsize,
                                    COLL_TAG_BARRIER, comm,
                                    MPI_STATUS_IGNORE);

        }
    }

    /* exchange messages */
    if ( rank < adjsize ) {
        mask = 0x1;
        while ( mask < adjsize ) {
            remote = rank ^ mask;
            mask <<= 1;
            if (remote >= adjsize) continue;

            /* post receive from the remote node */
            Request::sendrecv(NULL, 0, MPI_BYTE, remote,
                                                  COLL_TAG_BARRIER,
                                                  NULL, 0, MPI_BYTE, remote,
                                                  COLL_TAG_BARRIER,
                                                  comm, MPI_STATUS_IGNORE);
        }
    }

    /* non-power of 2 case */
    if (adjsize != size) {
        if (rank < (size - adjsize)) {
            /* send enter message to higher ranked node */
            remote = rank + adjsize;
            Request::send((void*)NULL, 0, MPI_BYTE, remote,
                                    COLL_TAG_BARRIER,
                                     comm);

        }
    }

    return MPI_SUCCESS;

}


/*
 * To make synchronous, uses sync sends and sync sendrecvs
 */

int barrier__ompi_bruck(MPI_Comm comm)
{
    int rank, size;
    int distance, to, from;

    rank = comm->rank();
    size = comm->size();
    XBT_DEBUG(
                 "ompi_coll_tuned_barrier_ompi_bruck rank %d", rank);

    /* exchange data with rank-2^k and rank+2^k */
    for (distance = 1; distance < size; distance <<= 1) {
        from = (rank + size - distance) % size;
        to   = (rank + distance) % size;

        /* send message to lower ranked node */
        Request::sendrecv(NULL, 0, MPI_BYTE, to,
                                              COLL_TAG_BARRIER,
                                              NULL, 0, MPI_BYTE, from,
                                              COLL_TAG_BARRIER,
                                              comm, MPI_STATUS_IGNORE);
    }

    return MPI_SUCCESS;

}


/*
 * To make synchronous, uses sync sends and sync sendrecvs
 */
/* special case for two processes */
int barrier__ompi_two_procs(MPI_Comm comm)
{
    int remote;

    remote = comm->rank();
    XBT_DEBUG(
                 "ompi_coll_tuned_barrier_ompi_two_procs rank %d", remote);
    remote = (remote + 1) & 0x1;

    Request::sendrecv(NULL, 0, MPI_BYTE, remote,
                                          COLL_TAG_BARRIER,
                                          NULL, 0, MPI_BYTE, remote,
                                          COLL_TAG_BARRIER,
                                          comm, MPI_STATUS_IGNORE);
    return (MPI_SUCCESS);
}


/*
 * Linear functions are copied from the BASIC coll module
 * they do not segment the message and are simple implementations
 * but for some small number of nodes and/or small data sizes they
 * are just as fast as tuned/tree based segmenting operations
 * and as such may be selected by the decision functions
 * These are copied into this module due to the way we select modules
 * in V1. i.e. in V2 we will handle this differently and so will not
 * have to duplicate code.
 * GEF Oct05 after asking Jeff.
 */

/* copied function (with appropriate renaming) starts here */

int barrier__ompi_basic_linear(MPI_Comm comm)
{
    int i;
    int size = comm->size();
    int rank = comm->rank();

    /* All non-root send & receive zero-length message. */

    if (rank > 0) {
        Request::send (NULL, 0, MPI_BYTE, 0,
                                 COLL_TAG_BARRIER,
                                  comm);

        Request::recv (NULL, 0, MPI_BYTE, 0,
                                 COLL_TAG_BARRIER,
                                 comm, MPI_STATUS_IGNORE);
    }

    /* The root collects and broadcasts the messages. */

    else {
        MPI_Request* requests;

        requests = new MPI_Request[size];
        for (i = 1; i < size; ++i) {
          requests[i] = Request::irecv(NULL, 0, MPI_BYTE, i, COLL_TAG_BARRIER, comm);
        }
        Request::waitall( size-1, requests+1, MPI_STATUSES_IGNORE );

        for (i = 1; i < size; ++i) {
            requests[i] = Request::isend(NULL, 0, MPI_BYTE, i,
                                     COLL_TAG_BARRIER,
                                      comm
                                     );
        }
        Request::waitall( size-1, requests+1, MPI_STATUSES_IGNORE );
        delete[] requests;
    }

    /* All done */

    return MPI_SUCCESS;

}
/* copied function (with appropriate renaming) ends here */

/*
 * Another recursive doubling type algorithm, but in this case
 * we go up the tree and back down the tree.
 */
int barrier__ompi_tree(MPI_Comm comm)
{
    int rank, size, depth;
    int jump, partner;

    rank = comm->rank();
    size = comm->size();
    XBT_DEBUG(
                 "ompi_coll_tuned_barrier_ompi_tree %d",
                 rank);

    /* Find the nearest power of 2 of the communicator size. */
    for(depth = 1; depth < size; depth <<= 1 );

    for (jump=1; jump<depth; jump<<=1) {
        partner = rank ^ jump;
        if (!(partner & (jump-1)) && partner < size) {
            if (partner > rank) {
                Request::recv (NULL, 0, MPI_BYTE, partner,
                                         COLL_TAG_BARRIER, comm,
                                         MPI_STATUS_IGNORE);
            } else if (partner < rank) {
                Request::send (NULL, 0, MPI_BYTE, partner,
                                         COLL_TAG_BARRIER,
                                          comm);
            }
        }
    }

    depth>>=1;
    for (jump = depth; jump>0; jump>>=1) {
        partner = rank ^ jump;
        if (!(partner & (jump-1)) && partner < size) {
            if (partner > rank) {
                Request::send (NULL, 0, MPI_BYTE, partner,
                                         COLL_TAG_BARRIER,
                                          comm);
            } else if (partner < rank) {
                Request::recv (NULL, 0, MPI_BYTE, partner,
                                         COLL_TAG_BARRIER, comm,
                                         MPI_STATUS_IGNORE);
            }
        }
    }

    return MPI_SUCCESS;
}

}
}
