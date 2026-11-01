/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This algorithm is adapted from MPICH's barrier_intra_k_dissemination.c:
 *   Copyright (C) by Argonne National Laboratory.
 *   See COPYRIGHT in the MPICH top-level directory. */

#include "../colls_private.hpp"

#include <algorithm>
#include <vector>

namespace simgrid::smpi {

int barrier__k_dissemination(MPI_Comm comm, int k)
{
  const int rank = comm->rank();
  const int nranks = comm->size();
  const int tag = COLL_TAG_BARRIER;

  if (nranks == 1)
    return MPI_SUCCESS;

  if (nranks < k)
    k = nranks;
  if (k < 2)
    k = 2;

  int nphases = 0;
  int p_of_k = 1;
  while (p_of_k < nranks) {
    p_of_k *= k;
    nphases++;
  }

  int shift = 1;
  std::vector<MPI_Request> send_reqs(std::max(k - 1, 1));
  std::vector<MPI_Request> recv_reqs(std::max(2 * (k - 1), 1));

  for (int i = 0; i < nphases; i++) {
    for (int j = 1; j < k; j++) {
      const int to = (rank + j * shift) % nranks;
      const int from = (nranks + (rank - j * shift) % nranks) % nranks;

      recv_reqs[(j - 1) + (k - 1) * (i & 1)] = Request::irecv(nullptr, 0, MPI_BYTE, from, tag, comm);
      if (i > 0 && j == 1)
        Request::waitall(k - 1, &recv_reqs[(k - 1) * ((i - 1) & 1)], MPI_STATUSES_IGNORE);
      send_reqs[j - 1] = Request::isend(nullptr, 0, MPI_BYTE, to, tag, comm);
    }
    Request::waitall(k - 1, send_reqs.data(), MPI_STATUSES_IGNORE);
    shift *= k;
  }

  Request::waitall(k - 1, &recv_reqs[(k - 1) * ((nphases - 1) & 1)], MPI_STATUSES_IGNORE);

  return MPI_SUCCESS;
}

int barrier__k_dissemination2(MPI_Comm comm)
{
  return barrier__k_dissemination(comm, 2);
}

} // namespace simgrid::smpi
