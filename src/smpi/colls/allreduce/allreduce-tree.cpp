/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Binomial-tree allreduce in the spirit of MPICH's allreduce_intra_tree.c
 * (binomial variant): a binomial reduce to rank 0 followed by a binomial
 * broadcast. Only valid for commutative operations, as in MPICH's kary and
 * knomial_2 trees.
 *
 *   Copyright (C) by Argonne National Laboratory.
 *   See COPYRIGHT in the MPICH top-level directory. */

#include "../colls_private.hpp"

#include <vector>

namespace simgrid::smpi {

int allreduce__tree(const void* sendbuf, void* recvbuf, int count, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm)
{
  const int rank = comm->rank();
  const int size = comm->size();
  const MPI_Aint extent = dtype->get_extent();
  const int tag = COLL_TAG_ALLREDUCE;
  const int single_size = static_cast<int>(count * extent);

  if (sendbuf != MPI_IN_PLACE)
    Datatype::copy(sendbuf, count, dtype, recvbuf, count, dtype);

  std::vector<unsigned char> tmp(static_cast<size_t>(single_size));

  int mask = 1;
  while (mask < size) {
    if (rank & mask) {
      const int dst = rank & ~mask;
      Request::send(recvbuf, count, dtype, dst, tag, comm);
      break;
    }
    if ((rank | mask) < size) {
      Request::recv(tmp.data(), count, dtype, rank | mask, tag, comm, MPI_STATUS_IGNORE);
      if (op != MPI_OP_NULL)
        op->apply(tmp.data(), recvbuf, &count, dtype);
    }
    mask <<= 1;
  }

  mask = 1;
  while (mask < size)
    mask <<= 1;
  mask >>= 1;
  while (mask > 0) {
    if ((rank & mask) == 0 && (rank | mask) < size)
      Request::send(recvbuf, count, dtype, rank | mask, tag, comm);
    else if (rank & mask) {
      Request::recv(recvbuf, count, dtype, rank & ~mask, tag, comm, MPI_STATUS_IGNORE);
      break;
    }
    mask >>= 1;
  }

  return MPI_SUCCESS;
}

} // namespace simgrid::smpi
