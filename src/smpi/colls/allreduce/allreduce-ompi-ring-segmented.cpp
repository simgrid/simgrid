/* Copyright (c) 2013-2021. The SimGrid Team.
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

/*
 *  ompi_coll_tuned_allreduce_intra_ring_segmented
 *
 *   Function:       Pipelined ring algorithm for allreduce operation
 *   Accepts:        Same as MPI_Allreduce(), segment size
 *   Returns:        MPI_SUCCESS or error code
 *
 *   Description:    Implements pipelined ring algorithm for allreduce:
 *                   user supplies suggested segment size for the pipelining of
 *                   reduce operation.
 *                   The segment size determines the number of phases, np, for
 *                   the algorithm execution.
 *                   The message is automatically divided into blocks of
 *                   approximately  (count / (np * segcount)) elements.
 *                   At the end of reduction phase, allgather like step is
 *                   executed.
 *                   Algorithm requires (np + 1)*(N - 1) steps.
 *
 *   Limitations:    The algorithm DOES NOT preserve order of operations so it
 *                   can be used only for commutative operations.
 *                   In addition, algorithm cannot work if the total size is
 *                   less than size * segment size.
 *         Example on 3 nodes with 2 phases
 *         Initial state
 *   #      0              1             2
 *        [00a]          [10a]         [20a]
 *        [00b]          [10b]         [20b]
 *        [01a]          [11a]         [21a]
 *        [01b]          [11b]         [21b]
 *        [02a]          [12a]         [22a]
 *        [02b]          [12b]         [22b]
 *
 *        COMPUTATION PHASE 0 (a)
 *         Step 0: rank r sends block ra to rank (r+1) and receives block (r-1)a
 *                 from rank (r-1) [with wraparound].
 *    #     0              1             2
 *        [00a]        [00a+10a]       [20a]
 *        [00b]          [10b]         [20b]
 *        [01a]          [11a]       [11a+21a]
 *        [01b]          [11b]         [21b]
 *      [22a+02a]        [12a]         [22a]
 *        [02b]          [12b]         [22b]
 *
 *         Step 1: rank r sends block (r-1)a to rank (r+1) and receives block
 *                 (r-2)a from rank (r-1) [with wraparound].
 *    #     0              1             2
 *        [00a]        [00a+10a]   [00a+10a+20a]
 *        [00b]          [10b]         [20b]
 *    [11a+21a+01a]      [11a]       [11a+21a]
 *        [01b]          [11b]         [21b]
 *      [22a+02a]    [22a+02a+12a]     [22a]
 *        [02b]          [12b]         [22b]
 *
 *        COMPUTATION PHASE 1 (b)
 *         Step 0: rank r sends block rb to rank (r+1) and receives block (r-1)b
 *                 from rank (r-1) [with wraparound].
 *    #     0              1             2
 *        [00a]        [00a+10a]       [20a]
 *        [00b]        [00b+10b]       [20b]
 *        [01a]          [11a]       [11a+21a]
 *        [01b]          [11b]       [11b+21b]
 *      [22a+02a]        [12a]         [22a]
 *      [22b+02b]        [12b]         [22b]
 *
 *         Step 1: rank r sends block (r-1)b to rank (r+1) and receives block
 *                 (r-2)b from rank (r-1) [with wraparound].
 *    #     0              1             2
 *        [00a]        [00a+10a]   [00a+10a+20a]
 *        [00b]          [10b]     [0bb+10b+20b]
 *    [11a+21a+01a]      [11a]       [11a+21a]
 *    [11b+21b+01b]      [11b]         [21b]
 *      [22a+02a]    [22a+02a+12a]     [22a]
 *        [02b]      [22b+01b+12b]     [22b]
 *
 *
 *        DISTRIBUTION PHASE: ring ALLGATHER with ranks shifted by 1 (same as
 *         in regular ring algorithm.
 *
 */

