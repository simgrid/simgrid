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
 * Function: allgather_ring
 * return: int
 * inputs:
 *   send_buff: send input buffer
 *   send_count: number of elements to send
 *   send_type: data type of elements being sent
 *   recv_buff: receive output buffer
 *   recv_count: number of elements to received
 *   recv_type: data type of elements being received
 *   comm: communication
 * Descrp: Function works in P - 1 steps. In step i, node j - i -> j -> j+ i.
 * Auther: Ahmad Faraj
 ****************************************************************************/

namespace simgrid{
namespace smpi{

int
Coll_allgatherv_ring::allgatherv(const void *send_buff, int send_count,
                               MPI_Datatype send_type, void *recv_buff,
                               const int *recv_counts, const int *recv_disps, MPI_Datatype recv_type,
                               MPI_Comm comm)
{

  MPI_Aint extent;
  int i, src, dst, rank, num_procs;
  int tag = COLL_TAG_ALLGATHERV;
  MPI_Status status;

  char *sendptr = (char *) send_buff;
  char *recvptr = (char *) recv_buff;

  rank = comm->rank();
  num_procs = comm->size();
  extent = send_type->get_extent();

  // local send/recv
  Request::sendrecv(sendptr, send_count, send_type, rank, tag,
               recvptr + recv_disps[rank] * extent,
               recv_counts[rank], recv_type, rank, tag, comm, &status);

  for (i = 1; i < num_procs; i++) {
    src = (rank - i + num_procs) % num_procs;
    dst = (rank + i) % num_procs;

    Request::sendrecv(sendptr, send_count, send_type, dst, tag,
                 recvptr + recv_disps[src] * extent, recv_counts[src], recv_type,
                 src, tag, comm, &status);
  }

  return MPI_SUCCESS;
}

}
}
