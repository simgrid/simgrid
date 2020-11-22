/* Copyright (c) 2013-2020. The SimGrid Team.
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
 * Copyright (c) 2009      University of Houston. All rights reserved.
 *
 * Additional copyrights may follow
 *
 *  Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:

 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.

 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer listed
 *   in this license in the documentation and/or other materials
 *   provided with the distribution.

 * - Neither the name of the copyright holders nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.

 * The copyright holders provide no reassurances that the source code
 * provided does not infringe any patent, copyright, or any other
 * intellectual property rights of third parties.  The copyright holders
 * disclaim any liability to any recipient for claims brought against
 * recipient by any third party for infringement of that parties
 * intellectual property rights.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "../coll_tuned_topo.hpp"
#include "../colls_private.hpp"
#define MAXTREEFANOUT 32
namespace simgrid {
namespace smpi {

int bcast__ompi_split_bintree( void* buffer,
                               int count,
                               MPI_Datatype datatype,
                               int root,
                               MPI_Comm comm)
{
    unsigned int segsize ;
    int rank, size;
    int segindex, i, lr, pair;
    int segcount[2];       /* Number ompi_request_wait_allof elements sent with each segment */
    uint32_t counts[2];
    int num_segments[2];   /* Number of segments */
    int sendcount[2];      /* the same like segcount, except for the last segment */
    size_t realsegsize[2];
    char *tmpbuf[2];
    size_t type_size;
    ptrdiff_t type_extent;


    MPI_Request base_req, new_req;
    ompi_coll_tree_t *tree;
