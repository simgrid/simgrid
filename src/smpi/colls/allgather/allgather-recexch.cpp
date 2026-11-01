/* Copyright (c) 2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This algorithm is adapted from MPICH's allgather_intra_recexch.c and
 * recexchalgo.c:
 *   Copyright (C) by Argonne National Laboratory.
 *   See COPYRIGHT in the MPICH top-level directory. */

#include "../colls_private.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
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

    int mask = 1;
    int phase = 0;
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

int recexch_step2rank_to_origrank(int rank, int rem, int k)
{
  return (rank < rem / (k - 1)) ? (rank * k) + (k - 1) : rank + rem;
}

int recexch_origrank_to_step2rank(int rank, int T, int rem, int k)
{
  return (rank < T) ? rank / k : rank - rem;
}

void recexch_get_count_and_offset(int rank, int phase, int k, int nranks, int* count, int* offset)
{
  int p_of_k = 1;
  while (p_of_k <= nranks)
    p_of_k *= k;
  p_of_k /= k;

  const int rem = nranks - p_of_k;
  const int T = (rem * k) / (k - 1);

  int k_power_phase = 1;
  while (phase > 0) {
    k_power_phase *= k;
    phase--;
  }

  const int step2rank = recexch_origrank_to_step2rank(rank, T, rem, k);
  int min = ((step2rank / k_power_phase) * k_power_phase) - 1;
  const int max = min + k_power_phase;
  const int orig_min = (min >= 0) ? recexch_step2rank_to_origrank(min, rem, k) : min;
  const int orig_max = recexch_step2rank_to_origrank(max, rem, k);
  *count = orig_max - orig_min;
  *offset = orig_min + 1;
}

int recexch_reverse_digits_step2(int rank, int comm_size, int k)
{
  int pofk = 1;
  int log_pofk = 0;
  while (pofk <= comm_size) {
    pofk *= k;
    log_pofk++;
  }
  pofk /= k;
  log_pofk--;

  const int rem = comm_size - pofk;
  const int T = (rem * k) / (k - 1);
  int step2rank = recexch_origrank_to_step2rank(rank, T, rem, k);

  std::vector<int> digit(log_pofk, 0);
  int i_digit = 0;
  while (step2rank != 0) {
    digit[i_digit++] = step2rank % k;
    step2rank /= k;
  }

  int step2_reverse_rank = 0;
  int power = 1;
  for (int i = 0; i < log_pofk; i++) {
    step2_reverse_rank += digit[log_pofk - 1 - i] * power;
    power *= k;
  }
  return recexch_step2rank_to_origrank(step2_reverse_rank, rem, k);
}

