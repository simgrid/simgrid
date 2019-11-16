/* Copyright (c) 2013-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"

/**
 * Alltoall Bruck
 *
 * Openmpi calls this routine when the message size sent to each rank < 2000 bytes and size < 12
 * FIXME: uh, check smpi_pmpi again, but this routine is called for > 12, not
 * less...
 **/
namespace simgrid{
namespace smpi{
int alltoallv__bruck(const void *sendbuf, const int *sendcounts, const int *senddisps,
                     MPI_Datatype sendtype, void *recvbuf,
                     const int *recvcounts,const int *recvdisps, MPI_Datatype recvtype,
                     MPI_Comm comm)
{
  int system_tag = COLL_TAG_ALLTOALLV;
  int i, rank, size, err, count;
  MPI_Aint lb;
  MPI_Aint sendext = 0;
  MPI_Aint recvext = 0;

  // FIXME: check implementation
  rank = comm->rank();
  size = comm->size();
  XBT_DEBUG("<%d> algorithm alltoall_bruck() called.", rank);

  sendtype->extent(&lb, &sendext);
  recvtype->extent(&lb, &recvext);
  /* Local copy from self */
  err =
      Datatype::copy((char *)sendbuf + senddisps[rank] * sendext,
                         sendcounts[rank], sendtype,
                         (char *)recvbuf + recvdisps[rank] * recvext,
                         recvcounts[rank], recvtype);
  if (err == MPI_SUCCESS && size > 1) {
    /* Initiate all send/recv to/from others. */

    int bblock = 4; // MPIR_PARAM_ALLTOALL_THROTTLE
    // if (bblock == 0) bblock = comm_size;

    // MPI_Request* requests = new MPI_Request[2 * (bblock - 1)];
    int ii, ss, dst;
    /* post only bblock isends/irecvs at a time as suggested by Tony Ladd */
    for (ii = 0; ii < size; ii += bblock) {
      MPI_Request* requests = new MPI_Request[2 * bblock];

      ss    = size - ii < bblock ? size - ii : bblock;
      count = 0;

      /* do the communication -- post ss sends and receives: */
      for (i = 0; i < ss; i++) {
        dst = (rank + i + ii) % size;
        if (dst == rank) {
          XBT_DEBUG("<%d> skip request creation [src = %d, recvcount = %d]", rank, i, recvcounts[dst]);
          continue;
        }

        requests[count] =
            Request::irecv((char*)recvbuf + recvdisps[dst] * recvext, recvcounts[dst], recvtype, dst, system_tag, comm);
        count++;
      }
      /* Now create all sends  */
      for (i = 0; i < ss; i++) {
        dst = (rank - i - ii + size) % size;
        if (dst == rank) {
          XBT_DEBUG("<%d> skip request creation [dst = %d, sendcount = %d]", rank, i, sendcounts[dst]);
          continue;
        }
        requests[count] =
            Request::isend((char*)sendbuf + senddisps[dst] * sendext, sendcounts[dst], sendtype, dst, system_tag, comm);
        count++;
      }
      /* Wait for them all. */
      // Colls::startall(count, requests);
      XBT_DEBUG("<%d> wait for %d requests", rank, count);
      Request::waitall(count, requests, MPI_STATUSES_IGNORE);
      delete[] requests;
    }
  }
  return MPI_SUCCESS;
}
}
}
