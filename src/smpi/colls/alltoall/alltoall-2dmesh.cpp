/* Copyright (c) 2013-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"
#include <cmath>

/*****************************************************************************

 * Function: alltoall_2dmesh_shoot

 * Return: int

 * Inputs:
    send_buff: send input buffer
    send_count: number of elements to send
    send_type: data type of elements being sent
    recv_buff: receive output buffer
    recv_count: number of elements to received
    recv_type: data type of elements being received
    comm: communicator

 * Descrp: Function realizes the alltoall operation using the 2dmesh
           algorithm. It actually performs allgather operation in x dimension
           then in the y dimension. Each node then extracts the needed data.
           The communication in each dimension follows "simple."

 * Auther: Ahmad Faraj

****************************************************************************/
static int alltoall_check_is_2dmesh(int num, int *i, int *j)
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

      return 1;
    }
    x++;
  }
  return 0;
}
namespace simgrid{
namespace smpi{

int Coll_alltoall_2dmesh::alltoall(const void *send_buff, int send_count,
                                    MPI_Datatype send_type,
                                    void *recv_buff, int recv_count,
                                    MPI_Datatype recv_type, MPI_Comm comm)
{
  MPI_Status s;
  MPI_Aint extent;

  int i, j, src, dst, rank, num_procs, count, num_reqs;
  int X, Y, send_offset, recv_offset;
  int my_row_base, my_col_base, src_row_base, block_size;
  int tag = COLL_TAG_ALLTOALL;

  rank = comm->rank();
  num_procs = comm->size();
  extent = send_type->get_extent();

  if (not alltoall_check_is_2dmesh(num_procs, &X, &Y))
    return MPI_ERR_OTHER;

  my_row_base = (rank / Y) * Y;
  my_col_base = rank % Y;

  block_size = extent * send_count;

  unsigned char* tmp_buff1 = smpi_get_tmp_sendbuffer(block_size * num_procs * Y);
  unsigned char* tmp_buff2 = smpi_get_tmp_recvbuffer(block_size * Y);

  num_reqs = X;
  if (Y > X)
    num_reqs = Y;

  MPI_Status* statuses = new MPI_Status[num_reqs];
  MPI_Request* reqs    = new MPI_Request[num_reqs];
  MPI_Request* req_ptr = reqs;

  count = send_count * num_procs;

  for (i = 0; i < Y; i++) {
    src = i + my_row_base;
    if (src == rank)
      continue;

    recv_offset = (src % Y) * block_size * num_procs;
    *(req_ptr++) = Request::irecv(tmp_buff1 + recv_offset, count, recv_type, src, tag, comm);
  }

  for (i = 0; i < Y; i++) {
    dst = i + my_row_base;
    if (dst == rank)
      continue;
    Request::send(send_buff, count, send_type, dst, tag, comm);
  }

  Request::waitall(Y - 1, reqs, statuses);
  req_ptr = reqs;

  for (i = 0; i < Y; i++) {
    send_offset = (rank * block_size) + (i * block_size * num_procs);
    recv_offset = (my_row_base * block_size) + (i * block_size);

    if (i + my_row_base == rank)
      Request::sendrecv((char *) send_buff + recv_offset, send_count, send_type,
                   rank, tag,
                   (char *) recv_buff + recv_offset, recv_count, recv_type,
                   rank, tag, comm, &s);

    else
      Request::sendrecv(tmp_buff1 + send_offset, send_count, send_type,
                   rank, tag,
                   (char *) recv_buff + recv_offset, recv_count, recv_type,
                   rank, tag, comm, &s);
  }


  for (i = 0; i < X; i++) {
    src = (i * Y + my_col_base);
    if (src == rank)
      continue;
    src_row_base = (src / Y) * Y;

    *(req_ptr++) = Request::irecv((char *) recv_buff + src_row_base * block_size, recv_count * Y,
              recv_type, src, tag, comm);
  }

  for (i = 0; i < X; i++) {
    dst = (i * Y + my_col_base);
    if (dst == rank)
      continue;

    recv_offset = 0;
    for (j = 0; j < Y; j++) {
      send_offset = (dst + j * num_procs) * block_size;

      if (j + my_row_base == rank)
        Request::sendrecv((char *) send_buff + dst * block_size, send_count,
                     send_type, rank, tag, tmp_buff2 + recv_offset, recv_count,
                     recv_type, rank, tag, comm, &s);
      else
        Request::sendrecv(tmp_buff1 + send_offset, send_count, send_type,
                     rank, tag,
                     tmp_buff2 + recv_offset, recv_count, recv_type,
                     rank, tag, comm, &s);

      recv_offset += block_size;
    }

    Request::send(tmp_buff2, send_count * Y, send_type, dst, tag, comm);
  }
  Request::waitall(X - 1, reqs, statuses);
  delete[] reqs;
  delete[] statuses;
  smpi_free_tmp_buffer(tmp_buff1);
  smpi_free_tmp_buffer(tmp_buff2);
  return MPI_SUCCESS;
}
}
}
