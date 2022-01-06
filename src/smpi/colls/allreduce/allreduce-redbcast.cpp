/* Copyright (c) 2013-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"
namespace simgrid{
namespace smpi{
int allreduce__redbcast(const void *buf, void *buf2, int count,
                        MPI_Datatype datatype, MPI_Op op,
                        MPI_Comm comm)
{
  colls::reduce(buf, buf2, count, datatype, op, 0, comm);
  colls::bcast(buf2, count, datatype, 0, comm);
  return MPI_SUCCESS;
}
}
}
