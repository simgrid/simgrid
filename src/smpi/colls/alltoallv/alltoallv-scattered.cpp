/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This algorithm is adapted from MPICH's alltoallv_intra_scattered.c:
 *   Copyright (C) by Argonne National Laboratory.
 *   See COPYRIGHT in the MPICH top-level directory. */

#include "../colls_private.hpp"

#include <vector>

namespace simgrid::smpi {

int alltoallv__scattered(const void* sendbuf, const int* sendcounts, const int* sdispls, MPI_Datatype sendtype,
                         void* recvbuf, const int* recvcounts, const int* rdispls, MPI_Datatype recvtype,
                         MPI_Comm comm)
{
  const int rank = comm->rank();
  const int size = comm->size();
  const int tag = COLL_TAG_ALLTOALLV;
  const MPI_Aint send_extent = sendtype->get_extent();
  const MPI_Aint recv_extent = recvtype->get_extent();
  const bool has_send_type = sendtype->size() != 0;
  const bool has_recv_type = recvtype->size() != 0;

  std::vector<MPI_Request> reqs(2 * size);
  std::vector<MPI_Status> statuses(2 * size);
  int req_cnt = 0;

  for (int i = 0; i < size; i++) {
    const int dst = (rank + i) % size;
    if (recvcounts[dst] && has_recv_type)
      reqs[req_cnt++] =
          Request::irecv(static_cast<unsigned char*>(recvbuf) + static_cast<size_t>(rdispls[dst]) * recv_extent,
                         recvcounts[dst], recvtype, dst, tag, comm);
  }
  for (int i = 0; i < size; i++) {
    const int dst = (rank - i + size) % size;
    if (sendcounts[dst] && has_send_type)
      reqs[req_cnt++] =
          Request::isend(static_cast<const unsigned char*>(sendbuf) + static_cast<size_t>(sdispls[dst]) * send_extent,
                         sendcounts[dst], sendtype, dst, tag, comm);
  }

  Request::waitall(req_cnt, reqs.data(), statuses.data());

  return MPI_SUCCESS;
}

} // namespace simgrid::smpi
