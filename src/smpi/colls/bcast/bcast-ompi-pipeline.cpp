/* Copyright (c) 2013-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../coll_tuned_topo.hpp"
#include "../colls_private.hpp"

#define MAXTREEFANOUT 32

namespace simgrid{
namespace smpi{
int bcast__ompi_pipeline( void* buffer,
                          int original_count,
                          MPI_Datatype datatype,
                          int root,
                          MPI_Comm comm)
{
    int count_by_segment = original_count;
    size_t type_size;
    size_t segsize =1024  << 7;
    //mca_coll_tuned_module_t *tuned_module = (mca_coll_tuned_module_t*) module;
    //mca_coll_tuned_comm_t *data = tuned_module->tuned_data;

//    return ompi_coll_tuned_bcast_intra_generic( buffer, count, datatype, root, comm, module,
//                                                count_by_segment, data->cached_pipeline );
    ompi_coll_tree_t * tree = ompi_coll_tuned_topo_build_chain( 1, comm, root );
    int i;
    int rank, size;
    int segindex;
    int num_segments; /* Number of segments */
    int sendcount;    /* number of elements sent in this segment */
    size_t realsegsize;
    char *tmpbuf;
    ptrdiff_t extent;
    MPI_Request recv_reqs[2] = {MPI_REQUEST_NULL, MPI_REQUEST_NULL};
    MPI_Request* send_reqs   = nullptr;
    int req_index;

    /**
     * Determine number of elements sent per operation.
     */
    type_size = datatype->size();

    size = comm->size();
    rank = comm->rank();
    if(size==1)return MPI_SUCCESS;


    const double a_p16  = 3.2118e-6; /* [1 / byte] */
    const double b_p16  = 8.7936;
    const double a_p64  = 2.3679e-6; /* [1 / byte] */
    const double b_p64  = 1.1787;
    const double a_p128 = 1.6134e-6; /* [1 / byte] */
    const double b_p128 = 2.1102;
    size_t message_size;

    /* else we need data size for decision function */
    message_size = type_size * (unsigned long)original_count;   /* needed for decision */

    if (size < (a_p128 * message_size + b_p128)) {
            //Pipeline with 128KB segments
            segsize = 1024  << 7;
    }else if (size < (a_p64 * message_size + b_p64)) {
            // Pipeline with 64KB segments
            segsize = 1024 << 6;
    }else if (size < (a_p16 * message_size + b_p16)) {
            //Pipeline with 16KB segments
            segsize = 1024 << 4;
    }

    COLL_TUNED_COMPUTED_SEGCOUNT( segsize, type_size, count_by_segment );

    XBT_DEBUG("coll:tuned:bcast_intra_pipeline rank %d ss %5zu type_size %lu count_by_segment %d",
                 comm->rank(), segsize, (unsigned long)type_size, count_by_segment);



    extent = datatype->get_extent();
    num_segments = (original_count + count_by_segment - 1) / count_by_segment;
    realsegsize = count_by_segment * extent;

    /* Set the buffer pointers */
    tmpbuf = (char *) buffer;

    if( tree->tree_nextsize != 0 ) {
      send_reqs = new MPI_Request[tree->tree_nextsize];
    }

    /* Root code */
    if( rank == root ) {
        /*
           For each segment:
           - send segment to all children.
             The last segment may have less elements than other segments.
        */
        sendcount = count_by_segment;
        for( segindex = 0; segindex < num_segments; segindex++ ) {
            if( segindex == (num_segments - 1) ) {
                sendcount = original_count - segindex * count_by_segment;
            }
            for( i = 0; i < tree->tree_nextsize; i++ ) {
                send_reqs[i] = Request::isend(tmpbuf, sendcount, datatype,
                                         tree->tree_next[i],
                                         COLL_TAG_BCAST, comm);
           }

            /* complete the sends before starting the next sends */
            Request::waitall( tree->tree_nextsize, send_reqs,
                                         MPI_STATUSES_IGNORE );

            /* update tmp buffer */
            tmpbuf += realsegsize;

        }
    }

    /* Intermediate nodes code */
    else if( tree->tree_nextsize > 0 ) {
        /*
           Create the pipeline.
           1) Post the first receive
           2) For segments 1 .. num_segments
              - post new receive
              - wait on the previous receive to complete
              - send this data to children
           3) Wait on the last segment
           4) Compute number of elements in last segment.
           5) Send the last segment to children
         */
        req_index = 0;
        recv_reqs[req_index]=Request::irecv(tmpbuf, count_by_segment, datatype,
                           tree->tree_prev, COLL_TAG_BCAST,
                           comm);

        for( segindex = 1; segindex < num_segments; segindex++ ) {

            req_index = req_index ^ 0x1;

            /* post new irecv */
            recv_reqs[req_index]= Request::irecv( tmpbuf + realsegsize, count_by_segment,
                                datatype, tree->tree_prev,
                                COLL_TAG_BCAST,
                                comm);

            /* wait for and forward the previous segment to children */
            Request::wait( &recv_reqs[req_index ^ 0x1],
                                     MPI_STATUSES_IGNORE );

            for( i = 0; i < tree->tree_nextsize; i++ ) {
                send_reqs[i]=Request::isend(tmpbuf, count_by_segment, datatype,
                                         tree->tree_next[i],
                                         COLL_TAG_BCAST, comm );
            }

            /* complete the sends before starting the next iteration */
            Request::waitall( tree->tree_nextsize, send_reqs,
                                         MPI_STATUSES_IGNORE );

            /* Update the receive buffer */
            tmpbuf += realsegsize;
        }

        /* Process the last segment */
        Request::wait( &recv_reqs[req_index], MPI_STATUSES_IGNORE );
        sendcount = original_count - (num_segments - 1) * count_by_segment;
        for( i = 0; i < tree->tree_nextsize; i++ ) {
            send_reqs[i] = Request::isend(tmpbuf, sendcount, datatype,
                                     tree->tree_next[i],
                                     COLL_TAG_BCAST, comm);
        }

        Request::waitall( tree->tree_nextsize, send_reqs,
                                     MPI_STATUSES_IGNORE );
    }

    /* Leaf nodes */
    else {
        /*
           Receive all segments from parent in a loop:
           1) post irecv for the first segment
           2) for segments 1 .. num_segments
              - post irecv for the next segment
              - wait on the previous segment to arrive
           3) wait for the last segment
        */
        req_index = 0;
        recv_reqs[req_index] = Request::irecv(tmpbuf, count_by_segment, datatype,
                                 tree->tree_prev, COLL_TAG_BCAST,
                                 comm);

        for( segindex = 1; segindex < num_segments; segindex++ ) {
            req_index = req_index ^ 0x1;
            tmpbuf += realsegsize;
            /* post receive for the next segment */
            recv_reqs[req_index] = Request::irecv(tmpbuf, count_by_segment, datatype,
                                     tree->tree_prev, COLL_TAG_BCAST,
                                     comm);
            /* wait on the previous segment */
            Request::wait( &recv_reqs[req_index ^ 0x1],
                                     MPI_STATUS_IGNORE );
        }

        Request::wait( &recv_reqs[req_index], MPI_STATUS_IGNORE );
    }

    delete[] send_reqs;
    ompi_coll_tuned_topo_destroy_tree(&tree);

    return (MPI_SUCCESS);
}

}
}
