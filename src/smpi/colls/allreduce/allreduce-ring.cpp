/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This algorithm is adapted from MPICH's allreduce_intra_ring.c:
 *   Copyright (C) by Argonne National Laboratory.
 *   See COPYRIGHT in the MPICH top-level directory. */

#include "../colls_private.hpp"

#include <vector>

namespace simgrid::smpi {

int allreduce__ring(const void* sendbuf, void* recvbuf, int count, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm)
{
  const int rank = comm->rank();
  const int nranks = comm->size();
  const MPI_Aint extent = dtype->get_extent();
  const int tag = COLL_TAG_ALLREDUCE;

  std::vector<int> cnts(nranks, 0);
  std::vector<int> displs(nranks, 0);

  int total_count = 0;
  for (int i = 0; i < nranks; i++) {
    cnts[i] = (count + nranks - 1) / nranks;
    if (total_count + cnts[i] > count) {
      cnts[i] = count - total_count;
      break;
    }
    total_count += cnts[i];
  }
  for (int i = 1; i < nranks; i++)
    displs[i] = displs[i - 1] + cnts[i - 1];

  if (sendbuf != MPI_IN_PLACE)
    Datatype::copy(sendbuf, count, dtype, recvbuf, count, dtype);

  std::vector<unsigned char> tmpbuf(static_cast<size_t>(count) * extent);

  const int src = (nranks + rank - 1) % nranks;
  const int dst = (rank + 1) % nranks;

  for (int i = 0; i < nranks - 1; i++) {
    const int recv_rank = (nranks + rank - 2 - i) % nranks;
    const int send_rank = (nranks + rank - 1 - i) % nranks;

    MPI_Request reqs[2];
    reqs[0] = Request::irecv(tmpbuf.data(), cnts[recv_rank], dtype, src, tag, comm);
    reqs[1] = Request::isend(static_cast<char*>(recvbuf) + displs[send_rank] * extent, cnts[send_rank], dtype, dst,
                             tag, comm);
    Request::waitall(2, reqs, MPI_STATUSES_IGNORE);

    if (op != MPI_OP_NULL)
      op->apply(tmpbuf.data(), static_cast<char*>(recvbuf) + displs[recv_rank] * extent, &cnts[recv_rank], dtype);
  }

  return allgatherv__mpich_ring(MPI_IN_PLACE, -1, MPI_DATATYPE_NULL, recvbuf, cnts.data(), displs.data(), dtype,
                                comm);
}

} // namespace simgrid::smpi
