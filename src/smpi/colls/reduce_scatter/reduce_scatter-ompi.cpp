/* Copyright (c) 2013-2021. The SimGrid Team.
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

#include "../coll_tuned_topo.hpp"
#include "../colls_private.hpp"

/*
 * Recursive-halving function is (*mostly*) copied from the BASIC coll module.
 * I have removed the part which handles "large" message sizes
 * (non-overlapping version of reduce_Scatter).
 */

/* copied function (with appropriate renaming) starts here */

/*
 *  reduce_scatter_ompi_basic_recursivehalving
 *
 *  Function:   - reduce scatter implementation using recursive-halving
 *                algorithm
 *  Accepts:    - same as MPI_Reduce_scatter()
 *  Returns:    - MPI_SUCCESS or error code
 *  Limitation: - Works only for commutative operations.
 */
namespace simgrid {
namespace smpi {
int reduce_scatter__ompi_basic_recursivehalving(const void *sbuf,
                                                void *rbuf,
                                                const int *rcounts,
                                                MPI_Datatype dtype,
                                                MPI_Op op,
                                                MPI_Comm comm
                                                )
{
    int i, rank, size, count, err = MPI_SUCCESS;
    int tmp_size = 1, remain = 0, tmp_rank;
    ptrdiff_t true_lb, true_extent, lb, extent, buf_size;
    unsigned char *result_buf = nullptr, *result_buf_free = nullptr;

    /* Initialize */
    rank = comm->rank();
    size = comm->size();

    XBT_DEBUG("coll:tuned:reduce_scatter_ompi_basic_recursivehalving, rank %d", rank);
    if ((op != MPI_OP_NULL && not op->is_commutative()))
      throw std::invalid_argument(
          "reduce_scatter ompi_basic_recursivehalving can only be used for commutative operations!");

    /* Find displacements and the like */
    int* disps = new int[size];

    disps[0] = 0;
    for (i = 0; i < (size - 1); ++i) {
        disps[i + 1] = disps[i] + rcounts[i];
    }
    count = disps[size - 1] + rcounts[size - 1];

    /* short cut the trivial case */
    if (0 == count) {
      delete[] disps;
      return MPI_SUCCESS;
    }

    /* get datatype information */
    dtype->extent(&lb, &extent);
    dtype->extent(&true_lb, &true_extent);
    buf_size = true_extent + (ptrdiff_t)(count - 1) * extent;

    /* Handle MPI_IN_PLACE */
    if (MPI_IN_PLACE == sbuf) {
        sbuf = rbuf;
    }

    /* Allocate temporary receive buffer. */
    unsigned char* recv_buf_free = smpi_get_tmp_recvbuffer(buf_size);
    unsigned char* recv_buf      = recv_buf_free - lb;
    if (nullptr == recv_buf_free) {
      err = MPI_ERR_OTHER;
      goto cleanup;
    }

    /* allocate temporary buffer for results */
    result_buf_free = smpi_get_tmp_sendbuffer(buf_size);
    result_buf = result_buf_free - lb;

    /* copy local buffer into the temporary results */
    err =Datatype::copy(sbuf, count, dtype, result_buf, count, dtype);
    if (MPI_SUCCESS != err) goto cleanup;

    /* figure out power of two mapping: grow until larger than
       comm size, then go back one, to get the largest power of
       two less than comm size */
    while (tmp_size <= size) tmp_size <<= 1;
    tmp_size >>= 1;
    remain = size - tmp_size;

    /* If comm size is not a power of two, have the first "remain"
       procs with an even rank send to rank + 1, leaving a power of
       two procs to do the rest of the algorithm */
    if (rank < 2 * remain) {
        if ((rank & 1) == 0) {
            Request::send(result_buf, count, dtype, rank + 1,
                                    COLL_TAG_REDUCE_SCATTER,
                                    comm);
            /* we don't participate from here on out */
            tmp_rank = -1;
        } else {
            Request::recv(recv_buf, count, dtype, rank - 1,
                                    COLL_TAG_REDUCE_SCATTER,
                                    comm, MPI_STATUS_IGNORE);

            /* integrate their results into our temp results */
            if(op!=MPI_OP_NULL) op->apply( recv_buf, result_buf, &count, dtype);

            /* adjust rank to be the bottom "remain" ranks */
            tmp_rank = rank / 2;
        }
    } else {
        /* just need to adjust rank to show that the bottom "even
           remain" ranks dropped out */
        tmp_rank = rank - remain;
    }

    /* For ranks not kicked out by the above code, perform the
       recursive halving */
    if (tmp_rank >= 0) {
        int mask, send_index, recv_index, last_index;

        /* recalculate disps and rcounts to account for the
           special "remainder" processes that are no longer doing
           anything */
        int* tmp_rcounts = new int[tmp_size];
        int* tmp_disps   = new int[tmp_size];

        for (i = 0 ; i < tmp_size ; ++i) {
            if (i < remain) {
                /* need to include old neighbor as well */
                tmp_rcounts[i] = rcounts[i * 2 + 1] + rcounts[i * 2];
            } else {
                tmp_rcounts[i] = rcounts[i + remain];
            }
        }

        tmp_disps[0] = 0;
        for (i = 0; i < tmp_size - 1; ++i) {
            tmp_disps[i + 1] = tmp_disps[i] + tmp_rcounts[i];
        }

        /* do the recursive halving communication.  Don't use the
           dimension information on the communicator because I
           think the information is invalidated by our "shrinking"
           of the communicator */
        mask = tmp_size >> 1;
        send_index = recv_index = 0;
        last_index = tmp_size;
        while (mask > 0) {
            int tmp_peer, peer, send_count, recv_count;
            MPI_Request request;

            tmp_peer = tmp_rank ^ mask;
            peer = (tmp_peer < remain) ? tmp_peer * 2 + 1 : tmp_peer + remain;

            /* figure out if we're sending, receiving, or both */
            send_count = recv_count = 0;
            if (tmp_rank < tmp_peer) {
                send_index = recv_index + mask;
                for (i = send_index ; i < last_index ; ++i) {
                    send_count += tmp_rcounts[i];
                }
                for (i = recv_index ; i < send_index ; ++i) {
                    recv_count += tmp_rcounts[i];
                }
            } else {
                recv_index = send_index + mask;
                for (i = send_index ; i < recv_index ; ++i) {
                    send_count += tmp_rcounts[i];
                }
                for (i = recv_index ; i < last_index ; ++i) {
                    recv_count += tmp_rcounts[i];
                }
            }

            /* actual data transfer.  Send from result_buf,
               receive into recv_buf */
            if (send_count > 0 && recv_count != 0) {
                request=Request::irecv(recv_buf + (ptrdiff_t)tmp_disps[recv_index] * extent,
                                         recv_count, dtype, peer,
                                         COLL_TAG_REDUCE_SCATTER,
                                         comm);
                if (MPI_SUCCESS != err) {
                  delete[] tmp_rcounts;
                  delete[] tmp_disps;
                  goto cleanup;
                }
            }
            if (recv_count > 0 && send_count != 0) {
                Request::send(result_buf + (ptrdiff_t)tmp_disps[send_index] * extent,
                                        send_count, dtype, peer,
                                        COLL_TAG_REDUCE_SCATTER,
                                        comm);
                if (MPI_SUCCESS != err) {
                  delete[] tmp_rcounts;
                  delete[] tmp_disps;
                  goto cleanup;
                }
            }
            if (send_count > 0 && recv_count != 0) {
                Request::wait(&request, MPI_STATUS_IGNORE);
            }

            /* if we received something on this step, push it into
               the results buffer */
            if (recv_count > 0) {
                if(op!=MPI_OP_NULL) op->apply(
                               recv_buf + (ptrdiff_t)tmp_disps[recv_index] * extent,
                               result_buf + (ptrdiff_t)tmp_disps[recv_index] * extent,
                               &recv_count, dtype);
            }

            /* update for next iteration */
            send_index = recv_index;
            last_index = recv_index + mask;
            mask >>= 1;
        }

        /* copy local results from results buffer into real receive buffer */
        if (0 != rcounts[rank]) {
            err = Datatype::copy(result_buf + disps[rank] * extent,
                                       rcounts[rank], dtype,
                                       rbuf, rcounts[rank], dtype);
            if (MPI_SUCCESS != err) {
              delete[] tmp_rcounts;
              delete[] tmp_disps;
              goto cleanup;
            }
        }

        delete[] tmp_rcounts;
        delete[] tmp_disps;
    }

    /* Now fix up the non-power of two case, by having the odd
       procs send the even procs the proper results */
    if (rank < (2 * remain)) {
        if ((rank & 1) == 0) {
            if (rcounts[rank]) {
                Request::recv(rbuf, rcounts[rank], dtype, rank + 1,
                                        COLL_TAG_REDUCE_SCATTER,
                                        comm, MPI_STATUS_IGNORE);
            }
        } else {
            if (rcounts[rank - 1]) {
                Request::send(result_buf + disps[rank - 1] * extent,
                                        rcounts[rank - 1], dtype, rank - 1,
                                        COLL_TAG_REDUCE_SCATTER,
                                        comm);
            }
        }
    }

 cleanup:
    delete[] disps;
    if (nullptr != recv_buf_free)
      smpi_free_tmp_buffer(recv_buf_free);
    if (nullptr != result_buf_free)
      smpi_free_tmp_buffer(result_buf_free);

    return err;
}

/* copied function (with appropriate renaming) ends here */


/*
 *   Coll_reduce_scatter_ompi_ring::reduce_scatter
 *
 *   Function:       Ring algorithm for reduce_scatter operation
 *   Accepts:        Same as MPI_Reduce_scatter()
 *   Returns:        MPI_SUCCESS or error code
 *
 *   Description:    Implements ring algorithm for reduce_scatter:
 *                   the block sizes defined in rcounts are exchanged and
 8                    updated until they reach proper destination.
 *                   Algorithm requires 2 * max(rcounts) extra buffering
 *
 *   Limitations:    The algorithm DOES NOT preserve order of operations so it
 *                   can be used only for commutative operations.
 *         Example on 5 nodes:
 *         Initial state
 *   #      0              1             2              3             4
 *        [00]           [10]   ->     [20]           [30]           [40]
 *        [01]           [11]          [21]  ->       [31]           [41]
 *        [02]           [12]          [22]           [32]  ->       [42]
 *    ->  [03]           [13]          [23]           [33]           [43] --> ..
 *        [04]  ->       [14]          [24]           [34]           [44]
 *
 *        COMPUTATION PHASE
 *         Step 0: rank r sends block (r-1) to rank (r+1) and
 *                 receives block (r+1) from rank (r-1) [with wraparound].
 *   #      0              1             2              3             4
 *        [00]           [10]        [10+20]   ->     [30]           [40]
 *        [01]           [11]          [21]          [21+31]  ->     [41]
 *    ->  [02]           [12]          [22]           [32]         [32+42] -->..
 *      [43+03] ->       [13]          [23]           [33]           [43]
 *        [04]         [04+14]  ->     [24]           [34]           [44]
 *
 *         Step 1:
 *   #      0              1             2              3             4
 *        [00]           [10]        [10+20]       [10+20+30] ->     [40]
 *    ->  [01]           [11]          [21]          [21+31]      [21+31+41] ->
 *     [32+42+02] ->     [12]          [22]           [32]         [32+42]
 *        [03]        [43+03+13] ->    [23]           [33]           [43]
 *        [04]         [04+14]      [04+14+24]  ->    [34]           [44]
 *
 *         Step 2:
 *   #      0              1             2              3             4
 *     -> [00]           [10]        [10+20]       [10+20+30]   [10+20+30+40] ->
 *   [21+31+41+01]->     [11]          [21]          [21+31]      [21+31+41]
 *     [32+42+02]   [32+42+02+12]->    [22]           [32]         [32+42]
 *        [03]        [43+03+13]   [43+03+13+23]->    [33]           [43]
 *        [04]         [04+14]      [04+14+24]    [04+14+24+34] ->   [44]
 *
 *         Step 3:
 *   #      0             1              2              3             4
 * [10+20+30+40+00]     [10]         [10+20]       [10+20+30]   [10+20+30+40]
 *  [21+31+41+01] [21+31+41+01+11]     [21]          [21+31]      [21+31+41]
 *    [32+42+02]   [32+42+02+12] [32+42+02+12+22]     [32]         [32+42]
 *       [03]        [43+03+13]    [43+03+13+23] [43+03+13+23+33]    [43]
 *       [04]         [04+14]       [04+14+24]    [04+14+24+34] [04+14+24+34+44]
 *    DONE :)
 *
 */
int reduce_scatter__ompi_ring(const void *sbuf, void *rbuf, const int *rcounts,
                              MPI_Datatype dtype,
                              MPI_Op op,
                              MPI_Comm comm
                              )
{
    int ret, line, rank, size, i, k, recv_from, send_to, total_count, max_block_count;
    int inbi;
    unsigned char *tmpsend = nullptr, *tmprecv = nullptr, *accumbuf = nullptr, *accumbuf_free = nullptr;
    unsigned char *inbuf_free[2] = {nullptr, nullptr}, *inbuf[2] = {nullptr, nullptr};
    ptrdiff_t true_lb, true_extent, lb, extent, max_real_segsize;
    MPI_Request reqs[2] = {nullptr, nullptr};

    size = comm->size();
    rank = comm->rank();

    XBT_DEBUG(  "coll:tuned:reduce_scatter_ompi_ring rank %d, size %d",
                 rank, size);

    /* Determine the maximum number of elements per node,
       corresponding block size, and displacements array.
    */
    int* displs = new int[size];

    displs[0] = 0;
    total_count = rcounts[0];
    max_block_count = rcounts[0];
    for (i = 1; i < size; i++) {
        displs[i] = total_count;
        total_count += rcounts[i];
        if (max_block_count < rcounts[i]) max_block_count = rcounts[i];
    }

    /* Special case for size == 1 */
    if (1 == size) {
        if (MPI_IN_PLACE != sbuf) {
            ret = Datatype::copy((char*)sbuf, total_count, dtype, (char*)rbuf, total_count, dtype);
            if (ret < 0) { line = __LINE__; goto error_hndl; }
        }
        delete[] displs;
        return MPI_SUCCESS;
    }

    /* Allocate and initialize temporary buffers, we need:
       - a temporary buffer to perform reduction (size total_count) since
       rbuf can be of rcounts[rank] size.
       - up to two temporary buffers used for communication/computation overlap.
    */
    dtype->extent(&lb, &extent);
    dtype->extent(&true_lb, &true_extent);

    max_real_segsize = true_extent + (ptrdiff_t)(max_block_count - 1) * extent;

    accumbuf_free = smpi_get_tmp_recvbuffer(true_extent + (ptrdiff_t)(total_count - 1) * extent);
    if (nullptr == accumbuf_free) {
      ret  = -1;
      line = __LINE__;
      goto error_hndl;
    }
    accumbuf = accumbuf_free - lb;

    inbuf_free[0] = smpi_get_tmp_sendbuffer(max_real_segsize);
    if (nullptr == inbuf_free[0]) {
      ret  = -1;
      line = __LINE__;
      goto error_hndl;
    }
    inbuf[0] = inbuf_free[0] - lb;
    if (size > 2) {
      inbuf_free[1] = smpi_get_tmp_sendbuffer(max_real_segsize);
      if (nullptr == inbuf_free[1]) {
        ret  = -1;
        line = __LINE__;
        goto error_hndl;
      }
      inbuf[1] = inbuf_free[1] - lb;
    }

    /* Handle MPI_IN_PLACE for size > 1 */
    if (MPI_IN_PLACE == sbuf) {
        sbuf = rbuf;
    }

    ret = Datatype::copy((char*)sbuf, total_count, dtype, accumbuf, total_count, dtype);
    if (ret < 0) { line = __LINE__; goto error_hndl; }

    /* Computation loop */

    /*
       For each of the remote nodes:
       - post irecv for block (r-2) from (r-1) with wrap around
       - send block (r-1) to (r+1)
       - in loop for every step k = 2 .. n
       - post irecv for block (r - 1 + n - k) % n
       - wait on block (r + n - k) % n to arrive
       - compute on block (r + n - k ) % n
       - send block (r + n - k) % n
       - wait on block (r)
       - compute on block (r)
       - copy block (r) to rbuf
       Note that we must be careful when computing the beginning of buffers and
       for send operations and computation we must compute the exact block size.
    */
    send_to = (rank + 1) % size;
    recv_from = (rank + size - 1) % size;

    inbi = 0;
    /* Initialize first receive from the neighbor on the left */
    reqs[inbi]=Request::irecv(inbuf[inbi], max_block_count, dtype, recv_from,
                             COLL_TAG_REDUCE_SCATTER, comm
                             );
    tmpsend = accumbuf + (ptrdiff_t)displs[recv_from] * extent;
    Request::send(tmpsend, rcounts[recv_from], dtype, send_to,
                            COLL_TAG_REDUCE_SCATTER,
                             comm);

    for (k = 2; k < size; k++) {
        const int prevblock = (rank + size - k) % size;

        inbi = inbi ^ 0x1;

        /* Post irecv for the current block */
        reqs[inbi]=Request::irecv(inbuf[inbi], max_block_count, dtype, recv_from,
                                 COLL_TAG_REDUCE_SCATTER, comm
                                 );

        /* Wait on previous block to arrive */
        Request::wait(&reqs[inbi ^ 0x1], MPI_STATUS_IGNORE);

        /* Apply operation on previous block: result goes to rbuf
           rbuf[prevblock] = inbuf[inbi ^ 0x1] (op) rbuf[prevblock]
        */
        tmprecv = accumbuf + (ptrdiff_t)displs[prevblock] * extent;
        if (op != MPI_OP_NULL)
          op->apply(inbuf[inbi ^ 0x1], tmprecv, &rcounts[prevblock], dtype);

        /* send previous block to send_to */
        Request::send(tmprecv, rcounts[prevblock], dtype, send_to,
                                COLL_TAG_REDUCE_SCATTER,
                                 comm);
    }

    /* Wait on the last block to arrive */
    Request::wait(&reqs[inbi], MPI_STATUS_IGNORE);

    /* Apply operation on the last block (my block)
       rbuf[rank] = inbuf[inbi] (op) rbuf[rank] */
    tmprecv = accumbuf + (ptrdiff_t)displs[rank] * extent;
    if (op != MPI_OP_NULL)
      op->apply(inbuf[inbi], tmprecv, &rcounts[rank], dtype);

    /* Copy result from tmprecv to rbuf */
    ret = Datatype::copy(tmprecv, rcounts[rank], dtype, (char*)rbuf, rcounts[rank], dtype);
    if (ret < 0) { line = __LINE__; goto error_hndl; }

    delete[] displs;
    if (nullptr != accumbuf_free)
      smpi_free_tmp_buffer(accumbuf_free);
    if (nullptr != inbuf_free[0])
      smpi_free_tmp_buffer(inbuf_free[0]);
    if (nullptr != inbuf_free[1])
      smpi_free_tmp_buffer(inbuf_free[1]);

    return MPI_SUCCESS;

 error_hndl:
    XBT_DEBUG( "%s:%4d\tRank %d Error occurred %d\n",
                 __FILE__, line, rank, ret);
    delete[] displs;
    if (nullptr != accumbuf_free)
      smpi_free_tmp_buffer(accumbuf_free);
    if (nullptr != inbuf_free[0])
      smpi_free_tmp_buffer(inbuf_free[0]);
    if (nullptr != inbuf_free[1])
      smpi_free_tmp_buffer(inbuf_free[1]);
    return ret;
}
}
}

