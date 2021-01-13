/* Copyright (c) 2013-2021. The SimGrid Team.
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
 * Function: is_3dmesh
 * return: bool
 * num: the number of processors in a communicator
 * i: x dimension
 * j: y dimension
 * k: z dimension
 * descp: takes a number and tries to find a factoring of x*y*z mesh out of it
 ****************************************************************************/
#ifndef THREED
#define THREED
static bool is_3dmesh(int num, int* i, int* j, int* k)
{
  int x, max = num / 3;
  x = cbrt(num);
  *i = *j = *k = 0;
  while (x <= max) {
    if ((num % (x * x)) == 0) {
      *i = *j = x;
      *k = num / (x * x);
      return true;
    }
    x++;
  }
  return false;
}
#endif
/*****************************************************************************
 * Function: allgather_3dmesh_shoot
 * return: int
 * send_buff: send input buffer
 * send_count: number of elements to send
 * send_type: data type of elements being sent
 * recv_buff: receive output buffer
 * recv_count: number of elements to received
 * recv_type: data type of elements being received
 * comm: communication
 * Descrp: Function realizes the allgather operation using the 2dmesh
 * algorithm. Allgather ommunication occurs first in the x dimension, y
 * dimension, and then in the z dimension. Communication in each dimension
 * follows "simple"
 * Author: Ahmad Faraj
****************************************************************************/
namespace simgrid{
namespace smpi{


int allgather__3dmesh(const void *send_buff, int send_count,
                      MPI_Datatype send_type, void *recv_buff,
                      int recv_count, MPI_Datatype recv_type,
                      MPI_Comm comm)
{
  MPI_Aint extent;

  int i, src, dst, rank, num_procs, block_size, my_z_base;
  int my_z, X, Y, Z, send_offset, recv_offset;
  int two_dsize, my_row_base, my_col_base, src_row_base, src_z_base, num_reqs;
  int tag = COLL_TAG_ALLGATHER;

  rank = comm->rank();
  num_procs = comm->size();
  extent = send_type->get_extent();

  if (not is_3dmesh(num_procs, &X, &Y, &Z))
    throw std::invalid_argument("allgather_3dmesh algorithm can't be used with this number of processes!");

  num_reqs = X;

  if (Y > X)
    num_reqs = Y;
  if (Z > Y)
    num_reqs = Z;

  two_dsize = X * Y;
  my_z = rank / two_dsize;

  my_row_base = (rank / X) * X;
  my_col_base = (rank % Y) + (my_z * two_dsize);
  my_z_base = my_z * two_dsize;

  block_size = extent * send_count;

  auto* req            = new MPI_Request[num_reqs];
  MPI_Request* req_ptr = req;

  // do local allgather/local copy
  recv_offset = rank * block_size;
  Datatype::copy(send_buff, send_count, send_type, (char *)recv_buff + recv_offset,
                 recv_count, recv_type);

  // do rowwise comm
  for (i = 0; i < Y; i++) {
    src = i + my_row_base;
    if (src == rank)
      continue;
    recv_offset = src * block_size;
    *(req_ptr++) = Request::irecv((char *)recv_buff + recv_offset, send_count, recv_type, src, tag,
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

  // do colwise comm, it does not matter here if i*X or i *Y since X == Y

  for (i = 0; i < X; i++) {
    src = (i * Y + my_col_base);
    if (src == rank)
      continue;

    src_row_base = (src / X) * X;
    recv_offset = src_row_base * block_size;
    *(req_ptr++) = Request::irecv((char *)recv_buff + recv_offset, recv_count * Y, recv_type, src, tag,
               comm);
  }

  send_offset = my_row_base * block_size;

  for (i = 0; i < X; i++) {
    dst = (i * Y + my_col_base);
    if (dst == rank)
      continue;
    Request::send((char *)recv_buff + send_offset, send_count * Y, send_type, dst, tag,
              comm);
  }

  Request::waitall(X - 1, req, MPI_STATUSES_IGNORE);
  req_ptr = req;

  for (i = 1; i < Z; i++) {
    src = (rank + i * two_dsize) % num_procs;
    src_z_base = (src / two_dsize) * two_dsize;

    recv_offset = (src_z_base * block_size);

    *(req_ptr++) = Request::irecv((char *)recv_buff + recv_offset, recv_count * two_dsize, recv_type,
               src, tag, comm);
  }

  for (i = 1; i < Z; i++) {
    dst = (rank + i * two_dsize) % num_procs;
    send_offset = my_z_base * block_size;
    Request::send((char *)recv_buff + send_offset, send_count * two_dsize, send_type,
              dst, tag, comm);
  }
  Request::waitall(Z - 1, req, MPI_STATUSES_IGNORE);

  delete[] req;

  return MPI_SUCCESS;
}


}
}
