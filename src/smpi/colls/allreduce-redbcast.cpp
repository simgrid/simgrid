/* Copyright (c) 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "colls_private.h"

int smpi_coll_tuned_allreduce_redbcast(void *buf, void *buf2, int count,
                                       MPI_Datatype datatype, MPI_Op op,
                                       MPI_Comm comm)
{
  mpi_coll_reduce_fun(buf, buf2, count, datatype, op, 0, comm);
  mpi_coll_bcast_fun(buf2, count, datatype, 0, comm);
  return MPI_SUCCESS;
}
