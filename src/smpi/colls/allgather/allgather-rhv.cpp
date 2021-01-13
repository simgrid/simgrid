/* Copyright (c) 2013-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"

namespace simgrid{
namespace smpi{


// now only work with power of two processes

int
allgather__rhv(const void *sbuf, int send_count,
               MPI_Datatype send_type, void *rbuf,
               int recv_count, MPI_Datatype recv_type,
               MPI_Comm comm)
{
  MPI_Status status;
  MPI_Aint s_extent, r_extent;

  // local int variables
  int i, dst, send_base_offset, recv_base_offset, send_chunk, recv_chunk,
      send_offset, recv_offset;
  int tag = COLL_TAG_ALLGATHER;
  unsigned int mask;
  int curr_count;

  // get size of the communicator, followed by rank
  unsigned int num_procs = comm->size();

  if((num_procs&(num_procs-1)))
    throw std::invalid_argument("allgather rhv algorithm can't be used with non power of two number of processes!");

  unsigned int rank = comm->rank();

  // get size of single element's type for send buffer and recv buffer
  s_extent = send_type->get_extent();
  r_extent = recv_type->get_extent();

  // multiply size of each element by number of elements to send or recv
  send_chunk = s_extent * send_count;
  recv_chunk = r_extent * recv_count;

  if (send_chunk != recv_chunk) {
    XBT_WARN("MPI_allgather_rhv use default MPI_allgather.");
    allgather__default(sbuf, send_count, send_type, rbuf, recv_count,
                       recv_type, comm);
    return MPI_SUCCESS;
  }

  // compute starting offset location to perform local copy
  int size = num_procs / 2;
  int base_offset = 0;
  mask = 1;
  while (mask < num_procs) {
    if (rank & mask) {
      base_offset += size;
    }
    mask <<= 1;
    size /= 2;
  }

  //  printf("node %d base_offset %d\n",rank,base_offset);

  //perform a remote copy

  dst = base_offset;
  Request::sendrecv(sbuf, send_count, send_type, dst, tag,
               (char *)rbuf + base_offset * recv_chunk, recv_count, recv_type, dst, tag,
               comm, &status);


  mask >>= 1;
  i = 1;
  int phase = 0;
  curr_count = recv_count;
  while (mask >= 1) {
    // destination pair for both send and recv
    dst = rank ^ mask;

    // compute offsets
    send_base_offset = base_offset;
    if (rank & mask) {
      recv_base_offset = base_offset - i;
      base_offset -= i;
    } else {
      recv_base_offset = base_offset + i;
    }
    send_offset = send_base_offset * recv_chunk;
    recv_offset = recv_base_offset * recv_chunk;

    //  printf("node %d send to %d in phase %d s_offset = %d r_offset = %d count = %d\n",rank,dst,phase, send_base_offset, recv_base_offset, curr_count);

    Request::sendrecv((char*)rbuf + send_offset, curr_count, recv_type, dst, tag, (char*)rbuf + recv_offset, curr_count,
                      recv_type, dst, tag, comm, &status);

    curr_count *= 2;
    i *= 2;
    mask >>= 1;
    phase++;
  }

  return MPI_SUCCESS;
}


}
}
