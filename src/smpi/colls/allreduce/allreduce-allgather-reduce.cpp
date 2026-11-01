/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2009 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 *
 * Additional copyrights may follow
 */

#include "../colls_private.hpp"

#include <memory>

namespace simgrid::smpi {

int allreduce__allgather_reduce(const void* sbuf, void* rbuf, int count, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm)
{
  int size = comm->size();

  MPI_Aint extent = dtype->get_extent();
  size_t allbytes = static_cast<size_t>(count) * size * extent;
  size_t bytes    = static_cast<size_t>(count) * extent;

  std::unique_ptr<char[]> partial(new char[allbytes ? allbytes : 1]);
  std::unique_ptr<char[]> sendtmp(new char[bytes ? bytes : 1]);

  const void* sendbuf = (sbuf == MPI_IN_PLACE) ? rbuf : sbuf;
  Datatype::copy(sendbuf, count, dtype, sendtmp.get(), count, dtype);
  allgather__ompi(sendtmp.get(), count, dtype, partial.get(), count, dtype, comm);

  for (int target = 1; target < size; target++) {
    if (op != MPI_OP_NULL)
      op->apply(partial.get() + static_cast<ptrdiff_t>(target) * bytes, partial.get(), &count, dtype);
  }

  Datatype::copy(partial.get(), count, dtype, rbuf, count, dtype);
  return MPI_SUCCESS;
}

} // namespace simgrid::smpi
