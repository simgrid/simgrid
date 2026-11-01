/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This algorithm is adapted from MPICH's barrier_intra_recexch.c:
 *   Copyright (C) by Argonne National Laboratory.
 *   See COPYRIGHT in the MPICH top-level directory. */

#include "../colls_private.hpp"

namespace simgrid::smpi {

int barrier__recexch(MPI_Comm comm, int k)
{
  unsigned char dummy = 0;
  return allreduce__recexch(MPI_IN_PLACE, &dummy, 0, MPI_BYTE, MPI_SUM, comm, k);
}

int barrier__recexch2(MPI_Comm comm)
{
  return barrier__recexch(comm, 2);
}

} // namespace simgrid::smpi
