/* Copyright (c) 2013-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"
//#include <star-reduction.c>
namespace simgrid {
namespace smpi {
int reduce__flat_tree(const void *sbuf, void *rbuf, int count,
                     MPI_Datatype dtype, MPI_Op op,
                     int root, MPI_Comm comm)
{
  int i, tag = COLL_TAG_REDUCE;
  int size;
  int rank;
  MPI_Aint extent;
  unsigned char* origin = nullptr;
  const unsigned char* inbuf;
  MPI_Status status;

  rank = comm->rank();
  size = comm->size();

  /* If not root, send data to the root. */
  extent = dtype->get_extent();

  if (rank != root) {
    Request::send(sbuf, count, dtype, root, tag, comm);
    return 0;
  }

  /* Root receives and reduces messages.  Allocate buffer to receive
     messages. */

  if (size > 1)
    origin = smpi_get_tmp_recvbuffer(count * extent);

  /* Initialize the receive buffer. */
  if (rank == (size - 1))
    Request::sendrecv(sbuf, count, dtype, rank, tag,
                 rbuf, count, dtype, rank, tag, comm, &status);
  else
    Request::recv(rbuf, count, dtype, size - 1, tag, comm, &status);

  /* Loop receiving and calling reduction function (C or Fortran). */

  for (i = size - 2; i >= 0; --i) {
    if (rank == i)
      inbuf = static_cast<const unsigned char*>(sbuf);
    else {
      Request::recv(origin, count, dtype, i, tag, comm, &status);
      inbuf = origin;
    }

    /* Call reduction function. */
    if(op!=MPI_OP_NULL) op->apply( inbuf, rbuf, &count, dtype);

  }

  smpi_free_tmp_buffer(origin);

  /* All done */
  return 0;
}
}
}
