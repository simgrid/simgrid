/* Copyright (c) 2013-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"

namespace simgrid{
namespace smpi{
// Allgather - gather/bcast algorithm
int allgather__GB(const void *send_buff, int send_count,
                  MPI_Datatype send_type, void *recv_buff,
                  int recv_count, MPI_Datatype recv_type,
                  MPI_Comm comm)
{
  int num_procs;
  num_procs = comm->size();
  colls::gather(send_buff, send_count, send_type, recv_buff, recv_count, recv_type, 0, comm);
  colls::bcast(recv_buff, (recv_count * num_procs), recv_type, 0, comm);

  return MPI_SUCCESS;
}

}
}
