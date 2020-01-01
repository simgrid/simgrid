/* Copyright (c) 2013-2020. The SimGrid Team.
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

 * Function: is_2dmesh

 * Return: bool

 * Inputs:
     num: the number of processors in a communicator
     i: x dimension
     j: y dimension

 * Descp: takes a number and tries to find a factoring of x, y mesh out of it

 * Author: Ahmad Faraj
 ****************************************************************************/
#ifndef TWOD
#define TWOD
static bool is_2dmesh(int num, int* i, int* j)
{
  int x, max = num / 2;
  x = sqrt(double(num));

  while (x <= max) {
    if ((num % x) == 0) {
      *i = x;
      *j = num / x;

      if (*i > *j) {
        x = *i;
        *i = *j;
        *j = x;
      }

      return true;
    }
    x++;
  }
  return false;
}
#endif
/*****************************************************************************
 * Function: allgather_2dmesh_shoot
 * return: int
 * send_buff: send input buffer
 * send_count: number of elements to send
 * send_type: data type of elements being sent
 * recv_buff: receive output buffer
 * recv_count: number of elements to received
 * recv_type: data type of elements being received
 * comm: communication
 * Descrp: Function realizes the allgather operation using the 2dmesh
 * algorithm. Allgather ommunication occurs first in the x dimension then in
 * the y dimension.  The communication in each dimension follows
 * "simple"
 * Author: Ahmad Faraj
****************************************************************************/

namespace simgrid{
namespace smpi{


int
allgather__2dmesh(const void *send_buff, int send_count, MPI_Datatype
                  send_type, void *recv_buff, int recv_count,
                  MPI_Datatype recv_type, MPI_Comm comm)
{
  MPI_Aint extent;

  int i, src, dst, rank, num_procs;
  int X, Y, send_offset, recv_offset;
  int my_row_base, my_col_base, src_row_base, block_size, num_reqs;
  int tag = COLL_TAG_ALLGATHER;

  rank = comm->rank();
  num_procs = comm->size();

  extent = send_type->get_extent();

  block_size = extent * send_count;

  if (not is_2dmesh(num_procs, &X, &Y))
    throw std::invalid_argument("allgather_2dmesh algorithm can't be used with this number of processes!");

  my_row_base = (rank / Y) * Y;
  my_col_base = rank % Y;

  num_reqs = X;
  if (Y > X)
    num_reqs = Y;

  MPI_Request* req     = new MPI_Request[num_reqs];
  MPI_Request* req_ptr = req;

  // do local allgather/local copy
  recv_offset = rank * block_size;
  Datatype::copy(send_buff, send_count, send_type, (char *)recv_buff + recv_offset,
                 recv_count, recv_type);

  // do row-wise comm
  for (i = 0; i < Y; i++) {
    src = i + my_row_base;
    if (src == rank)
      continue;
    recv_offset = src * block_size;
    *(req_ptr++) = Request::irecv((char *)recv_buff + recv_offset, recv_count, recv_type, src, tag,
               comm);
  }


  for (i = 0; i < Y; i++) {
    dst = i + my_row_base;
    if (dst == rank)
      continue;
    Request::send(send_buff, send_count, send_type, dst, tag, comm);
  }

  Request::waitall(Y - 1, req, MPI_STATUSES_IGNORE);

  req_ptr = req;

  // do colwise comm
  for (i = 0; i < X; i++) {
    src = (i * Y + my_col_base);
    if (src == rank)
      continue;
    src_row_base = (src / Y) * Y;
    recv_offset = src_row_base * block_size;
    *(req_ptr++) = Request::irecv((char *)recv_buff + recv_offset, recv_count * Y, recv_type, src, tag,
               comm);
  }

  for (i = 0; i < X; i++) {
    dst = (i * Y + my_col_base);
    if (dst == rank)
      continue;
    send_offset = my_row_base * block_size;
    Request::send((char *)recv_buff + send_offset, send_count * Y, send_type, dst, tag,
              comm);
  }

  Request::waitall(X - 1, req, MPI_STATUSES_IGNORE);

  delete[] req;

  return MPI_SUCCESS;
}

}
}