//    mca_coll_tuned_module_t *tuned_module = (mca_coll_tuned_module_t*) module;
//    mca_coll_tuned_comm_t *data = tuned_module->tuned_data;

    size = comm->size();
    rank = comm->rank();


    //compute again segsize
    const size_t intermediate_message_size = 370728;
    size_t message_size = datatype->size() * (unsigned long)count;
    if(message_size < intermediate_message_size)
      segsize = 1024 ;
    else
      segsize = 1024 << 3;

    XBT_DEBUG("ompi_coll_tuned_bcast_intra_split_bintree rank %d root %d ss %5u", rank, root, segsize);

    if (size == 1) {
        return MPI_SUCCESS;
    }

    /* setup the binary tree topology. */
    tree = ompi_coll_tuned_topo_build_tree(2,comm,root);

    type_size = datatype->size();

    /* Determine number of segments and number of elements per segment */
    counts[0] = count/2;
    if (count % 2 != 0) counts[0]++;
    counts[1] = count - counts[0];

    /* Note that ompi_datatype_type_size() will never return a negative
       value in typelng; it returns an int [vs. an unsigned type]
       because of the MPI spec. */
    if (segsize < ((uint32_t)type_size)) {
      segsize = type_size; /* push segsize up to hold one type */
    }
    segcount[0] = segcount[1] = segsize / type_size;
    num_segments[0]           = counts[0] / segcount[0];
    if ((counts[0] % segcount[0]) != 0)
      num_segments[0]++;
    num_segments[1] = counts[1] / segcount[1];
    if ((counts[1] % segcount[1]) != 0)
      num_segments[1]++;

    /* if the message is too small to be split into segments */
    if( (counts[0] == 0 || counts[1] == 0) ||
        (segsize > counts[0] * type_size) ||
        (segsize > counts[1] * type_size) ) {
        /* call linear version here ! */
        return bcast__SMP_linear( buffer, count, datatype, root, comm);
    }
    type_extent = datatype->get_extent();


    /* Determine real segment size */
    realsegsize[0] = segcount[0] * type_extent;
    realsegsize[1] = segcount[1] * type_extent;

    /* set the buffer pointers */
    tmpbuf[0] = (char *) buffer;
    tmpbuf[1] = (char *) buffer+counts[0] * type_extent;

    /* Step 1:
       Root splits the buffer in 2 and sends segmented message down the branches.
       Left subtree of the tree receives first half of the buffer, while right
       subtree receives the remaining message.
    */

    /* determine if I am left (0) or right (1), (root is right) */
    lr = ((rank + size - root)%size + 1)%2;

    /* root code */
    if( rank == root ) {
        /* determine segment count */
        sendcount[0] = segcount[0];
        sendcount[1] = segcount[1];
        /* for each segment */
        for (segindex = 0; segindex < num_segments[0]; segindex++) {
            /* for each child */
            for( i = 0; i < tree->tree_nextsize && i < 2; i++ ) {
                if (segindex >= num_segments[i]) { /* no more segments */
                    continue;
                }
                /* determine how many elements are being sent in this round */
                if(segindex == (num_segments[i] - 1))
                    sendcount[i] = counts[i] - segindex*segcount[i];
                /* send data */
                Request::send(tmpbuf[i], sendcount[i], datatype,
                                  tree->tree_next[i], COLL_TAG_BCAST, comm);
                /* update tmp buffer */
                tmpbuf[i] += realsegsize[i];
            }
        }
    }

    /* intermediate nodes code */
    else if( tree->tree_nextsize > 0 ) {
      /* Intermediate nodes:
       * It will receive segments only from one half of the data.
       * Which one is determined by whether the node belongs to the "left" or "right"
       * subtree. Topology building function builds binary tree such that
       * odd "shifted ranks" ((rank + size - root)%size) are on the left subtree,
       * and even on the right subtree.
       *
       * Create the pipeline. We first post the first receive, then in the loop we
       * post the next receive and after that wait for the previous receive to complete
       * and we disseminating the data to all children.
       */
      sendcount[lr] = segcount[lr];
      base_req      = Request::irecv(tmpbuf[lr], sendcount[lr], datatype, tree->tree_prev, COLL_TAG_BCAST, comm);

      for (segindex = 1; segindex < num_segments[lr]; segindex++) {
        /* determine how many elements to expect in this round */
        if (segindex == (num_segments[lr] - 1))
          sendcount[lr] = counts[lr] - segindex * segcount[lr];
        /* post new irecv */
        new_req = Request::irecv(tmpbuf[lr] + realsegsize[lr], sendcount[lr], datatype, tree->tree_prev, COLL_TAG_BCAST,
                                 comm);

        /* wait for and forward current segment */
        Request::waitall(1, &base_req, MPI_STATUSES_IGNORE);
        for (i = 0; i < tree->tree_nextsize; i++) { /* send data to children (segcount[lr]) */
          Request::send(tmpbuf[lr], segcount[lr], datatype, tree->tree_next[i], COLL_TAG_BCAST, comm);
        } /* end of for each child */

        /* update the base request */
        base_req = new_req;
        /* go to the next buffer (ie. the one corresponding to the next recv) */
        tmpbuf[lr] += realsegsize[lr];
        } /* end of for segindex */

        /* wait for the last segment and forward current segment */
        Request::waitall( 1, &base_req, MPI_STATUSES_IGNORE );
        for( i = 0; i < tree->tree_nextsize; i++ ) {  /* send data to children */
            Request::send(tmpbuf[lr], sendcount[lr], datatype,
                              tree->tree_next[i], COLL_TAG_BCAST, comm);
        } /* end of for each child */
    }

    /* leaf nodes */
    else {
        /* Just consume segments as fast as possible */
        sendcount[lr] = segcount[lr];
        for (segindex = 0; segindex < num_segments[lr]; segindex++) {
            /* determine how many elements to expect in this round */
            if (segindex == (num_segments[lr] - 1)) sendcount[lr] = counts[lr] - segindex*segcount[lr];
            /* receive segments */
            Request::recv(tmpbuf[lr], sendcount[lr], datatype,
                              tree->tree_prev, COLL_TAG_BCAST,
                              comm, MPI_STATUS_IGNORE);
            /* update the initial pointer to the buffer */
            tmpbuf[lr] += realsegsize[lr];
        }
    }

    /* reset the buffer pointers */
    tmpbuf[0] = (char *) buffer;
    tmpbuf[1] = (char *) buffer+counts[0] * type_extent;

    /* Step 2:
       Find your immediate pair (identical node in opposite subtree) and SendRecv
       data buffer with them.
       The tree building function ensures that
       if (we are not root)
       if we are in the left subtree (lr == 0) our pair is (rank+1)%size.
       if we are in the right subtree (lr == 1) our pair is (rank-1)%size
       If we have even number of nodes the rank (size-1) will pair up with root.
    */
    if (lr == 0) {
        pair = (rank+1)%size;
    } else {
        pair = (rank+size-1)%size;
    }

    if ( (size%2) != 0 && rank != root) {

        Request::sendrecv( tmpbuf[lr], counts[lr], datatype,
                                        pair, COLL_TAG_BCAST,
                                        tmpbuf[(lr+1)%2], counts[(lr+1)%2], datatype,
                                        pair, COLL_TAG_BCAST,
                                        comm, MPI_STATUS_IGNORE);
    } else if ( (size%2) == 0 ) {
        /* root sends right buffer to the last node */
        if( rank == root ) {
            Request::send(tmpbuf[1], counts[1], datatype,
                              (root+size-1)%size, COLL_TAG_BCAST, comm);

        }
        /* last node receives right buffer from the root */
        else if (rank == (root+size-1)%size) {
            Request::recv(tmpbuf[1], counts[1], datatype,
                              root, COLL_TAG_BCAST,
                              comm, MPI_STATUS_IGNORE);
        }
        /* everyone else exchanges buffers */
        else {
            Request::sendrecv( tmpbuf[lr], counts[lr], datatype,
                                            pair, COLL_TAG_BCAST,
                                            tmpbuf[(lr+1)%2], counts[(lr+1)%2], datatype,
                                            pair, COLL_TAG_BCAST,
                                            comm, MPI_STATUS_IGNORE);
        }
    }
    ompi_coll_tuned_topo_destroy_tree(&tree);
    return (MPI_SUCCESS);


}

}
}

