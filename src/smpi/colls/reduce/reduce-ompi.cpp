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
 *
 * Additional copyrights may follow
 */

#include "../coll_tuned_topo.hpp"
#include "../colls_private.hpp"

namespace simgrid{
namespace smpi{

int smpi_coll_tuned_ompi_reduce_generic(const void* sendbuf, void* recvbuf, int original_count,
                                    MPI_Datatype datatype, MPI_Op  op,
                                    int root, MPI_Comm comm,
                                    ompi_coll_tree_t* tree, int count_by_segment,
                                    int max_outstanding_reqs );
/**
 * This is a generic implementation of the reduce protocol. It used the tree
 * provided as an argument and execute all operations using a segment of
 * count times a datatype.
 * For the last communication it will update the count in order to limit
 * the number of datatype to the original count (original_count)
 *
 * Note that for non-commutative operations we cannot save memory copy
 * for the first block: thus we must copy sendbuf to accumbuf on intermediate
 * to keep the optimized loop happy.
 */
int smpi_coll_tuned_ompi_reduce_generic(const void* sendbuf, void* recvbuf, int original_count,
                                    MPI_Datatype datatype, MPI_Op  op,
                                    int root, MPI_Comm comm,
                                    ompi_coll_tree_t* tree, int count_by_segment,
                                    int max_outstanding_reqs )
{
  unsigned char *inbuf[2] = {nullptr, nullptr}, *inbuf_free[2] = {nullptr, nullptr};
  unsigned char *accumbuf = nullptr, *accumbuf_free = nullptr;
  const unsigned char *local_op_buffer = nullptr, *sendtmpbuf = nullptr;
  ptrdiff_t extent, lower_bound, segment_increment;
  MPI_Request reqs[2] = {MPI_REQUEST_NULL, MPI_REQUEST_NULL};
  int num_segments, line, ret, segindex, i, rank;
  int recvcount, prevcount, inbi;

  /**
   * Determine number of segments and number of elements
   * sent per operation
   */
  datatype->extent(&lower_bound, &extent);
  num_segments      = (original_count + count_by_segment - 1) / count_by_segment;
  segment_increment = count_by_segment * extent;

  sendtmpbuf = static_cast<const unsigned char*>(sendbuf);
  if (sendbuf == MPI_IN_PLACE) {
    sendtmpbuf = static_cast<const unsigned char*>(recvbuf);
    }

    XBT_DEBUG("coll:tuned:reduce_generic count %d, msg size %lu, segsize %lu, max_requests %d", original_count,
              (unsigned long)(num_segments * segment_increment), (unsigned long)segment_increment,
              max_outstanding_reqs);

    rank = comm->rank();

    /* non-leaf nodes - wait for children to send me data & forward up
       (if needed) */
    if( tree->tree_nextsize > 0 ) {
        ptrdiff_t true_extent, real_segment_size;
        true_extent=datatype->get_extent();

        /* handle non existent recv buffer (i.e. its NULL) and
           protect the recv buffer on non-root nodes */
        accumbuf = static_cast<unsigned char*>(recvbuf);
        if (nullptr == accumbuf || root != rank) {
          /* Allocate temporary accumulator buffer. */
          accumbuf_free = smpi_get_tmp_sendbuffer(true_extent + (original_count - 1) * extent);
          if (accumbuf_free == nullptr) {
            line = __LINE__;
            ret  = -1;
            goto error_hndl;
          }
          accumbuf = accumbuf_free - lower_bound;
        }

        /* If this is a non-commutative operation we must copy
           sendbuf to the accumbuf, in order to simplify the loops */
        if ((op != MPI_OP_NULL && not op->is_commutative())) {
          Datatype::copy(sendtmpbuf, original_count, datatype, accumbuf, original_count, datatype);
        }
        /* Allocate two buffers for incoming segments */
        real_segment_size = true_extent + (count_by_segment - 1) * extent;
        inbuf_free[0]     = smpi_get_tmp_recvbuffer(real_segment_size);
        if (inbuf_free[0] == nullptr) {
          line = __LINE__;
          ret  = -1;
          goto error_hndl;
        }
        inbuf[0] = inbuf_free[0] - lower_bound;
        /* if there is chance to overlap communication -
           allocate second buffer */
        if( (num_segments > 1) || (tree->tree_nextsize > 1) ) {
          inbuf_free[1] = smpi_get_tmp_recvbuffer(real_segment_size);
          if (inbuf_free[1] == nullptr) {
            line = __LINE__;
            ret  = -1;
            goto error_hndl;
            }
            inbuf[1] = inbuf_free[1] - lower_bound;
        }

        /* reset input buffer index and receive count */
        inbi = 0;
        recvcount = 0;
        /* for each segment */
        for( segindex = 0; segindex <= num_segments; segindex++ ) {
            prevcount = recvcount;
            /* recvcount - number of elements in current segment */
            recvcount = count_by_segment;
            if( segindex == (num_segments-1) )
                recvcount = original_count - count_by_segment * segindex;

            /* for each child */
            for( i = 0; i < tree->tree_nextsize; i++ ) {
                /**
                 * We try to overlap communication:
                 * either with next segment or with the next child
                 */
                /* post irecv for current segindex on current child */
                if( segindex < num_segments ) {
                    void* local_recvbuf = inbuf[inbi];
                    if( 0 == i ) {
                        /* for the first step (1st child per segment) and
                         * commutative operations we might be able to irecv
                         * directly into the accumulate buffer so that we can
                         * reduce(op) this with our sendbuf in one step as
                         * ompi_op_reduce only has two buffer pointers,
                         * this avoids an extra memory copy.
                         *
                         * BUT if the operation is non-commutative or
                         * we are root and are USING MPI_IN_PLACE this is wrong!
                         */
                        if(  (op==MPI_OP_NULL || op->is_commutative()) &&
                            !((MPI_IN_PLACE == sendbuf) && (rank == tree->tree_root)) ) {
                            local_recvbuf = accumbuf + segindex * segment_increment;
                        }
                    }

                    reqs[inbi]=Request::irecv(local_recvbuf, recvcount, datatype,
                                             tree->tree_next[i],
                                             COLL_TAG_REDUCE, comm
                                             );
                }
                /* wait for previous req to complete, if any.
                   if there are no requests reqs[inbi ^1] will be
                   MPI_REQUEST_NULL. */
                /* wait on data from last child for previous segment */
                Request::waitall( 1, &reqs[inbi ^ 1],
                                             MPI_STATUSES_IGNORE );
                local_op_buffer = inbuf[inbi ^ 1];
                if( i > 0 ) {
                    /* our first operation is to combine our own [sendbuf] data
                     * with the data we recvd from down stream (but only
                     * the operation is commutative and if we are not root and
                     * not using MPI_IN_PLACE)
                     */
                    if( 1 == i ) {
                        if( (op==MPI_OP_NULL || op->is_commutative())&&
                            !((MPI_IN_PLACE == sendbuf) && (rank == tree->tree_root)) ) {
                            local_op_buffer = sendtmpbuf + segindex * segment_increment;
                        }
                    }
                    /* apply operation */
                    if(op!=MPI_OP_NULL) op->apply( local_op_buffer,
                                   accumbuf + segindex * segment_increment,
                                   &recvcount, datatype );
                } else if ( segindex > 0 ) {
                    void* accumulator = accumbuf + (segindex-1) * segment_increment;
                    if( tree->tree_nextsize <= 1 ) {
                        if(  (op==MPI_OP_NULL || op->is_commutative()) &&
                            !((MPI_IN_PLACE == sendbuf) && (rank == tree->tree_root)) ) {
                            local_op_buffer = sendtmpbuf + (segindex-1) * segment_increment;
                        }
                    }
                    if(op!=MPI_OP_NULL) op->apply( local_op_buffer, accumulator, &prevcount,
                                   datatype );

                    /* all reduced on available data this step (i) complete,
                     * pass to the next process unless you are the root.
                     */
                    if (rank != tree->tree_root) {
                        /* send combined/accumulated data to parent */
                        Request::send( accumulator, prevcount,
                                                  datatype, tree->tree_prev,
                                                  COLL_TAG_REDUCE,
                                                  comm);
                    }

                    /* we stop when segindex = number of segments
                       (i.e. we do num_segment+1 steps for pipelining */
                    if (segindex == num_segments) break;
                }

                /* update input buffer index */
                inbi = inbi ^ 1;
            } /* end of for each child */
        } /* end of for each segment */

        /* clean up */
        smpi_free_tmp_buffer(inbuf_free[0]);
        smpi_free_tmp_buffer(inbuf_free[1]);
        smpi_free_tmp_buffer(accumbuf_free);
    }

    /* leaf nodes
       Depending on the value of max_outstanding_reqs and
       the number of segments we have two options:
       - send all segments using blocking send to the parent, or
       - avoid overflooding the parent nodes by limiting the number of
       outstanding requests to max_oustanding_reqs.
       TODO/POSSIBLE IMPROVEMENT: If there is a way to determine the eager size
       for the current communication, synchronization should be used only
       when the message/segment size is smaller than the eager size.
    */
    else {

        /* If the number of segments is less than a maximum number of outstanding
           requests or there is no limit on the maximum number of outstanding
           requests, we send data to the parent using blocking send */
        if ((0 == max_outstanding_reqs) ||
            (num_segments <= max_outstanding_reqs)) {

            segindex = 0;
            while ( original_count > 0) {
                if (original_count < count_by_segment) {
                    count_by_segment = original_count;
                }
                Request::send((char*)sendbuf +
                                         segindex * segment_increment,
                                         count_by_segment, datatype,
                                         tree->tree_prev,
                                         COLL_TAG_REDUCE,
                                         comm) ;
                segindex++;
                original_count -= count_by_segment;
            }
        }

        /* Otherwise, introduce flow control:
           - post max_outstanding_reqs non-blocking synchronous send,
           - for remaining segments
           - wait for a ssend to complete, and post the next one.
           - wait for all outstanding sends to complete.
        */
        else {

            int creq = 0;
            MPI_Request* sreq = new (std::nothrow) MPI_Request[max_outstanding_reqs];
            if (NULL == sreq) { line = __LINE__; ret = -1; goto error_hndl; }

            /* post first group of requests */
            for (segindex = 0; segindex < max_outstanding_reqs; segindex++) {
                sreq[segindex]=Request::isend((char*)sendbuf +
                                          segindex * segment_increment,
                                          count_by_segment, datatype,
                                          tree->tree_prev,
                                          COLL_TAG_REDUCE,
                                          comm);
                original_count -= count_by_segment;
            }

            creq = 0;
            while ( original_count > 0 ) {
                /* wait on a posted request to complete */
                Request::wait(&sreq[creq], MPI_STATUS_IGNORE);
                sreq[creq] = MPI_REQUEST_NULL;

                if( original_count < count_by_segment ) {
                    count_by_segment = original_count;
                }
                sreq[creq]=Request::isend((char*)sendbuf +
                                          segindex * segment_increment,
                                          count_by_segment, datatype,
                                          tree->tree_prev,
                                          COLL_TAG_REDUCE,
                                          comm );
                creq = (creq + 1) % max_outstanding_reqs;
                segindex++;
                original_count -= count_by_segment;
            }

            /* Wait on the remaining request to complete */
            Request::waitall( max_outstanding_reqs, sreq,
                                         MPI_STATUSES_IGNORE );

            /* free requests */
            delete[] sreq;
        }
    }
    ompi_coll_tuned_topo_destroy_tree(&tree);
    return MPI_SUCCESS;

 error_hndl:  /* error handler */
    XBT_DEBUG("ERROR_HNDL: node %d file %s line %d error %d\n",
                   rank, __FILE__, line, ret );
    smpi_free_tmp_buffer(inbuf_free[0]);
    smpi_free_tmp_buffer(inbuf_free[1]);
    smpi_free_tmp_buffer(accumbuf);
    return ret;
}

/* Attention: this version of the reduce operations does not
   work for:
   - non-commutative operations
   - segment sizes which are not multiplies of the extent of the datatype
     meaning that at least one datatype must fit in the segment !
*/


int reduce__ompi_chain(const void *sendbuf, void *recvbuf, int count,
                       MPI_Datatype datatype,
                       MPI_Op  op, int root,
                       MPI_Comm  comm
                       )
{
    uint32_t segsize=64*1024;
    int segcount = count;
    size_t typelng;
    int fanout = comm->size()/2;

    XBT_DEBUG("coll:tuned:reduce_intra_chain rank %d fo %d ss %5u", comm->rank(), fanout, segsize);

    /**
     * Determine number of segments and number of elements
     * sent per operation
     */
    typelng = datatype->size();

    COLL_TUNED_COMPUTED_SEGCOUNT( segsize, typelng, segcount );

    return smpi_coll_tuned_ompi_reduce_generic( sendbuf, recvbuf, count, datatype,
                                           op, root, comm,
                                           ompi_coll_tuned_topo_build_chain(fanout, comm, root),
                                           segcount, 0 );
}


int reduce__ompi_pipeline(const void *sendbuf, void *recvbuf,
                          int count, MPI_Datatype datatype,
                          MPI_Op  op, int root,
                          MPI_Comm  comm  )
{

    uint32_t segsize;
    int segcount = count;
    size_t typelng;
//    COLL_TUNED_UPDATE_PIPELINE( comm, tuned_module, root );

    /**
     * Determine number of segments and number of elements
     * sent per operation
     */
    const double a2 =  0.0410 / 1024.0; /* [1/B] */
    const double b2 =  9.7128;
    const double a4 =  0.0033 / 1024.0; /* [1/B] */
    const double b4 =  1.6761;
    typelng= datatype->size();
    int communicator_size = comm->size();
    size_t message_size = typelng * count;

    if (communicator_size > (a2 * message_size + b2)) {
        // Pipeline_1K
        segsize = 1024;
    }else if (communicator_size > (a4 * message_size + b4)) {
        // Pipeline_32K
        segsize = 32*1024;
    } else {
        // Pipeline_64K
        segsize = 64*1024;
    }

    XBT_DEBUG("coll:tuned:reduce_intra_pipeline rank %d ss %5u", comm->rank(), segsize);

    COLL_TUNED_COMPUTED_SEGCOUNT( segsize, typelng, segcount );

    return smpi_coll_tuned_ompi_reduce_generic( sendbuf, recvbuf, count, datatype,
                                           op, root, comm,
                                           ompi_coll_tuned_topo_build_chain( 1, comm, root),
                                           segcount, 0);
}

int reduce__ompi_binary(const void *sendbuf, void *recvbuf,
                        int count, MPI_Datatype datatype,
                        MPI_Op  op, int root,
                        MPI_Comm  comm)
{
    uint32_t segsize;
    int segcount = count;
    size_t typelng;



    /**
     * Determine number of segments and number of elements
     * sent per operation
     */
    typelng=datatype->size();

        // Binary_32K
    segsize = 32*1024;

    XBT_DEBUG("coll:tuned:reduce_intra_binary rank %d ss %5u", comm->rank(), segsize);

    COLL_TUNED_COMPUTED_SEGCOUNT( segsize, typelng, segcount );

    return smpi_coll_tuned_ompi_reduce_generic( sendbuf, recvbuf, count, datatype,
                                           op, root, comm,
                                           ompi_coll_tuned_topo_build_tree(2, comm, root),
                                           segcount, 0);
}

int reduce__ompi_binomial(const void *sendbuf, void *recvbuf,
                          int count, MPI_Datatype datatype,
                          MPI_Op  op, int root,
                          MPI_Comm  comm)
{

    uint32_t segsize=0;
    int segcount = count;
    size_t typelng;

    const double a1 =  0.6016 / 1024.0; /* [1/B] */
    const double b1 =  1.3496;

//    COLL_TUNED_UPDATE_IN_ORDER_BMTREE( comm, tuned_module, root );

    /**
     * Determine number of segments and number of elements
     * sent per operation
     */
    typelng= datatype->size();
    int communicator_size = comm->size();
    size_t message_size = typelng * count;
    if (((communicator_size < 8) && (message_size < 20480)) ||
               (message_size < 2048) || (count <= 1)) {
        /* Binomial_0K */
        segsize = 0;
    } else if (communicator_size > (a1 * message_size + b1)) {
        // Binomial_1K
        segsize = 1024;
    }

    XBT_DEBUG("coll:tuned:reduce_intra_binomial rank %d ss %5u", comm->rank(), segsize);
    COLL_TUNED_COMPUTED_SEGCOUNT( segsize, typelng, segcount );

    return smpi_coll_tuned_ompi_reduce_generic( sendbuf, recvbuf, count, datatype,
                                           op, root, comm,
                                           ompi_coll_tuned_topo_build_in_order_bmtree(comm, root),
                                           segcount, 0);
}

/*
 * reduce_intra_in_order_binary
 *
 * Function:      Logarithmic reduce operation for non-commutative operations.
 * Accepts:       same as MPI_Reduce()
 * Returns:       MPI_SUCCESS or error code
 */
int reduce__ompi_in_order_binary(const void *sendbuf, void *recvbuf,
                                 int count,
                                 MPI_Datatype datatype,
                                 MPI_Op  op, int root,
                                 MPI_Comm  comm)
{
    uint32_t segsize=0;
    int ret;
    int rank, size, io_root;
    int segcount = count;
    size_t typelng;

    rank = comm->rank();
    size = comm->size();
    XBT_DEBUG("coll:tuned:reduce_intra_in_order_binary rank %d ss %5u", rank, segsize);

    /**
     * Determine number of segments and number of elements
     * sent per operation
     */
    typelng=datatype->size();
    COLL_TUNED_COMPUTED_SEGCOUNT( segsize, typelng, segcount );

    /* An in-order binary tree must use root (size-1) to preserve the order of
       operations.  Thus, if root is not rank (size - 1), then we must handle
       1. MPI_IN_PLACE option on real root, and
       2. we must allocate temporary recvbuf on rank (size - 1).
       Note that generic function must be careful not to switch order of
       operations for non-commutative ops.
    */
    io_root = size - 1;
    const void* use_this_sendbuf = sendbuf;
    void* use_this_recvbuf       = recvbuf;
    unsigned char* tmp_sendbuf   = nullptr;
    unsigned char* tmp_recvbuf   = nullptr;
    if (io_root != root) {
        ptrdiff_t text, ext;

        ext=datatype->get_extent();
        text=datatype->get_extent();

        if ((root == rank) && (MPI_IN_PLACE == sendbuf)) {
          tmp_sendbuf = smpi_get_tmp_sendbuffer(text + (count - 1) * ext);
          if (NULL == tmp_sendbuf) {
            return MPI_ERR_INTERN;
          }
          Datatype::copy(recvbuf, count, datatype, tmp_sendbuf, count, datatype);
          use_this_sendbuf = tmp_sendbuf;
        } else if (io_root == rank) {
          tmp_recvbuf = smpi_get_tmp_recvbuffer(text + (count - 1) * ext);
          if (NULL == tmp_recvbuf) {
            return MPI_ERR_INTERN;
          }
          use_this_recvbuf = tmp_recvbuf;
        }
    }

    /* Use generic reduce with in-order binary tree topology and io_root */
    ret = smpi_coll_tuned_ompi_reduce_generic( use_this_sendbuf, use_this_recvbuf, count, datatype,
                                          op, io_root, comm,
                                          ompi_coll_tuned_topo_build_in_order_bintree(comm),
                                          segcount, 0 );
    if (MPI_SUCCESS != ret) { return ret; }

    /* Clean up */
    if (io_root != root) {
        if (root == rank) {
            /* Receive result from rank io_root to recvbuf */
            Request::recv(recvbuf, count, datatype, io_root,
                                    COLL_TAG_REDUCE, comm,
                                    MPI_STATUS_IGNORE);
            if (MPI_IN_PLACE == sendbuf) {
              smpi_free_tmp_buffer(tmp_sendbuf);
            }

        } else if (io_root == rank) {
            /* Send result from use_this_recvbuf to root */
            Request::send(use_this_recvbuf, count, datatype, root,
                                    COLL_TAG_REDUCE,
                                    comm);
            smpi_free_tmp_buffer(tmp_recvbuf);
        }
    }

    return MPI_SUCCESS;
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

/*
 *  reduce_lin_intra
 *
 *  Function:   - reduction using O(N) algorithm
 *  Accepts:    - same as MPI_Reduce()
 *  Returns:    - MPI_SUCCESS or error code
 */

int reduce__ompi_basic_linear(const void *sbuf, void *rbuf, int count,
                              MPI_Datatype dtype,
                              MPI_Op op,
                              int root,
                              MPI_Comm comm)
{
    int i, rank, size;
    ptrdiff_t true_extent, lb, extent;
    unsigned char* free_buffer  = nullptr;
    unsigned char* pml_buffer   = nullptr;
    unsigned char* inplace_temp = nullptr;
    const unsigned char* inbuf;

    /* Initialize */

    rank = comm->rank();
    size = comm->size();

    XBT_DEBUG("coll:tuned:reduce_intra_basic_linear rank %d", rank);

    /* If not root, send data to the root. */

    if (rank != root) {
        Request::send(sbuf, count, dtype, root,
                                COLL_TAG_REDUCE,
                                comm);
        return MPI_SUCCESS;
    }

    /* see discussion in ompi_coll_basic_reduce_lin_intra about
       extent and true extent */
    /* for reducing buffer allocation lengths.... */

    dtype->extent(&lb, &extent);
    true_extent = dtype->get_extent();

    if (MPI_IN_PLACE == sbuf) {
        sbuf = rbuf;
        inplace_temp = smpi_get_tmp_recvbuffer(true_extent + (count - 1) * extent);
        if (nullptr == inplace_temp) {
          return -1;
        }
        rbuf = inplace_temp - lb;
    }

    if (size > 1) {
      free_buffer = smpi_get_tmp_recvbuffer(true_extent + (count - 1) * extent);
      pml_buffer  = free_buffer - lb;
    }

    /* Initialize the receive buffer. */

    if (rank == (size - 1)) {
        Datatype::copy((char*)sbuf, count, dtype,(char*)rbuf, count, dtype);
    } else {
        Request::recv(rbuf, count, dtype, size - 1,
                                COLL_TAG_REDUCE, comm,
                                MPI_STATUS_IGNORE);
    }

    /* Loop receiving and calling reduction function (C or Fortran). */

    for (i = size - 2; i >= 0; --i) {
        if (rank == i) {
          inbuf = static_cast<const unsigned char*>(sbuf);
        } else {
            Request::recv(pml_buffer, count, dtype, i,
                                    COLL_TAG_REDUCE, comm,
                                    MPI_STATUS_IGNORE);
            inbuf = pml_buffer;
        }

        /* Perform the reduction */
        if(op!=MPI_OP_NULL) op->apply( inbuf, rbuf, &count, dtype);
    }

    if (nullptr != inplace_temp) {
      Datatype::copy(inplace_temp, count, dtype, (char*)sbuf, count, dtype);
      smpi_free_tmp_buffer(inplace_temp);
    }
    if (nullptr != free_buffer) {
      smpi_free_tmp_buffer(free_buffer);
    }

    /* All done */
    return MPI_SUCCESS;
}

/* copied function (with appropriate renaming) ends here */


}
}
