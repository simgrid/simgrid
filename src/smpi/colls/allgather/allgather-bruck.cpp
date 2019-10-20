/* Copyright (c) 2013-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"

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
 * Function: allgather_bruck
 * return: int
 * inputs:
 *   send_buff: send input buffer
 *   send_count: number of elements to send
 *   send_type: data type of elements being sent
 *   recv_buff: receive output buffer
 *   recv_count: number of elements to received
 *   recv_type: data type of elements being received
 *   comm: communication
 * Descrp: Function realizes the allgather operation using the bruck
 *         algorithm.
 * Author: MPICH
 * Comment: Original bruck algorithm from MPICH is slightly modified by
 *          Ahmad Faraj.
 ****************************************************************************/

namespace simgrid{
namespace smpi{



int Coll_allgather_bruck::allgather(const void *send_buff, int send_count,
                                    MPI_Datatype send_type, void *recv_buff,
                                    int recv_count, MPI_Datatype recv_type,
                                    MPI_Comm comm)
{
  // MPI variables
  MPI_Status status;
  MPI_Aint recv_extent;

  // local int variables
  int src, dst, rank, num_procs, count, remainder;
  int tag = COLL_TAG_ALLGATHER;
  int pof2 = 1;

  // local string variables
  char *send_ptr = (char *) send_buff;
  char *recv_ptr = (char *) recv_buff;

  // get size of the communicator, followed by rank
  num_procs = comm->size();
  rank = comm->rank();

  // get size of single element's type for recv buffer
  recv_extent = recv_type->get_extent();

  count = recv_count;

  unsigned char* tmp_buff = smpi_get_tmp_sendbuffer(num_procs * recv_count * recv_extent);

  // perform a local copy
  Datatype::copy(send_ptr, send_count, send_type, tmp_buff, recv_count, recv_type);
  while (pof2 <= (num_procs / 2)) {
    src = (rank + pof2) % num_procs;
    dst = (rank - pof2 + num_procs) % num_procs;

    Request::sendrecv(tmp_buff, count, recv_type, dst, tag,
                  tmp_buff + count * recv_extent, count, recv_type,
                  src, tag, comm, &status);
    count *= 2;
    pof2 *= 2;
  }

  remainder = num_procs - pof2;
  if (remainder) {
    src = (rank + pof2) % num_procs;
    dst = (rank - pof2 + num_procs) % num_procs;

    Request::sendrecv(tmp_buff, remainder * recv_count, recv_type, dst, tag,
                  tmp_buff + count * recv_extent, remainder * recv_count,
                  recv_type, src, tag, comm, &status);
  }

  Request::sendrecv(tmp_buff, (num_procs - rank) * recv_count, recv_type, rank,
                tag, recv_ptr + rank * recv_count * recv_extent,
                (num_procs - rank) * recv_count, recv_type, rank, tag, comm,
                &status);

  if (rank)
    Request::sendrecv(tmp_buff + (num_procs - rank) * recv_count * recv_extent,
                  rank * recv_count, recv_type, rank, tag, recv_ptr,
                  rank * recv_count, recv_type, rank, tag, comm, &status);
  smpi_free_tmp_buffer(tmp_buff);
  return MPI_SUCCESS;
}


}
}
