/* Copyright (c) 2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This algorithm is adapted from MPICH's allreduce_intra_recexch.c and
 * recexchalgo.c:
 *   Copyright (C) by Argonne National Laboratory.
 *   See COPYRIGHT in the MPICH top-level directory. */

#include "../colls_private.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace simgrid::smpi {

namespace {

struct RecexchNeighbors {
  int step1_sendto = -1;
  std::vector<int> step1_recvfrom;
  int step1_nrecvs = 0;
  std::vector<std::vector<int>> step2_nbrs;
  int step2_nphases = 0;
  int p_of_k = 1;
  int T = 0;
  int k = 2;
};

RecexchNeighbors recexch_get_neighbors(int rank, int nranks, int k)
{
  RecexchNeighbors res;
  if (nranks < k)
    k = (nranks > 2) ? nranks : 2;
  res.k = k;

  int p_of_k = 1;
  int log_p_of_k = 0;
  while (p_of_k <= nranks) {
    p_of_k *= k;
    log_p_of_k++;
  }
  p_of_k /= k;
  log_p_of_k--;

  res.p_of_k = p_of_k;
  res.step2_nphases = log_p_of_k;
  res.step1_recvfrom.assign(std::max(k - 1, 0), 0);
  res.step2_nbrs.resize(log_p_of_k, std::vector<int>(std::max(k - 1, 0), 0));

  const int rem = nranks - p_of_k;
  const int T = (rem * k) / (k - 1);
  res.T = T;

  int newrank = -1;
  if (rank < T) {
    if (rank % k != (k - 1)) {
      res.step1_sendto = rank + (k - 1 - rank % k);
      if (res.step1_sendto > T - 1)
        res.step1_sendto = T;
      newrank = -1;
    } else {
      for (int i = 0; i < k - 1; i++)
        res.step1_recvfrom[i] = rank - i - 1;
      res.step1_nrecvs = k - 1;
      newrank = rank / k;
    }
  } else {
    newrank = rank - rem;
    if (rank == T && (T - 1) % k != k - 1 && T >= 1) {
      const int nsenders = (T - 1) % k + 1;
      for (int j = nsenders - 1; j >= 0; j--)
        res.step1_recvfrom[nsenders - 1 - j] = T - nsenders + j;
      res.step1_nrecvs = nsenders;
    }
  }

  if (res.step1_sendto == -1) {
    std::vector<int> digit(log_p_of_k, 0);
    int temprank = newrank;
    int i_digit = 0;
    while (temprank != 0) {
      digit[i_digit++] = temprank % k;
      temprank /= k;
    }

    int phase = 0;
    int mask = 1;
    while (mask < p_of_k) {
      const int cbit = digit[phase];
      int cnt = 0;
      for (int i = 0; i < k; i++) {
        if (i != cbit) {
          digit[phase] = i;
          int nbr = 0;
          int power = 1;
          for (int j = 0; j < log_p_of_k; j++) {
            nbr += digit[j] * power;
            power *= k;
          }
          res.step2_nbrs[phase][cnt++] =
              (nbr < rem / (k - 1)) ? (nbr * k) + (k - 1) : nbr + rem;
        }
      }
      digit[phase] = cbit;
      phase++;
      mask *= k;
    }
  }
  return res;
}

int find_myidx(const std::vector<int>& nbrs, int k, int rank)
{
  for (int i = 0; i < k - 1; i++) {
    if (nbrs[i] > rank)
      return i;
  }
  return k - 1;
}

void do_reduce(std::vector<unsigned char*>& bufs, void* recvbuf, int k, int idx, int count, MPI_Datatype dtype,
               MPI_Op op)
{
  for (int i = 0; i < idx - 1; i++)
    if (op != MPI_OP_NULL)
      op->apply(bufs[i], bufs[i + 1], &count, dtype);
  if (idx > 0 && op != MPI_OP_NULL)
    op->apply(bufs[idx - 1], recvbuf, &count, dtype);
  if (idx < k - 1) {
    if (op != MPI_OP_NULL)
      op->apply(recvbuf, bufs[idx], &count, dtype);
    for (int i = idx; i < k - 2; i++)
      if (op != MPI_OP_NULL)
        op->apply(bufs[i], bufs[i + 1], &count, dtype);
    Datatype::copy(bufs[k - 2], count, dtype, recvbuf, count, dtype);
  }
}

} // namespace

