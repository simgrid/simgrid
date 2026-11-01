/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This algorithm is adapted from MPICH's reduce_intra_smp.c:
 *   Copyright (C) by Argonne National Laboratory.
 *   See COPYRIGHT in the MPICH top-level directory. */

#include "../colls_private.hpp"

#include <algorithm>
#include <memory>

namespace simgrid::smpi {

int reduce__mpich_smp(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root,
                      MPI_Comm comm)
{
  if (comm->get_leaders_comm() == MPI_COMM_NULL)
    comm->init_smp();

  const int rank = comm->rank();
  MPI_Comm intra = comm->get_intra_comm();
  MPI_Comm leaders = comm->get_leaders_comm();
  int* leaders_map = comm->get_leaders_map();

  const int root_pid = comm->group()->actor(root);
  const bool on_root_node = (leaders_map[rank] == leaders_map[root]);

  const size_t bytes = static_cast<size_t>(count) * datatype->get_extent();
  std::unique_ptr<unsigned char[]> tmp(new unsigned char[bytes ? bytes : 1]);
  const void* send = sendbuf;

  if (not on_root_node) {
    reduce__binomial(sendbuf, tmp.get(), count, datatype, op, 0, intra);
    send = tmp.get();
  }

  if (leaders->rank() != MPI_UNDEFINED) {
    const int leader_root = leaders->group()->rank(leaders_map[root]);
    if (leader_root != leaders->rank()) {
      std::unique_ptr<unsigned char[]> scratch(new unsigned char[bytes ? bytes : 1]);
      reduce__binomial(send, scratch.get(), count, datatype, op, leader_root, leaders);
    } else if (rank != root) {
      reduce__binomial(send, tmp.get(), count, datatype, op, leader_root, leaders);
      send = tmp.get();
    } else {
      reduce__binomial(send, recvbuf, count, datatype, op, leader_root, leaders);
      send = MPI_IN_PLACE;
    }
  }

  if (on_root_node) {
    const int intra_root = intra->group()->rank(root_pid);
    reduce__binomial(send, recvbuf, count, datatype, op, intra_root, intra);
  }

  return MPI_SUCCESS;
}

} // namespace simgrid::smpi
