#include "colls.h"

/*****************************************************************************

Copyright (c) 2006, Ahmad Faraj & Xin Yuan,
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

  * Neither the name of the Florida State University nor the names of its
    contributors may be used to endorse or promote products derived from this
    software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  *************************************************************************
  *     Any results obtained from executing this software require the     *
  *     acknowledgment and citation of the software and its owners.       *
  *     The full citation is given below:                                 *
  *                                                                       *
  *     A. Faraj and X. Yuan. "Automatic Generation and Tuning of MPI     *
  *     Collective Communication Routines." The 19th ACM International    *
  *     Conference on Supercomputing (ICS), Cambridge, Massachusetts,     *
  *     June 20-22, 2005.                                                 *
  *************************************************************************

*****************************************************************************/

/*****************************************************************************

 * Function: bcast_scatter_rdb_allgather

 * Return: int

 * Inputs:
    buff: send input buffer
    count: number of elements to send
    data_type: data type of elements being sent
    root: source of data
    comm: communicator

 * Descrp: broadcasts using a scatter followed by rdb allgather.

 * Auther: MPICH / modified by Ahmad Faraj

 ****************************************************************************/

int
smpi_coll_tuned_bcast_scatter_rdb_allgather(void *buff, int count, MPI_Datatype
                                            data_type, int root, MPI_Comm comm)
{
  MPI_Aint extent;
  MPI_Status status;

  int i, j, k, src, dst, rank, num_procs, send_offset, recv_offset;
  int mask, relative_rank, curr_size, recv_size, send_size, nbytes;
  int scatter_size, tree_root, relative_dst, dst_tree_root;
  int my_tree_root, offset, tmp_mask, num_procs_completed;
  int tag = 1;

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &num_procs);
  MPI_Type_extent(data_type, &extent);

  nbytes = extent * count;
  scatter_size = (nbytes + num_procs - 1) / num_procs;  // ceiling division 
  curr_size = (rank == root) ? nbytes : 0;      // root starts with all the data
  relative_rank = (rank >= root) ? rank - root : rank - root + num_procs;

  mask = 0x1;
  while (mask < num_procs) {
    if (relative_rank & mask) {
      src = rank - mask;
      if (src < 0)
        src += num_procs;
      recv_size = nbytes - relative_rank * scatter_size;
      //  recv_size is larger than what might actually be sent by the
      //  sender. We don't need compute the exact value because MPI
      //  allows you to post a larger recv.
      if (recv_size <= 0)
        curr_size = 0;          // this process doesn't receive any data
      // because of uneven division 
      else {
        MPI_Recv((char *)buff + relative_rank * scatter_size, recv_size,
                 MPI_BYTE, src, tag, comm, &status);
        MPI_Get_count(&status, MPI_BYTE, &curr_size);
      }
      break;
    }
    mask <<= 1;
  }

  // This process is responsible for all processes that have bits
  // set from the LSB upto (but not including) mask.  Because of
  // the "not including", we start by shifting mask back down
  // one.

  mask >>= 1;
  while (mask > 0) {
    if (relative_rank + mask < num_procs) {
      send_size = curr_size - scatter_size * mask;
      // mask is also the size of this process's subtree 

      if (send_size > 0) {
        dst = rank + mask;
        if (dst >= num_procs)
          dst -= num_procs;
        MPI_Send((char *)buff + scatter_size * (relative_rank + mask),
                 send_size, MPI_BYTE, dst, tag, comm);

        curr_size -= send_size;
      }
    }
    mask >>= 1;
  }

  // done scatter now do allgather


  mask = 0x1;
  i = 0;
  while (mask < num_procs) {
    relative_dst = relative_rank ^ mask;

    dst = (relative_dst + root) % num_procs;

    /* find offset into send and recv buffers.
       zero out the least significant "i" bits of relative_rank and
       relative_dst to find root of src and dst
       subtrees. Use ranks of roots as index to send from
       and recv into  buffer */

    dst_tree_root = relative_dst >> i;
    dst_tree_root <<= i;

    my_tree_root = relative_rank >> i;
    my_tree_root <<= i;

    send_offset = my_tree_root * scatter_size;
    recv_offset = dst_tree_root * scatter_size;

    if (relative_dst < num_procs) {
      MPI_Sendrecv((char *)buff + send_offset, curr_size, MPI_BYTE, dst, tag,
                   (char *)buff + recv_offset, scatter_size * mask, MPI_BYTE, dst,
                   tag, comm, &status);
      MPI_Get_count(&status, MPI_BYTE, &recv_size);
      curr_size += recv_size;
    }

    /* if some processes in this process's subtree in this step
       did not have any destination process to communicate with
       because of non-power-of-two, we need to send them the
       data that they would normally have received from those
       processes. That is, the haves in this subtree must send to
       the havenots. We use a logarithmic recursive-halfing algorithm
       for this. */

    if (dst_tree_root + mask > num_procs) {
      num_procs_completed = num_procs - my_tree_root - mask;
      /* num_procs_completed is the number of processes in this
         subtree that have all the data. Send data to others
         in a tree fashion. First find root of current tree
         that is being divided into two. k is the number of
         least-significant bits in this process's rank that
         must be zeroed out to find the rank of the root */
      j = mask;
      k = 0;
      while (j) {
        j >>= 1;
        k++;
      }
      k--;

      offset = scatter_size * (my_tree_root + mask);
      tmp_mask = mask >> 1;

      while (tmp_mask) {
        relative_dst = relative_rank ^ tmp_mask;
        dst = (relative_dst + root) % num_procs;

        tree_root = relative_rank >> k;
        tree_root <<= k;

        /* send only if this proc has data and destination
           doesn't have data. */

        if ((relative_dst > relative_rank)
            && (relative_rank < tree_root + num_procs_completed)
            && (relative_dst >= tree_root + num_procs_completed)) {
          MPI_Send((char *)buff + offset, recv_size, MPI_BYTE, dst, tag, comm);

          /* recv_size was set in the previous
             receive. that's the amount of data to be
             sent now. */
        }
        /* recv only if this proc. doesn't have data and sender
           has data */
        else if ((relative_dst < relative_rank)
                 && (relative_dst < tree_root + num_procs_completed)
                 && (relative_rank >= tree_root + num_procs_completed)) {

          MPI_Recv((char *)buff + offset, scatter_size * num_procs_completed,
                   MPI_BYTE, dst, tag, comm, &status);

          /* num_procs_completed is also equal to the no. of processes
             whose data we don't have */
          MPI_Get_count(&status, MPI_BYTE, &recv_size);
          curr_size += recv_size;
        }
        tmp_mask >>= 1;
        k--;
      }
    }
    mask <<= 1;
    i++;
  }

  return MPI_SUCCESS;
}