int allreduce__recexch(const void* sendbuf, void* recvbuf, int count, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm,
                       int k)
{
  const int rank = comm->rank();
  const int nranks = comm->size();
  const MPI_Aint extent = dtype->get_extent();
  const int single_size = static_cast<int>(count * extent);
  const int tag = COLL_TAG_ALLREDUCE;

  if (nranks == 1) {
    if (sendbuf != MPI_IN_PLACE)
      Datatype::copy(sendbuf, count, dtype, recvbuf, count, dtype);
    return MPI_SUCCESS;
  }

  if (sendbuf != MPI_IN_PLACE && count > 0)
    Datatype::copy(sendbuf, count, dtype, recvbuf, count, dtype);

  const RecexchNeighbors neighbors = recexch_get_neighbors(rank, nranks, k);
  k = neighbors.k;
  const bool in_step2 = (neighbors.step1_sendto == -1);

  std::vector<std::vector<unsigned char>> buffers(std::max(k - 1, 1),
                                                  std::vector<unsigned char>(std::max(single_size, 1)));

  if (not in_step2) {
    Request::send(recvbuf, count, dtype, neighbors.step1_sendto, tag, comm);
  } else if (neighbors.step1_nrecvs > 0) {
    std::vector<MPI_Request> reqs(neighbors.step1_nrecvs);
    for (int i = 0; i < neighbors.step1_nrecvs; i++)
      reqs[i] = Request::irecv(buffers[i].data(), count, dtype, neighbors.step1_recvfrom[i], tag, comm);
    Request::waitall(neighbors.step1_nrecvs, reqs.data(), MPI_STATUSES_IGNORE);

    for (int i = 0; i < neighbors.step1_nrecvs && count > 0; i++)
      if (op != MPI_OP_NULL)
        op->apply(buffers[i].data(), recvbuf, &count, dtype);
  }

  for (int phase = 0; phase < neighbors.step2_nphases && in_step2; phase++) {
    std::vector<MPI_Request> recv_reqs;
    for (int i = 0; i < k - 1; i++)
      recv_reqs.push_back(Request::irecv(buffers[i].data(), count, dtype, neighbors.step2_nbrs[phase][i], tag, comm));

    std::vector<MPI_Request> send_reqs;
    for (int i = 0; i < k - 1; i++)
      send_reqs.push_back(Request::isend(recvbuf, count, dtype, neighbors.step2_nbrs[phase][i], tag, comm));

    Request::waitall(static_cast<int>(send_reqs.size()), send_reqs.data(), MPI_STATUSES_IGNORE);
    Request::waitall(static_cast<int>(recv_reqs.size()), recv_reqs.data(), MPI_STATUSES_IGNORE);

    const int myidx = op->is_commutative() ? k - 1 : find_myidx(neighbors.step2_nbrs[phase], k, rank);

    std::vector<unsigned char*> ptrs(k - 1);
    for (int i = 0; i < k - 1; i++)
      ptrs[i] = buffers[i].data();
    do_reduce(ptrs, recvbuf, k, myidx, count, dtype, op);
  }

  if (neighbors.step1_sendto != -1) {
    Request::recv(recvbuf, count, dtype, neighbors.step1_sendto, tag, comm, MPI_STATUS_IGNORE);
  } else if (neighbors.step1_nrecvs > 0) {
    std::vector<MPI_Request> reqs(neighbors.step1_nrecvs);
    for (int i = 0; i < neighbors.step1_nrecvs; i++)
      reqs[i] = Request::isend(recvbuf, count, dtype, neighbors.step1_recvfrom[i], tag, comm);
    Request::waitall(neighbors.step1_nrecvs, reqs.data(), MPI_STATUSES_IGNORE);
  }

  return MPI_SUCCESS;
}

int allreduce__recexch2(const void* sendbuf, void* recvbuf, int count, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm)
{
  return allreduce__recexch(sendbuf, recvbuf, count, dtype, op, comm, 2);
}

} // namespace simgrid::smpi
