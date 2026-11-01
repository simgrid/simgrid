/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This algorithm is adapted from MPICH's alltoall_intra_scattered.c:
 *   Copyright (C) by Argonne National Laboratory.
 *   See COPYRIGHT in the MPICH top-level directory. */

#include "../colls_private.hpp"

#include <vector>

namespace simgrid::smpi {

int alltoall__scattered(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                        MPI_Datatype recvtype, MPI_Comm comm)
{
  const int rank = comm->rank();
  const int size = comm->size();
  const int tag = COLL_TAG_ALLTOALL;
  const MPI_Aint send_extent = sendtype->get_extent();
  const MPI_Aint recv_extent = recvtype->get_extent();

  std::vector<MPI_Request> reqs(2 * size);
  std::vector<MPI_Status> statuses(2 * size);

  for (int i = 0; i < size; i++) {
    const int dst = (rank + i) % size;
    reqs[i] = Request::irecv(static_cast<unsigned char*>(recvbuf) + static_cast<size_t>(dst) * recvcount * recv_extent,
                             recvcount, recvtype, dst, tag, comm);
  }
  for (int i = 0; i < size; i++) {
    const int dst = (rank - i + size) % size;
    reqs[i + size] =
        Request::isend(static_cast<const unsigned char*>(sendbuf) + static_cast<size_t>(dst) * sendcount * send_extent,
                       sendcount, sendtype, dst, tag, comm);
  }
  Request::waitall(2 * size, reqs.data(), statuses.data());

  return MPI_SUCCESS;
}

} // namespace simgrid::smpi
