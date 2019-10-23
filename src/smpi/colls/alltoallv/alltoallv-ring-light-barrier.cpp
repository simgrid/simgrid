/* Copyright (c) 2013-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"
/*****************************************************************************

 * Function: alltoall_ring_light_barrier

 * Return: int

 * Inputs:
    send_buff: send input buffer
    send_count: number of elements to send
    send_type: data type of elements being sent
    recv_buff: receive output buffer
    recv_count: number of elements to received
    recv_type: data type of elements being received
    comm: communicator

 * Descrp: Function works in P - 1 steps. In step i, node j - i -> j -> j + i.
           Light barriers are inserted between communications in different
           phases.

 * Author: Ahmad Faraj

 ****************************************************************************/
namespace simgrid{
namespace smpi{
int
Coll_alltoallv_ring_light_barrier::alltoallv(const void *send_buff, const int *send_counts, const int *send_disps,
                                            MPI_Datatype send_type,
                                            void *recv_buff, const int *recv_counts, const int *recv_disps,
                                            MPI_Datatype recv_type,
                                            MPI_Comm comm)
{
  MPI_Aint send_chunk, recv_chunk;
  MPI_Status s;
  int i, src, dst, rank, num_procs, next_dst, next_src;
  int tag = COLL_TAG_ALLTOALLV;

  char send_sync = 'a', recv_sync = 'b';
  char *send_ptr = (char *) send_buff;
  char *recv_ptr = (char *) recv_buff;

  rank = comm->rank();
  num_procs = comm->size();
  send_chunk = send_type->get_extent();
  recv_chunk = recv_type->get_extent();

  Request::sendrecv(send_ptr + send_disps[rank] * send_chunk, send_counts[rank], send_type, rank, tag,
               recv_ptr + recv_disps[rank] * recv_chunk, recv_counts[rank], recv_type, rank, tag,
               comm, &s);

  for (i = 1; i < num_procs; i++) {
    src = (rank - i + num_procs) % num_procs;
    dst = (rank + i) % num_procs;

    Request::sendrecv(send_ptr + send_disps[dst] * send_chunk, send_counts[dst], send_type,
                 dst, tag, recv_ptr + recv_disps[src] * recv_chunk, recv_counts[src],
                 recv_type, src, tag, comm, &s);

    if ((i + 1) < num_procs) {
      next_src = (rank - (i + 1) + num_procs) % num_procs;
      next_dst = (rank + (i + 1) + num_procs) % num_procs;
      Request::sendrecv(&send_sync, 1, MPI_CHAR, next_src, tag,
                   &recv_sync, 1, MPI_CHAR, next_dst, tag, comm, &s);

    }
  }

  return MPI_SUCCESS;
}
}
}
