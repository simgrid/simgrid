/* Copyright (c) 2013-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"
#include "smpi_status.hpp"

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

 * Function: bcast_scatter_LR_allgather

 * Return: int

 * Inputs:
    buff: send input buffer
    count: number of elements to send
    data_type: data type of elements being sent
    root: source of data
    comm: communicator

 * Descrp: broadcasts using a scatter followed by LR allgather.

 * Author: MPIH / modified by Ahmad Faraj

 ****************************************************************************/
namespace simgrid {
namespace smpi {
int bcast__scatter_LR_allgather(void *buff, int count,
                                MPI_Datatype data_type, int root,
                                MPI_Comm comm)
{
  MPI_Aint extent;
  MPI_Status status;
  int i, src, dst, rank, num_procs;
  int mask, relative_rank, curr_size, recv_size, send_size, nbytes;
  int scatter_size, left, right, next_src;
  int tag = COLL_TAG_BCAST;

  rank = comm->rank();
  num_procs = comm->size();
  extent = data_type->get_extent();


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
        Request::recv((char *) buff + relative_rank * scatter_size, recv_size,
                 MPI_BYTE, src, tag, comm, &status);
        curr_size = Status::get_count(&status, MPI_BYTE);
      }
      break;
    }
    mask <<= 1;
  }

  // This process is responsible for all processes that have bits
  // set from the LSB up to (but not including) mask.  Because of
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
        Request::send((char *) buff + scatter_size * (relative_rank + mask),
                 send_size, MPI_BYTE, dst, tag, comm);

        curr_size -= send_size;
      }
    }
    mask >>= 1;
  }

  // done scatter now do allgather
  int* recv_counts = new int[num_procs];
  int* disps       = new int[num_procs];

  for (i = 0; i < num_procs; i++) {
    recv_counts[i] = nbytes - i * scatter_size;
    if (recv_counts[i] > scatter_size)
      recv_counts[i] = scatter_size;
    if (recv_counts[i] < 0)
      recv_counts[i] = 0;
  }

  disps[0] = 0;
  for (i = 1; i < num_procs; i++)
    disps[i] = disps[i - 1] + recv_counts[i - 1];

  left = (num_procs + rank - 1) % num_procs;
  right = (rank + 1) % num_procs;

  src = rank;
  next_src = left;

  for (i = 1; i < num_procs; i++) {
    Request::sendrecv((char *) buff + disps[(src - root + num_procs) % num_procs],
                 recv_counts[(src - root + num_procs) % num_procs],
                 MPI_BYTE, right, tag,
                 (char *) buff +
                 disps[(next_src - root + num_procs) % num_procs],
                 recv_counts[(next_src - root + num_procs) % num_procs],
                 MPI_BYTE, left, tag, comm, &status);
    src = next_src;
    next_src = (num_procs + next_src - 1) % num_procs;
  }

  delete[] recv_counts;
  delete[] disps;

  return MPI_SUCCESS;
}

}
}
