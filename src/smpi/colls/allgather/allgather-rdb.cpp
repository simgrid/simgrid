/* Copyright (c) 2013-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"
#include "smpi_status.hpp"

namespace simgrid{
namespace smpi{

int
allgather__rdb(const void *sbuf, int send_count,
               MPI_Datatype send_type, void *rbuf,
               int recv_count, MPI_Datatype recv_type,
               MPI_Comm comm)
{
  // MPI variables
  MPI_Status status;
  MPI_Aint send_chunk, recv_chunk;

  // local int variables
  unsigned int i, j, k, dst, send_offset, recv_offset, tree_root;
  int dst_tree_root, rank_tree_root, last_recv_count = 0, num_procs_completed;
  int offset, tmp_mask;
  int tag = COLL_TAG_ALLGATHER;
  unsigned int mask = 1;
  int success = 0;
  int curr_count = recv_count;

  // local string variables
  char *send_ptr = (char *) sbuf;
  char *recv_ptr = (char *) rbuf;

  // get size of the communicator, followed by rank
  unsigned int num_procs = comm->size();
  unsigned int rank = comm->rank();

  // get size of single element's type for send buffer and recv buffer
  send_chunk = send_type->get_extent();
  recv_chunk = recv_type->get_extent();

  // multiply size of each element by number of elements to send or recv
  send_chunk *= send_count;
  recv_chunk *= recv_count;

  // perform a local copy
  Request::sendrecv(send_ptr, send_count, send_type, rank, tag,
               recv_ptr + rank * recv_chunk, recv_count, recv_type, rank, tag,
               comm, &status);

  i = 0;
  while (mask < num_procs) {
    dst = rank ^ mask;
    dst_tree_root = dst >> i;
    dst_tree_root <<= i;
    rank_tree_root = rank >> i;
    rank_tree_root <<= i;
    send_offset = rank_tree_root * send_chunk;
    recv_offset = dst_tree_root * recv_chunk;

    if (dst < num_procs) {
      Request::sendrecv(recv_ptr + send_offset, curr_count, send_type, dst,
                   tag, recv_ptr + recv_offset, mask * recv_count,
                   recv_type, dst, tag, comm, &status);
      last_recv_count = Status::get_count(&status, recv_type);
      curr_count += last_recv_count;
    }

    if (dst_tree_root + mask > num_procs) {
      num_procs_completed = num_procs - rank_tree_root - mask;
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

      offset = recv_chunk * (rank_tree_root + mask);
      tmp_mask = mask >> 1;

      while (tmp_mask) {
        dst = rank ^ tmp_mask;

        tree_root = rank >> k;
        tree_root <<= k;

        /* send only if this proc has data and destination
           doesn't have data. at any step, multiple processes
           can send if they have the data */
        if ((dst > rank)
            && (rank < tree_root + num_procs_completed)
            && (dst >= tree_root + num_procs_completed)) {
          Request::send(recv_ptr + offset, last_recv_count, recv_type, dst,
                   tag, comm);

          /* last_recv_cnt was set in the previous
             receive. that's the amount of data to be
             sent now. */
        }
        /* recv only if this proc. doesn't have data and sender
           has data */
        else if ((dst < rank)
                 && (dst < tree_root + num_procs_completed)
                 && (rank >= tree_root + num_procs_completed)) {
          Request::recv(recv_ptr + offset,
                   recv_count * num_procs_completed,
                   recv_type, dst, tag, comm, &status);
          // num_procs_completed is also equal to the no. of processes
          // whose data we don't have
          last_recv_count = Status::get_count(&status, recv_type);
          curr_count += last_recv_count;
        }
        tmp_mask >>= 1;
        k--;
      }
    }

    mask <<= 1;
    i++;
  }

  return success;
}


}
}
