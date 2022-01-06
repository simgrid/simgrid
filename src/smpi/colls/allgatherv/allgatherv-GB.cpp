/* Copyright (c) 2013-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"

namespace simgrid{
namespace smpi{

// Allgather - gather/bcast algorithm
int allgatherv__GB(const void *send_buff, int send_count,
                   MPI_Datatype send_type, void *recv_buff,
                   const int *recv_counts, const int *recv_disps, MPI_Datatype recv_type,
                   MPI_Comm comm)
{
  colls::gatherv(send_buff, send_count, send_type, recv_buff, recv_counts, recv_disps, recv_type, 0, comm);
  int num_procs, i, current, max = 0;
  num_procs = comm->size();
  for (i = 0; i < num_procs; i++) {
    current = recv_disps[i] + recv_counts[i];
    if (current > max)
      max = current;
  }
  colls::bcast(recv_buff, max, recv_type, 0, comm);

  return MPI_SUCCESS;
}

}
}
