/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Knomial-tree broadcast, in the spirit of MVAPICH's bcast_knomial_osu.c and of
 * the knomial phase of MPICH's bcast_intra_tree.c: a node sends to the ranks at
 * distance mask*1..mask*(k-1) with mask stepping down by a factor k, so a rank
 * receives from the nearest lower multiple of k*mask. With a positive segment
 * size the message is split in segments of that many bytes, each segment being
 * broadcast on its own (MVAPICH's pipelined knomial).
 *
 *   Copyright (c) 2001-2025, The Ohio State University. All rights reserved.
 *   See the COPYRIGHT file in the MVAPICH top-level directory. */

#include "../colls_private.hpp"

#include <algorithm>
#include <vector>

namespace simgrid::smpi {

int bcast__knomial(void* buff, int count, MPI_Datatype datatype, int root, MPI_Comm comm, int k, int seg_bytes)
{
  const int rank      = comm->rank();
  const int size      = comm->size();
  const int tag       = COLL_TAG_BCAST;
  const size_t dsize  = datatype->size();
  const size_t nbytes = static_cast<size_t>(count) * dsize;
  if (nbytes == 0 || size == 1 || k < 2)
    return MPI_SUCCESS;

  std::vector<size_t> bounds{0};
  if (seg_bytes > 0 && static_cast<size_t>(seg_bytes) < nbytes) {
    const size_t segment = std::max<size_t>(1, static_cast<size_t>(seg_bytes) / dsize);
    for (size_t elem = segment; elem < static_cast<size_t>(count); elem += segment)
      bounds.push_back(elem);
    bounds.push_back(count);
  } else {
    bounds.push_back(count);
  }

  const int relative = (rank + size - root) % size;
  auto* base         = static_cast<char*>(buff);

  for (size_t c = 0; c + 1 < bounds.size(); c++) {
    const int length = static_cast<int>(bounds[c + 1] - bounds[c]);
    auto* slot       = base + bounds[c] * dsize;

    int mask = 1;
    int src  = -1;
    while (mask < size) {
      if (relative % (k * mask) != 0) {
        src = relative / (k * mask) * (k * mask) + root;
        if (src >= size)
          src -= size;
        break;
      }
      mask *= k;
    }
    if (src >= 0)
      Request::recv(slot, length, datatype, src, tag, comm, MPI_STATUS_IGNORE);

    for (mask /= k; mask > 0; mask /= k) {
      std::vector<MPI_Request> requests;
      for (int step = 1; step < k; step++) {
        if (relative + mask * step < size) {
          int dst = rank + mask * step;
          if (dst >= size)
            dst -= size;
          requests.push_back(Request::isend(slot, length, datatype, dst, tag, comm));
        }
      }
      if (not requests.empty())
        Request::waitall(requests.size(), requests.data(), MPI_STATUSES_IGNORE);
    }
  }

  return MPI_SUCCESS;
}

int bcast__knomial2(void* buff, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
  return bcast__knomial(buff, count, datatype, root, comm, 4, 0);
}

int bcast__pipelined_knomial2(void* buff, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
  return bcast__knomial(buff, count, datatype, root, comm, 4, 8192);
}

} // namespace simgrid::smpi
