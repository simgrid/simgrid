/* Copyright (c) 2013-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"

int flattree_segment_in_byte = 8192;
namespace simgrid {
namespace smpi {
int bcast__flattree_pipeline(void *buff, int count,
                             MPI_Datatype data_type, int root,
                             MPI_Comm comm)
{
  int i, j, rank, num_procs;
  int tag = COLL_TAG_BCAST;

  MPI_Aint extent;
  extent = data_type->get_extent();

  int segment = flattree_segment_in_byte / extent;
  segment =  segment == 0 ? 1 :segment;
  int pipe_length = count / segment;
  int increment = segment * extent;
  if (pipe_length==0) {
    XBT_INFO("MPI_bcast_flattree_pipeline: pipe_length=0, use default MPI_bcast_flattree.");
    return bcast__flattree(buff, count, data_type, root, comm);
  }
  rank = comm->rank();
  num_procs = comm->size();

  auto* request_array = new MPI_Request[pipe_length];
  auto* status_array  = new MPI_Status[pipe_length];

  if (rank != root) {
    for (i = 0; i < pipe_length; i++) {
      request_array[i] = Request::irecv((char *)buff + (i * increment), segment, data_type, root, tag, comm);
    }
    Request::waitall(pipe_length, request_array, status_array);
  }

  else {
    // Root sends data to all others
    for (j = 0; j < num_procs; j++) {
      if (j == rank)
        continue;
      else {
        for (i = 0; i < pipe_length; i++) {
          Request::send((char *)buff + (i * increment), segment, data_type, j, tag, comm);
        }
      }
    }

  }

  delete[] request_array;
  delete[] status_array;
  return MPI_SUCCESS;
}

}
}
