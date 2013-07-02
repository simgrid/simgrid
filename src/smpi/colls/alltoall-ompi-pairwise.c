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
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "colls_private.h"

int smpi_coll_tuned_alltoall_ompi_pairwise(void *sbuf, int scount, 
                                            MPI_Datatype sdtype,
                                            void* rbuf, int rcount,
                                            MPI_Datatype rdtype,
                                            MPI_Comm comm)
{
    int rank, size, step;
    int sendto, recvfrom;
    void * tmpsend, *tmprecv;
    MPI_Aint lb, sext, rext;

    size = smpi_comm_size(comm);
    rank = smpi_comm_rank(comm);

    XBT_VERB(
                 "coll:tuned:alltoall_ompi_pairwise rank %d", rank);

    smpi_datatype_extent (sdtype, &lb, &sext);
    smpi_datatype_extent (rdtype, &lb, &rext);

    
    /* Perform pairwise exchange - starting from 1 so the local copy is last */
    for (step = 1; step < size + 1; step++) {

        /* Determine sender and receiver for this step. */
        sendto  = (rank + step) % size;
        recvfrom = (rank + size - step) % size;

        /* Determine sending and receiving locations */
        tmpsend = (char*)sbuf + sendto * sext * scount;
        tmprecv = (char*)rbuf + recvfrom * rext * rcount;

        /* send and receive */
        smpi_mpi_sendrecv( tmpsend, scount, sdtype, sendto, 
                                        COLL_TAG_ALLTOALL,
                                        tmprecv, rcount, rdtype, recvfrom, 
                                        COLL_TAG_ALLTOALL,
                                        comm, MPI_STATUS_IGNORE);
    }

    return MPI_SUCCESS;
}
