/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Kary-tree broadcast in the spirit of MPICH's bcast_intra_tree.c and
 * bcast_intra_pipelined_tree.c: rank `root' is the root of a tree in which a
 * node numbered `lrank' (relative to the root) has children
 * lrank*k+1 .. lrank*k+k. With a zero chunk size the whole message goes down
 * the tree in one step; with a positive one it is split in chunks of that many
 * bytes, each chunk being forwarded as soon as it has been received.
 *
 *   Copyright (C) by Argonne National Laboratory.
 *   See COPYRIGHT in the MPICH top-level directory. */

#include "../colls_private.hpp"

#include <algorithm>

namespace simgrid::smpi {

int bcast__tree(void* buff, int count, MPI_Datatype datatype, int root, MPI_Comm comm, int k, int chunk_bytes)
{
  const int rank = comm->rank();
  const int size = comm->size();
  const int tag  = COLL_TAG_BCAST;

  const size_t dsize = datatype->size();
  const size_t nbytes = static_cast<size_t>(count) * dsize;
  if (nbytes == 0 || size == 1 || k < 1)
    return MPI_SUCCESS;

  const int lrank    = (rank + size - root) % size;
  const int parent   = lrank == 0 ? -1 : (lrank - 1) / k + root;
  const int parent_i = parent % size;

  /* Chunk boundaries, in elements, so that every chunk is a whole number of datatypes. */
  std::vector<size_t> bounds{0};
  if (chunk_bytes > 0 && static_cast<size_t>(chunk_bytes) < nbytes) {
    const size_t chunk = std::max<size_t>(1, static_cast<size_t>(chunk_bytes) / dsize);
    for (size_t elem = chunk; elem < static_cast<size_t>(count); elem += chunk)
      bounds.push_back(elem);
    bounds.push_back(count);
  } else {
    bounds.push_back(count);
  }

  auto* base = static_cast<char*>(buff);
  for (size_t c = 0; c + 1 < bounds.size(); c++) {
    const int length = static_cast<int>(bounds[c + 1] - bounds[c]);
    auto* slot       = base + bounds[c] * dsize;

    if (lrank != 0)
      Request::recv(slot, length, datatype, parent_i, tag, comm, MPI_STATUS_IGNORE);

    for (int j = 1; j <= k; j++) {
      const int child = lrank * k + j;
      if (child >= size)
        break;
      Request::send(slot, length, datatype, (child + root) % size, tag, comm);
    }
  }

  return MPI_SUCCESS;
}

int bcast__tree2(void* buff, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
  return bcast__tree(buff, count, datatype, root, comm, 2, 0);
}

int bcast__pipelined_tree2(void* buff, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
  return bcast__tree(buff, count, datatype, root, comm, 2, 8192);
}

} // namespace simgrid::smpi
