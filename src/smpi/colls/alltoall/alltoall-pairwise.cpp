/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This algorithm is adapted from MPICH's alltoall_intra_pairwise.c:
 *   Copyright (C) by Argonne National Laboratory.
 *   See COPYRIGHT in the MPICH top-level directory. */

#include "../colls_private.hpp"

namespace simgrid::smpi {

int alltoall__pairwise(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                       MPI_Datatype recvtype, MPI_Comm comm)
{
  const int rank = comm->rank();
  const int size = comm->size();
  const int tag = COLL_TAG_ALLTOALL;
  const MPI_Aint send_extent = sendtype->get_extent();
  const MPI_Aint recv_extent = recvtype->get_extent();

  Datatype::copy(static_cast<const unsigned char*>(sendbuf) + static_cast<size_t>(rank) * sendcount * send_extent,
                 sendcount, sendtype,
                 static_cast<unsigned char*>(recvbuf) + static_cast<size_t>(rank) * recvcount * recv_extent,
                 recvcount, recvtype);

  bool is_pof2 = (size & (size - 1)) == 0;
  MPI_Status status;

  for (int i = 1; i < size; i++) {
    int src;
    int dst;
    if (is_pof2) {
      src = dst = rank ^ i;
    } else {
      src = (rank - i + size) % size;
      dst = (rank + i) % size;
    }

    Request::sendrecv(static_cast<const unsigned char*>(sendbuf) + static_cast<size_t>(dst) * sendcount * send_extent,
                      sendcount, sendtype, dst, tag,
                      static_cast<unsigned char*>(recvbuf) + static_cast<size_t>(src) * recvcount * recv_extent,
                      recvcount, recvtype, src, tag, comm, &status);
  }

  return MPI_SUCCESS;
}

} // namespace simgrid::smpi
