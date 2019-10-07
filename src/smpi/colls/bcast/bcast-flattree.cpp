/* Copyright (c) 2013-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"
namespace simgrid{
namespace smpi{
int
Coll_bcast_flattree::bcast(void *buff, int count, MPI_Datatype data_type,
                               int root, MPI_Comm comm)
{
  int i, rank, num_procs;
  int tag = COLL_TAG_BCAST;

  rank = comm->rank();
  num_procs = comm->size();

  if (rank != root) {
    Request::recv(buff, count, data_type, root, tag, comm, MPI_STATUS_IGNORE);
  }

  else {
    MPI_Request* reqs    = new MPI_Request[num_procs - 1];
    MPI_Request* req_ptr = reqs;

    // Root sends data to all others
    for (i = 0; i < num_procs; i++) {
      if (i == rank)
        continue;
      *(req_ptr++) = Request::isend(buff, count, data_type, i, tag, comm);
    }

    // wait on all requests
    Request::waitall(num_procs - 1, reqs, MPI_STATUSES_IGNORE);

    delete[] reqs;
  }
  return MPI_SUCCESS;
}

}
}
