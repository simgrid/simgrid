/* Copyright (c) 2013-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"
#include "smpi_win.hpp"

/*****************************************************************************

 * Function: alltoall_pair

 * Return: int

 * Inputs:
    send_buff: send input buffer
    send_count: number of elements to send
    send_type: data type of elements being sent
    recv_buff: receive output buffer
    recv_count: number of elements to received
    recv_type: data type of elements being received
    comm: communicator

 * Descrp: Function works when P is power of two. In each phase of P - 1
           phases, nodes in pair communicate their data.

 * Author: Ahmad Faraj

 ****************************************************************************/
namespace simgrid{
namespace smpi{
int alltoall__pair_rma(const void *send_buff, int send_count, MPI_Datatype send_type,
                       void *recv_buff, int recv_count, MPI_Datatype recv_type,
                       MPI_Comm comm)
{

  MPI_Aint send_chunk, recv_chunk;
  MPI_Win win;
  int assert = 0;
  int i, dst, rank, num_procs;

  char *send_ptr = (char *) send_buff;

  rank = comm->rank();
  num_procs = comm->size();
  send_chunk = send_type->get_extent();
  recv_chunk = recv_type->get_extent();

  win=new  Win(recv_buff, num_procs * recv_chunk * send_count, recv_chunk, 0,
                 comm);
  send_chunk *= send_count;
  recv_chunk *= recv_count;

  win->fence(assert);
  for (i = 0; i < num_procs; i++) {
    dst = rank ^ i;
    win->put(send_ptr + dst * send_chunk, send_count, send_type, dst,
            rank /* send_chunk*/, send_count, send_type);
  }
  win->fence(assert);
  delete win;
  return 0;
}


int alltoall__pair(const void *send_buff, int send_count,
                   MPI_Datatype send_type,
                   void *recv_buff, int recv_count,
                   MPI_Datatype recv_type, MPI_Comm comm)
{

  MPI_Aint send_chunk, recv_chunk;
  MPI_Status s;
  int i, src, dst, rank, num_procs;
  int tag = COLL_TAG_ALLTOALL;
  char *send_ptr = (char *) send_buff;
  char *recv_ptr = (char *) recv_buff;

  rank = comm->rank();
  num_procs = comm->size();

  if((num_procs&(num_procs-1)))
    throw std::invalid_argument("alltoall pair algorithm can't be used with non power of two number of processes!");

  send_chunk = send_type->get_extent();
  recv_chunk = recv_type->get_extent();

  send_chunk *= send_count;
  recv_chunk *= recv_count;

  for (i = 0; i < num_procs; i++) {
    src = dst = rank ^ i;
    Request::sendrecv(send_ptr + dst * send_chunk, send_count, send_type, dst, tag, recv_ptr + src * recv_chunk,
                      recv_count, recv_type, src, tag, comm, &s);
  }

  return MPI_SUCCESS;
}
}
}
