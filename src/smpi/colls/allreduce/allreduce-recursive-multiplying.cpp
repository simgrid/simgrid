/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This algorithm is adapted from MPICH's allreduce_intra_recursive_multiplying.c:
 *   Copyright (C) by Argonne National Laboratory.
 *   See COPYRIGHT in the MPICH top-level directory. */

#include "../colls_private.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace simgrid::smpi {

namespace {

void local_reduce(const void* in, void* inout, int count, MPI_Datatype dtype, MPI_Op op)
{
  if (op != MPI_OP_NULL)
    op->apply(in, inout, &count, dtype);
}

} // namespace

int allreduce__recursive_multiplying(const void* sendbuf, void* recvbuf, int count, MPI_Datatype dtype, MPI_Op op,
                                     MPI_Comm comm, int k)
{
  const int rank = comm->rank();
  const int comm_size = comm->size();
  const MPI_Aint extent = dtype->get_extent();
  const int single_size = static_cast<int>(count * extent);
  const int tag = COLL_TAG_ALLREDUCE;

  if (k < 2)
    k = 2;

  int pofk = 1;
  while (pofk * k <= comm_size)
    pofk *= k;

  std::vector<unsigned char> tmp_buf(static_cast<size_t>(std::max(k - 1, 1)) * single_size);
  int virt_rank = rank;

  if (sendbuf != MPI_IN_PLACE)
    Datatype::copy(sendbuf, count, dtype, recvbuf, count, dtype);

  if (pofk < comm_size) {
    if (rank >= pofk) {
      const int pre_dst = rank % pofk;
      Request::send(recvbuf, count, dtype, pre_dst, tag, comm);
      virt_rank = -1;
    } else {
      std::vector<MPI_Request> reqs;
      int nreqs = 0;
      for (int pre_src = (rank % pofk) + pofk; pre_src < comm_size; pre_src += pofk)
        reqs.push_back(Request::irecv(tmp_buf.data() + static_cast<size_t>(nreqs++) * single_size, count, dtype,
                                      pre_src, tag, comm));
      Request::waitall(nreqs, reqs.data(), MPI_STATUSES_IGNORE);

      for (int i = 0; i < nreqs; i++) {
        if (i == nreqs - 1)
          local_reduce(tmp_buf.data() + static_cast<size_t>(i) * single_size, recvbuf, count, dtype, op);
        else
          local_reduce(tmp_buf.data() + static_cast<size_t>(i) * single_size,
                       tmp_buf.data() + static_cast<size_t>(i + 1) * single_size, count, dtype, op);
      }
    }
  }

  if (virt_rank != -1) {
    int distance = 1;
    int next_distance = k;
    while (distance < pofk) {
      const int starting_rank = rank / next_distance * next_distance;
      const int rank_offset = starting_rank + rank % distance;
      std::vector<MPI_Request> reqs;
      int exchanges = 0;

      for (int dst = rank_offset; dst < starting_rank + next_distance; dst += distance) {
        if (dst != rank) {
          reqs.push_back(Request::isend(recvbuf, count, dtype, dst, tag, comm));
          reqs.push_back(Request::irecv(tmp_buf.data() + static_cast<size_t>(exchanges) * single_size, count, dtype,
                                        dst, tag, comm));
          exchanges++;
        }
      }
      Request::waitall(static_cast<int>(reqs.size()), reqs.data(), MPI_STATUSES_IGNORE);

      exchanges = 0;
      bool recvbuf_last = false;
      for (int dst = rank_offset; dst < starting_rank + next_distance - distance; dst += distance) {
        unsigned char* dst_buf = tmp_buf.data() + static_cast<size_t>(exchanges) * single_size;
        if (dst == rank - distance) {
          local_reduce(dst_buf, recvbuf, count, dtype, op);
          recvbuf_last = true;
          exchanges++;
        } else if (dst == rank) {
          local_reduce(recvbuf, dst_buf, count, dtype, op);
          recvbuf_last = false;
        } else {
          local_reduce(dst_buf, dst_buf + single_size, count, dtype, op);
          recvbuf_last = false;
          exchanges++;
        }
      }

      if (not recvbuf_last)
        Datatype::copy(tmp_buf.data() + static_cast<size_t>(exchanges) * single_size, count, dtype, recvbuf, count,
                       dtype);

      distance = next_distance;
      next_distance *= k;
    }
  }

  if (pofk < comm_size) {
    if (rank >= pofk) {
      const int post_src = rank % pofk;
      Request::recv(recvbuf, count, dtype, post_src, tag, comm, MPI_STATUS_IGNORE);
    } else {
      std::vector<MPI_Request> reqs;
      for (int post_dst = (rank % pofk) + pofk; post_dst < comm_size; post_dst += pofk)
        reqs.push_back(Request::isend(recvbuf, count, dtype, post_dst, tag, comm));
      Request::waitall(static_cast<int>(reqs.size()), reqs.data(), MPI_STATUSES_IGNORE);
    }
  }

  return MPI_SUCCESS;
}

int allreduce__recursive_multiplying2(const void* sendbuf, void* recvbuf, int count, MPI_Datatype dtype, MPI_Op op,
                                      MPI_Comm comm)
{
  return allreduce__recursive_multiplying(sendbuf, recvbuf, count, dtype, op, comm, 2);
}

} // namespace simgrid::smpi