#define COLL_TUNED_COMPUTED_SEGCOUNT(SEGSIZE, TYPELNG, SEGCOUNT)        \
    if( ((SEGSIZE) >= (TYPELNG)) &&                                     \
        ((SEGSIZE) < ((TYPELNG) * (SEGCOUNT))) ) {                      \
        size_t residual;                                                \
        (SEGCOUNT) = (int)((SEGSIZE) / (TYPELNG));                      \
        residual = (SEGSIZE) - (SEGCOUNT) * (TYPELNG);                  \
        if( residual > ((TYPELNG) >> 1) )                               \
            (SEGCOUNT)++;                                               \
    }                                                                   \

#define COLL_TUNED_COMPUTE_BLOCKCOUNT( COUNT, NUM_BLOCKS, SPLIT_INDEX,       \
                                       EARLY_BLOCK_COUNT, LATE_BLOCK_COUNT ) \
    EARLY_BLOCK_COUNT = LATE_BLOCK_COUNT = COUNT / NUM_BLOCKS;               \
    SPLIT_INDEX = COUNT % NUM_BLOCKS;                                        \
    if (0 != SPLIT_INDEX) {                                                  \
        EARLY_BLOCK_COUNT = EARLY_BLOCK_COUNT + 1;                           \
    }                                                                        \

#include "../colls_private.hpp"

