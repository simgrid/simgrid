/* Asynchronous parts of the basic collective algorithms, meant to be used both for the naive default implementation, but also for non blocking collectives */

/* Copyright (c) 2009-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "colls_private.hpp"
#include "src/smpi/include/smpi_actor.hpp"

namespace simgrid{
namespace smpi{


int Colls::Ibarrier(MPI_Comm comm, MPI_Request* request)
{
  int i;
  int size = comm->size();
  int rank = comm->rank();
  MPI_Request* requests;
  (*request) = new Request( nullptr, 0, MPI_BYTE,
                         rank,rank, COLL_TAG_BARRIER, comm, MPI_REQ_NON_PERSISTENT);
  (*request)->ref();
  if (rank > 0) {
    requests = new MPI_Request[2];
    requests[0] = Request::isend (nullptr, 0, MPI_BYTE, 0,
                             COLL_TAG_BARRIER,
                             comm);
    requests[1] = Request::irecv (nullptr, 0, MPI_BYTE, 0,
                             COLL_TAG_BARRIER,
                             comm);
    (*request)->set_nbc_requests(requests, 2);
  }
  else {
    requests = new MPI_Request[(size-1)*2];
    for (i = 1; i < 2*size-1; i+=2) {
        requests[i-1] = Request::irecv(nullptr, 0, MPI_BYTE, MPI_ANY_SOURCE,
                                 COLL_TAG_BARRIER, comm
                                 );
        requests[i] = Request::isend(nullptr, 0, MPI_BYTE, (i+1)/2,
                                 COLL_TAG_BARRIER,
                                 comm
                                 );
    }
    (*request)->set_nbc_requests(requests, 2*(size-1));
  }
  return MPI_SUCCESS;
}

}
}