void recexch_allgather(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                       MPI_Datatype recvtype, MPI_Comm comm, bool is_halving, int k)
{
  const int rank = comm->rank();
  const int nranks = comm->size();
  const MPI_Aint recv_extent = recvtype->get_extent();
  const int tag = COLL_TAG_ALLGATHER;

  const RecexchNeighbors neighbors = recexch_get_neighbors(rank, nranks, k);
  k = neighbors.k;

  const bool is_instep2 = (neighbors.step1_sendto == -1);
  if (is_instep2)
    Datatype::copy(sendbuf, sendcount, sendtype,
                   static_cast<char*>(recvbuf) + static_cast<size_t>(rank) * recv_extent * recvcount, recvcount,
                   recvtype);

  if (neighbors.step1_sendto != -1) {
    Request::send(sendbuf, recvcount, recvtype, neighbors.step1_sendto, tag, comm);
  } else if (neighbors.step1_nrecvs > 0) {
    std::vector<MPI_Request> reqs(neighbors.step1_nrecvs);
    for (int i = 0; i < neighbors.step1_nrecvs; i++) {
      const int src = neighbors.step1_recvfrom[i];
      reqs[i] = Request::irecv(static_cast<char*>(recvbuf) + static_cast<size_t>(src) * recv_extent * recvcount,
                               recvcount, recvtype, src, tag, comm);
    }
    Request::waitall(static_cast<int>(reqs.size()), reqs.data(), MPI_STATUSES_IGNORE);
  }

  if (is_halving && is_instep2) {
    const int partner = recexch_reverse_digits_step2(rank, nranks, k);
    if (rank != partner) {
      int count;
      int offset;
      recexch_get_count_and_offset(rank, 0, k, nranks, &count, &offset);
      const int send_count = count;
      const MPI_Aint send_offset = static_cast<size_t>(offset) * recv_extent * recvcount;
      recexch_get_count_and_offset(partner, 0, k, nranks, &count, &offset);
      const int recv_count = count;
      const MPI_Aint recv_offset = static_cast<size_t>(offset) * recv_extent * recvcount;
      MPI_Status status;
      Request::sendrecv(static_cast<char*>(recvbuf) + send_offset, send_count * recvcount, recvtype, partner, tag,
                        static_cast<char*>(recvbuf) + recv_offset, recv_count * recvcount, recvtype, partner, tag,
                        comm, &status);
    }
  }

  int phase = is_halving ? neighbors.step2_nphases - 1 : 0;
  int recv_phase = phase;

  for (int j = 0; j < neighbors.step2_nphases && is_instep2; j++) {
    const int single_phase_recv = 1;
    std::vector<MPI_Request> recv_reqs((k - 1) * 2);
    std::vector<MPI_Request> send_reqs((k - 1) * 2);
    int num_sreq = 0;
    int num_rreq = 0;

    const int total_phases = single_phase_recv ? 1 : 2;
    for (int iter = 0; iter < total_phases && (j + iter) < neighbors.step2_nphases; iter++) {
      for (int i = 0; i < k - 1; i++) {
        const int nbr = neighbors.step2_nbrs[recv_phase][i];
        const int rank_for_offset = is_halving ? recexch_reverse_digits_step2(nbr, nranks, k) : nbr;
        int count;
        int offset;
        recexch_get_count_and_offset(rank_for_offset, j + iter, k, nranks, &count, &offset);
        const MPI_Aint recv_offset = static_cast<size_t>(offset) * recv_extent * recvcount;
        recv_reqs[num_rreq++] =
            Request::irecv(static_cast<char*>(recvbuf) + recv_offset, count * recvcount, recvtype, nbr, tag, comm);
      }
      if (is_halving)
        recv_phase--;
      else
        recv_phase++;
    }

    for (int i = 0; i < k - 1; i++) {
      const int nbr = neighbors.step2_nbrs[phase][i];
      const int rank_for_offset = is_halving ? recexch_reverse_digits_step2(rank, nranks, k) : rank;
      int count;
      int offset;
      recexch_get_count_and_offset(rank_for_offset, j, k, nranks, &count, &offset);
      const MPI_Aint send_offset = static_cast<size_t>(offset) * recv_extent * recvcount;
      send_reqs[num_sreq++] =
          Request::isend(static_cast<char*>(recvbuf) + send_offset, count * recvcount, recvtype, nbr, tag, comm);
    }

    Request::waitall(k - 1, recv_reqs.data(), MPI_STATUSES_IGNORE);

    if (is_halving)
      phase--;
    else
      phase++;

    Request::waitall(num_sreq, send_reqs.data(), MPI_STATUSES_IGNORE);
  }

  if (neighbors.step1_sendto != -1) {
    Request::recv(recvbuf, recvcount * nranks, recvtype, neighbors.step1_sendto, tag, comm, MPI_STATUS_IGNORE);
  }

  if (neighbors.step1_nrecvs > 0) {
    std::vector<MPI_Request> reqs(neighbors.step1_nrecvs);
    for (int i = 0; i < neighbors.step1_nrecvs; i++)
      reqs[i] = Request::isend(recvbuf, recvcount * nranks, recvtype, neighbors.step1_recvfrom[i], tag, comm);
    Request::waitall(static_cast<int>(reqs.size()), reqs.data(), MPI_STATUSES_IGNORE);
  }
}

} // namespace

int allgather__recexch_doubling(const void* send_buff, int send_count, MPI_Datatype send_type, void* recv_buff,
                                int recv_count, MPI_Datatype recv_type, MPI_Comm comm)
{
  recexch_allgather(send_buff, send_count, send_type, recv_buff, recv_count, recv_type, comm, false, 2);
  return MPI_SUCCESS;
}

int allgather__recexch_halving(const void* send_buff, int send_count, MPI_Datatype send_type, void* recv_buff,
                               int recv_count, MPI_Datatype recv_type, MPI_Comm comm)
{
  recexch_allgather(send_buff, send_count, send_type, recv_buff, recv_count, recv_type, comm, true, 2);
  return MPI_SUCCESS;
}

} // namespace simgrid::smpi