namespace simgrid {
namespace smpi {
int allreduce__ompi_ring_segmented(const void *sbuf, void *rbuf, int count,
                                   MPI_Datatype dtype,
                                   MPI_Op op,
                                   MPI_Comm comm)
{
   int ret = MPI_SUCCESS;
   int line;
   int k, recv_from, send_to;
   int early_blockcount, late_blockcount, split_rank;
   int segcount, max_segcount;
   int num_phases, phase;
   int block_count;
   unsigned int inbi;
   size_t typelng;
   char *tmpsend = nullptr, *tmprecv = nullptr;
   unsigned char* inbuf[2] = {nullptr, nullptr};
   ptrdiff_t true_extent, extent;
   ptrdiff_t block_offset, max_real_segsize;
   MPI_Request reqs[2]  = {nullptr, nullptr};
   const size_t segsize = 1 << 20; /* 1 MB */
   int size = comm->size();
   int rank = comm->rank();

   XBT_DEBUG("coll:tuned:allreduce_intra_ring_segmented rank %d, count %d", rank, count);

   /* Special case for size == 1 */
   if (1 == size) {
      if (MPI_IN_PLACE != sbuf) {
      ret= Datatype::copy(sbuf, count, dtype,rbuf, count, dtype);
         if (ret < 0) { line = __LINE__; goto error_hndl; }
      }
      return MPI_SUCCESS;
   }

   /* Determine segment count based on the suggested segment size */
   extent = dtype->get_extent();
   if (MPI_SUCCESS != ret) { line = __LINE__; goto error_hndl; }
   true_extent = dtype->get_extent();
   if (MPI_SUCCESS != ret) { line = __LINE__; goto error_hndl; }
   typelng = dtype->size();
   if (MPI_SUCCESS != ret) { line = __LINE__; goto error_hndl; }
   segcount = count;
   COLL_TUNED_COMPUTED_SEGCOUNT(segsize, typelng, segcount)

   /* Special case for count less than size * segcount - use regular ring */
   if (count < size * segcount) {
      XBT_DEBUG( "coll:tuned:allreduce_ring_segmented rank %d/%d, count %d, switching to regular ring", rank, size, count);
      return (allreduce__lr(sbuf, rbuf, count, dtype, op, comm));
   }

   /* Determine the number of phases of the algorithm */
   num_phases = count / (size * segcount);
   if ((count % (size * segcount) >= size) &&
       (count % (size * segcount) > ((size * segcount) / 2))) {
      num_phases++;
   }

   /* Determine the number of elements per block and corresponding
      block sizes.
      The blocks are divided into "early" and "late" ones:
      blocks 0 .. (split_rank - 1) are "early" and
      blocks (split_rank) .. (size - 1) are "late".
      Early blocks are at most 1 element larger than the late ones.
      Note, these blocks will be split into num_phases segments,
      out of the largest one will have max_segcount elements.
    */
   COLL_TUNED_COMPUTE_BLOCKCOUNT( count, size, split_rank,
                                  early_blockcount, late_blockcount )
   COLL_TUNED_COMPUTE_BLOCKCOUNT( early_blockcount, num_phases, inbi,
                                  max_segcount, k)
   max_real_segsize = true_extent + (max_segcount - 1) * extent;

   /* Allocate and initialize temporary buffers */
   inbuf[0] = smpi_get_tmp_sendbuffer(max_real_segsize);
   if (nullptr == inbuf[0]) {
     ret  = -1;
     line = __LINE__;
     goto error_hndl;
   }
   if (size > 2) {
     inbuf[1] = smpi_get_tmp_recvbuffer(max_real_segsize);
     if (nullptr == inbuf[1]) {
       ret  = -1;
       line = __LINE__;
       goto error_hndl;
     }
   }

   /* Handle MPI_IN_PLACE */
   if (MPI_IN_PLACE != sbuf) {
      ret= Datatype::copy(sbuf, count, dtype,rbuf, count, dtype);
      if (ret < 0) { line = __LINE__; goto error_hndl; }
   }

   /* Computation loop: for each phase, repeat ring allreduce computation loop */
   for (phase = 0; phase < num_phases; phase ++) {
      ptrdiff_t phase_offset;
      int early_phase_segcount, late_phase_segcount, split_phase, phase_count;

      /*
         For each of the remote nodes:
         - post irecv for block (r-1)
         - send block (r)
           To do this, first compute block offset and count, and use block offset
           to compute phase offset.
         - in loop for every step k = 2 .. n
           - post irecv for block (r + n - k) % n
           - wait on block (r + n - k + 1) % n to arrive
           - compute on block (r + n - k + 1) % n
           - send block (r + n - k + 1) % n
         - wait on block (r + 1)
         - compute on block (r + 1)
         - send block (r + 1) to rank (r + 1)
         Note that we must be careful when computing the beginning of buffers and
         for send operations and computation we must compute the exact block size.
      */
      send_to = (rank + 1) % size;
      recv_from = (rank + size - 1) % size;

      inbi = 0;
      /* Initialize first receive from the neighbor on the left */
      reqs[inbi] = Request::irecv(inbuf[inbi], max_segcount, dtype, recv_from,
                               666, comm);
      /* Send first block (my block) to the neighbor on the right:
         - compute my block and phase offset
         - send data */
      block_offset = ((rank < split_rank)?
                      (rank * early_blockcount) :
                      (rank * late_blockcount + split_rank));
      block_count = ((rank < split_rank)? early_blockcount : late_blockcount);
      COLL_TUNED_COMPUTE_BLOCKCOUNT(block_count, num_phases, split_phase,
                                    early_phase_segcount, late_phase_segcount)
      phase_count = ((phase < split_phase)?
                     (early_phase_segcount) : (late_phase_segcount));
      phase_offset = ((phase < split_phase)?
                      (phase * early_phase_segcount) :
                      (phase * late_phase_segcount + split_phase));
      tmpsend = ((char*)rbuf) + (block_offset + phase_offset) * extent;
      Request::send(tmpsend, phase_count, dtype, send_to,
                              666, comm);

      for (k = 2; k < size; k++) {
         const int prevblock = (rank + size - k + 1) % size;

         inbi = inbi ^ 0x1;

         /* Post irecv for the current block */
         reqs[inbi] = Request::irecv(inbuf[inbi], max_segcount, dtype, recv_from,
                               666, comm);
         if (MPI_SUCCESS != ret) { line = __LINE__; goto error_hndl; }

         /* Wait on previous block to arrive */
         Request::wait(&reqs[inbi ^ 0x1], MPI_STATUS_IGNORE);

         /* Apply operation on previous block: result goes to rbuf
            rbuf[prevblock] = inbuf[inbi ^ 0x1] (op) rbuf[prevblock]
         */
         block_offset = ((prevblock < split_rank)?
                         (prevblock * early_blockcount) :
                         (prevblock * late_blockcount + split_rank));
         block_count = ((prevblock < split_rank)?
                        early_blockcount : late_blockcount);
         COLL_TUNED_COMPUTE_BLOCKCOUNT(block_count, num_phases, split_phase,
                                       early_phase_segcount, late_phase_segcount)
         phase_count = ((phase < split_phase)?
                        (early_phase_segcount) : (late_phase_segcount));
         phase_offset = ((phase < split_phase)?
                         (phase * early_phase_segcount) :
                         (phase * late_phase_segcount + split_phase));
         tmprecv = ((char*)rbuf) + (block_offset + phase_offset) * extent;
         if(op!=MPI_OP_NULL) op->apply( inbuf[inbi ^ 0x1], tmprecv, &phase_count, dtype);
         /* send previous block to send_to */
         Request::send(tmprecv, phase_count, dtype, send_to,
                              666, comm);
      }

      /* Wait on the last block to arrive */
      Request::wait(&reqs[inbi], MPI_STATUS_IGNORE);


      /* Apply operation on the last block (from neighbor (rank + 1)
         rbuf[rank+1] = inbuf[inbi] (op) rbuf[rank + 1] */
      recv_from = (rank + 1) % size;
      block_offset = ((recv_from < split_rank)?
                      (recv_from * early_blockcount) :
                      (recv_from * late_blockcount + split_rank));
      block_count = ((recv_from < split_rank)?
                     early_blockcount : late_blockcount);
      COLL_TUNED_COMPUTE_BLOCKCOUNT(block_count, num_phases, split_phase,
                                    early_phase_segcount, late_phase_segcount)
      phase_count = ((phase < split_phase)?
                     (early_phase_segcount) : (late_phase_segcount));
      phase_offset = ((phase < split_phase)?
                      (phase * early_phase_segcount) :
                      (phase * late_phase_segcount + split_phase));
      tmprecv = ((char*)rbuf) + (block_offset + phase_offset) * extent;
      if(op!=MPI_OP_NULL) op->apply( inbuf[inbi], tmprecv, &phase_count, dtype);
   }

   /* Distribution loop - variation of ring allgather */
   send_to = (rank + 1) % size;
   recv_from = (rank + size - 1) % size;
   for (k = 0; k < size - 1; k++) {
      const int recv_data_from = (rank + size - k) % size;
      const int send_data_from = (rank + 1 + size - k) % size;
      const int send_block_offset =
         ((send_data_from < split_rank)?
          (send_data_from * early_blockcount) :
          (send_data_from * late_blockcount + split_rank));
      const int recv_block_offset =
         ((recv_data_from < split_rank)?
          (recv_data_from * early_blockcount) :
          (recv_data_from * late_blockcount + split_rank));
      block_count = ((send_data_from < split_rank)?
                     early_blockcount : late_blockcount);

      tmprecv = (char*)rbuf + recv_block_offset * extent;
      tmpsend = (char*)rbuf + send_block_offset * extent;

      Request::sendrecv(tmpsend, block_count, dtype, send_to,
                                     666,
                                     tmprecv, early_blockcount, dtype, recv_from,
                                     666,
                                     comm, MPI_STATUS_IGNORE);

   }

   smpi_free_tmp_buffer(inbuf[0]);
   smpi_free_tmp_buffer(inbuf[1]);

   return MPI_SUCCESS;

 error_hndl:
   XBT_DEBUG("%s:%4d\tRank %d Error occurred %d\n",
                __FILE__, line, rank, ret);
   smpi_free_tmp_buffer(inbuf[0]);
   smpi_free_tmp_buffer(inbuf[1]);
   return ret;
}
}
}
