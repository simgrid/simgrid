/* Copyright (c) 2013-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"

/*Naive and simple basic alltoall implementation. */


namespace simgrid{
namespace smpi{


int alltoall__basic_linear(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                           void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
  int system_tag = COLL_TAG_ALLTOALL;
  int i;
  int count;
  MPI_Aint lb = 0, sendext = 0, recvext = 0;

  /* Initialize. */
  int rank = comm->rank();
  int size = comm->size();
  XBT_DEBUG("<%d> algorithm alltoall_basic_linear() called.", rank);
  sendtype->extent(&lb, &sendext);
  recvtype->extent(&lb, &recvext);
  /* simple optimization */
  int err = Datatype::copy(static_cast<const char *>(sendbuf) + rank * sendcount * sendext, sendcount, sendtype,
                               static_cast<char *>(recvbuf) + rank * recvcount * recvext, recvcount, recvtype);
  if (err == MPI_SUCCESS && size > 1) {
    /* Initiate all send/recv to/from others. */
    auto* requests = new MPI_Request[2 * (size - 1)];
    /* Post all receives first -- a simple optimization */
    count = 0;
    for (i = (rank + 1) % size; i != rank; i = (i + 1) % size) {
      requests[count] = Request::irecv_init(static_cast<char *>(recvbuf) + i * recvcount * recvext, recvcount,
                                        recvtype, i, system_tag, comm);
      count++;
    }
    /* Now post all sends in reverse order
     *   - We would like to minimize the search time through message queue
     *     when messages actually arrive in the order in which they were posted.
     * TODO: check the previous assertion
     */
    for (i = (rank + size - 1) % size; i != rank; i = (i + size - 1) % size) {
      requests[count] = Request::isend_init(static_cast<const char *>(sendbuf) + i * sendcount * sendext, sendcount,
                                        sendtype, i, system_tag, comm);
      count++;
    }
    /* Wait for them all. */
    Request::startall(count, requests);
    XBT_DEBUG("<%d> wait for %d requests", rank, count);
    Request::waitall(count, requests, MPI_STATUS_IGNORE);
    for(i = 0; i < count; i++) {
      if(requests[i]!=MPI_REQUEST_NULL)
        Request::unref(&requests[i]);
    }
    delete[] requests;
  }
  return err;
}

}
}
