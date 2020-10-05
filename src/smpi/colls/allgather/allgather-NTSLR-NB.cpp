/* Copyright (c) 2013-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"

namespace simgrid{
namespace smpi{

// Allgather-Non-Topology-Specific-Logical-Ring algorithm
int
allgather__NTSLR_NB(const void *sbuf, int scount, MPI_Datatype stype,
                    void *rbuf, int rcount, MPI_Datatype rtype,
                    MPI_Comm comm)
{
  MPI_Aint rextent, sextent;
  MPI_Status status, status2;
  int i, to, from, rank, size;
  int send_offset, recv_offset;
  int tag = COLL_TAG_ALLGATHER;

  rank = comm->rank();
  size = comm->size();
  rextent = rtype->get_extent();
  sextent = stype->get_extent();
  auto* rrequest_array = new MPI_Request[size];
  auto* srequest_array = new MPI_Request[size];

  // irregular case use default MPI functions
  if (scount * sextent != rcount * rextent) {
    XBT_WARN("MPI_allgather_NTSLR_NB use default MPI_allgather.");
    allgather__default(sbuf, scount, stype, rbuf, rcount, rtype, comm);
    return MPI_SUCCESS;
  }

  // topo non-specific
  to = (rank + 1) % size;
  from = (rank + size - 1) % size;

  //copy a single segment from sbuf to rbuf
  send_offset = rank * scount * sextent;

  Request::sendrecv(sbuf, scount, stype, rank, tag,
               (char *)rbuf + send_offset, rcount, rtype, rank, tag, comm, &status);


  //start sending logical ring message
  int increment = scount * sextent;

  //post all irecv first
  for (i = 0; i < size - 1; i++) {
    recv_offset = ((rank - i - 1 + size) % size) * increment;
    rrequest_array[i] = Request::irecv((char *)rbuf + recv_offset, rcount, rtype, from, tag + i, comm);
  }


  for (i = 0; i < size - 1; i++) {
    send_offset = ((rank - i + size) % size) * increment;
    srequest_array[i] = Request::isend((char *)rbuf + send_offset, scount, stype, to, tag + i, comm);
    Request::wait(&rrequest_array[i], &status);
    Request::wait(&srequest_array[i], &status2);
  }

  delete[] rrequest_array;
  delete[] srequest_array;

  return MPI_SUCCESS;
}

}
}
