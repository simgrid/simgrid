/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This algorithm is adapted from MPICH's alltoall_intra_k_brucks.c:
 *   Copyright (C) by Argonne National Laboratory.
 *   See COPYRIGHT in the MPICH top-level directory. */

#include "../colls_private.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace simgrid::smpi {

namespace {

/* Pack (pack=true) or unpack the blocks whose phase-th base-k digit is digitval. */
int brucks_sched_pup(bool pack, unsigned char* rbuf, unsigned char* pupbuf, MPI_Datatype rtype, int count,
                     int pow_k_phase, int k, int digitval, int comm_size, int* pupsize, MPI_Aint extent)
{
  int offset = pow_k_phase * digitval;
  int nconsecutive_occurrences = pow_k_phase;
  const int delta = (k - 1) * pow_k_phase;

  *pupsize = 0;
  while (offset < comm_size) {
    if (pack)
      Datatype::copy(rbuf + static_cast<size_t>(offset) * count * extent, count, rtype, pupbuf + *pupsize, count,
                     rtype);
    else
      Datatype::copy(pupbuf + *pupsize, count, rtype, rbuf + static_cast<size_t>(offset) * count * extent, count,
                     rtype);

    offset++;
    nconsecutive_occurrences--;
    if (nconsecutive_occurrences == 0) {
      offset += delta;
      nconsecutive_occurrences = pow_k_phase;
    }
    *pupsize += count * extent;
  }
  return MPI_SUCCESS;
}

} // namespace

int alltoall__k_brucks(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                       MPI_Datatype recvtype, MPI_Comm comm, int k)
{
  const int rank = comm->rank();
  const int size = comm->size();
  const int tag = COLL_TAG_ALLTOALL;
  const bool is_inplace = (sendbuf == MPI_IN_PLACE);

  if (k < 2)
    k = 2;

  int nphases = 0;
  for (int max = size - 1; max; max /= k)
    nphases++;

  int p_of_k = 1;
  for (int i = 0; i < nphases - 1; i++)
    p_of_k *= k;

  MPI_Aint s_extent = sendtype->get_extent();
  MPI_Aint r_extent = recvtype->get_extent();

  if (is_inplace) {
    sendcount = recvcount;
    sendtype = recvtype;
    s_extent = r_extent;
  }

  std::vector<unsigned char> tmp_buf(static_cast<size_t>(recvcount) * size * r_extent);
  const unsigned char* senddata = static_cast<const unsigned char*>(sendbuf);
  if (is_inplace) {
    Datatype::copy(recvbuf, size * recvcount, recvtype, tmp_buf.data(), size * recvcount, recvtype);
    senddata = tmp_buf.data();
  }

  Datatype::copy(senddata + static_cast<size_t>(rank) * sendcount * s_extent, (size - rank) * sendcount, sendtype,
                 recvbuf, (size - rank) * recvcount, recvtype);
  Datatype::copy(senddata, rank * sendcount, sendtype,
                 static_cast<unsigned char*>(recvbuf) + static_cast<size_t>(size - rank) * recvcount * r_extent,
                 rank * recvcount, recvtype);

  const size_t buf_size = static_cast<size_t>(p_of_k) * r_extent * recvcount;
  std::vector<std::vector<unsigned char>> tmp_sbuf(std::max(k - 1, 1), std::vector<unsigned char>(buf_size));
  std::vector<std::vector<unsigned char>> tmp_rbuf(std::max(k - 1, 1), std::vector<unsigned char>(buf_size));

  int delta = 1;
  for (int i = 0; i < nphases; i++) {
    std::vector<MPI_Request> reqs;
    std::vector<int> packsizes(k - 1, 0);

    for (int j = 1; j < k; j++) {
      if (delta * j >= size)
        break;
      const int src = (rank - delta * j + size) % size;
      const int dst = (rank + delta * j) % size;

      brucks_sched_pup(true, static_cast<unsigned char*>(recvbuf), tmp_sbuf[j - 1].data(), recvtype, recvcount, delta,
                       k, j, size, &packsizes[j - 1], r_extent);

      reqs.push_back(Request::irecv(tmp_rbuf[j - 1].data(), packsizes[j - 1], MPI_BYTE, src, tag, comm));
      reqs.push_back(Request::isend(tmp_sbuf[j - 1].data(), packsizes[j - 1], MPI_BYTE, dst, tag, comm));
    }
    Request::waitall(static_cast<int>(reqs.size()), reqs.data(), MPI_STATUSES_IGNORE);

    for (int j = 1; j < k; j++) {
      if (delta * j >= size)
        break;
      brucks_sched_pup(false, static_cast<unsigned char*>(recvbuf), tmp_rbuf[j - 1].data(), recvtype, recvcount,
                       delta, k, j, size, &packsizes[j - 1], r_extent);
    }
    delta *= k;
  }

  std::vector<unsigned char> rot_buf(static_cast<size_t>(recvcount) * size * r_extent);
  Datatype::copy(static_cast<unsigned char*>(recvbuf) + static_cast<size_t>(rank + 1) * recvcount * r_extent,
                 (size - rank - 1) * recvcount, recvtype, rot_buf.data(), (size - rank - 1) * recvcount, recvtype);
  Datatype::copy(recvbuf, (rank + 1) * recvcount, recvtype,
                 rot_buf.data() + static_cast<size_t>(size - rank - 1) * recvcount * r_extent,
                 (rank + 1) * recvcount, recvtype);

  for (int i = 0; i < size; i++)
    Datatype::copy(rot_buf.data() + static_cast<size_t>(i) * recvcount * r_extent, recvcount, recvtype,
                   static_cast<unsigned char*>(recvbuf) + static_cast<size_t>(size - i - 1) * recvcount * r_extent,
                   recvcount, recvtype);

  return MPI_SUCCESS;
}

int alltoall__k_brucks2(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                        MPI_Datatype recvtype, MPI_Comm comm)
{
  return alltoall__k_brucks(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, 2);
}

} // namespace simgrid::smpi
