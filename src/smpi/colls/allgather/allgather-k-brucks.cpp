/* Copyright (c) 2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This algorithm is adapted from MPICH's allgather_intra_k_brucks.c:
 *   Copyright (C) by Argonne National Laboratory.
 *   See COPYRIGHT in the MPICH top-level directory. */

#include "../colls_private.hpp"

#include <algorithm>
#include <cstddef>

namespace simgrid::smpi {

int allgather__k_brucks(const void* send_buff, int send_count, MPI_Datatype send_type, void* recv_buff,
                        int recv_count, MPI_Datatype recv_type, MPI_Comm comm, int k)
{
  const int rank = comm->rank();
  const int size = comm->size();
  const MPI_Aint recv_extent = recv_type->get_extent();
  const int tag = COLL_TAG_ALLGATHER;

  int nphases = 0;
  for (int max = size - 1; max > 0; max /= k)
    nphases++;

  int p_of_k = 1;
  for (int i = 0; i < nphases; i++)
    p_of_k *= k;
  const bool is_pok = (p_of_k == size);

  unsigned char* tmp_recvbuf =
      rank == 0 ? static_cast<unsigned char*>(recv_buff)
                : smpi_get_tmp_sendbuffer(size * recv_count * recv_extent);

  Datatype::copy(send_buff, send_count, send_type, tmp_recvbuf, recv_count, recv_type);

  int delta = 1;
  MPI_Status status;
  for (int i = 0; i < nphases; i++) {
    for (int j = 1; j < k; j++) {
      if (delta * j >= size)
        break;

      const int dst = (size + (rank - delta * j)) % size;
      const int src = (rank + delta * j) % size;

      int count;
      if (i == nphases - 1 && not is_pok) {
        const int left_count = recv_count * (size - delta * j);
        count = j == k - 1 ? left_count : std::min(recv_count * delta, left_count);
      } else {
        count = recv_count * delta;
      }

      Request::sendrecv(tmp_recvbuf, count, recv_type, dst, tag,
                        tmp_recvbuf + static_cast<size_t>(j) * recv_count * delta * recv_extent, count, recv_type,
                        src, tag, comm, &status);
    }
    delta *= k;
  }

  if (rank != 0) {
    Datatype::copy(tmp_recvbuf + static_cast<size_t>(size - rank) * recv_count * recv_extent,
                   rank * recv_count, recv_type, recv_buff, rank * recv_count, recv_type);
    Datatype::copy(tmp_recvbuf, (size - rank) * recv_count, recv_type,
                   static_cast<unsigned char*>(recv_buff) + static_cast<size_t>(rank) * recv_count * recv_extent,
                   (size - rank) * recv_count, recv_type);
    smpi_free_tmp_buffer(tmp_recvbuf);
  }

  return MPI_SUCCESS;
}

int allgather__k_brucks2(const void* send_buff, int send_count, MPI_Datatype send_type, void* recv_buff,
                         int recv_count, MPI_Datatype recv_type, MPI_Comm comm)
{
  return allgather__k_brucks(send_buff, send_count, send_type, recv_buff, recv_count, recv_type, comm, 2);
}

} // namespace simgrid::smpi
