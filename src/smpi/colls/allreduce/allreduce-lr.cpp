/* Copyright (c) 2013-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"

/* IMPLEMENTED BY PITCH PATARASUK
   Non-topology-specific all-reduce operation designed bandwidth optimally
   Bug fixing by Xin Yuan, 04/04/2008
*/

/* ** NOTE **
   Use -DMPICH2_REDUCTION if this code does not compile.
   MPICH1 code also work on MPICH2 on our cluster and the performance are similar.
   This code assume commutative and associative reduce operator (MPI_SUM, MPI_MAX, etc).
*/

//#include <star-reduction.c>
namespace simgrid {
namespace smpi {
int allreduce__lr(const void *sbuf, void *rbuf, int rcount,
                  MPI_Datatype dtype, MPI_Op op, MPI_Comm comm)
{
  int tag = COLL_TAG_ALLREDUCE;
  MPI_Status status;
  int rank, i, size, count;
  int send_offset, recv_offset;
  int remainder, remainder_flag, remainder_offset;

  rank = comm->rank();
  size = comm->size();

  /* make it compatible with all data type */
  MPI_Aint extent;
  extent = dtype->get_extent();

  /* when communication size is smaller than number of process (not support) */
  if (rcount < size) {
    XBT_WARN("MPI_allreduce_lr use default MPI_allreduce.");
    allreduce__default(sbuf, rbuf, rcount, dtype, op, comm);
    return MPI_SUCCESS;
  }

  /* when communication size is not divisible by number of process:
     call the native implementation for the remain chunk at the end of the operation */
  if (rcount % size != 0) {
    remainder = rcount % size;
    remainder_flag = 1;
    remainder_offset = (rcount / size) * size * extent;
  } else {
    remainder = remainder_flag = remainder_offset = 0;
  }

  /* size of each point-to-point communication is equal to the size of the whole message
     divided by number of processes
   */
  count = rcount / size;

  /* our ALL-REDUCE implementation
     1. copy (partial of)send_buf to recv_buf
     2. use logical ring reduce-scatter
     3. use logical ring all-gather
   */

  // copy partial data
  send_offset = ((rank - 1 + size) % size) * count * extent;
  recv_offset = ((rank - 1 + size) % size) * count * extent;
  Request::sendrecv((char *) sbuf + send_offset, count, dtype, rank, tag - 1,
               (char *) rbuf + recv_offset, count, dtype, rank, tag - 1, comm,
               &status);

  // reduce-scatter
  for (i = 0; i < (size - 1); i++) {
    send_offset = ((rank - 1 - i + 2 * size) % size) * count * extent;
    recv_offset = ((rank - 2 - i + 2 * size) % size) * count * extent;
    //    recv_offset = ((rank-i+2*size)%size)*count*extent;
    Request::sendrecv((char *) rbuf + send_offset, count, dtype, ((rank + 1) % size),
                 tag + i, (char *) rbuf + recv_offset, count, dtype,
                 ((rank + size - 1) % size), tag + i, comm, &status);

    // compute result to rbuf+recv_offset
    if(op!=MPI_OP_NULL) op->apply( (char *) sbuf + recv_offset, (char *) rbuf + recv_offset,
                   &count, dtype);
  }

  // all-gather
  for (i = 0; i < (size - 1); i++) {
    send_offset = ((rank - i + 2 * size) % size) * count * extent;
    recv_offset = ((rank - 1 - i + 2 * size) % size) * count * extent;
    Request::sendrecv((char *) rbuf + send_offset, count, dtype, ((rank + 1) % size),
                 tag + i, (char *) rbuf + recv_offset, count, dtype,
                 ((rank + size - 1) % size), tag + i, comm, &status);
  }

  /* when communication size is not divisible by number of process:
     call the native implementation for the remain chunk at the end of the operation */
  if (remainder_flag) {
    return colls::allreduce((char*)sbuf + remainder_offset, (char*)rbuf + remainder_offset, remainder, dtype, op, comm);
  }

  return 0;
}
}
}
