/* Copyright (c) 2013-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"
//#include <star-reduction.c>
namespace simgrid{
namespace smpi{
int allreduce__rdb(const void *sbuff, void *rbuff, int count,
                   MPI_Datatype dtype, MPI_Op op, MPI_Comm comm)
{
  int nprocs, rank, tag = COLL_TAG_ALLREDUCE;
  int mask, dst, pof2, newrank, rem, newdst;
  MPI_Aint extent, lb;
  MPI_Status status;
  /*
     #ifdef MPICH2_REDUCTION
     MPI_User_function * uop = MPIR_Op_table[op % 16 - 1];
     #else
     MPI_User_function *uop;
     MPIR_OP *op_ptr;
     op_ptr = MPIR_ToPointer(op);
     uop  = op_ptr->op;
     #endif
   */
  nprocs=comm->size();
  rank=comm->rank();

  dtype->extent(&lb, &extent);
  unsigned char* tmp_buf = smpi_get_tmp_sendbuffer(count * extent);

  Request::sendrecv(sbuff, count, dtype, rank, 500,
               rbuff, count, dtype, rank, 500, comm, &status);

  // find nearest power-of-two less than or equal to comm_size
  pof2 = 1;
  while (pof2 <= nprocs)
    pof2 <<= 1;
  pof2 >>= 1;

  rem = nprocs - pof2;

  // In the non-power-of-two case, all even-numbered
  // processes of rank < 2*rem send their data to
  // (rank+1). These even-numbered processes no longer
  // participate in the algorithm until the very end. The
  // remaining processes form a nice power-of-two.

  if (rank < 2 * rem) {
    // even
    if (rank % 2 == 0) {

      Request::send(rbuff, count, dtype, rank + 1, tag, comm);

      // temporarily set the rank to -1 so that this
      // process does not participate in recursive
      // doubling
      newrank = -1;
    } else                      // odd
    {
      Request::recv(tmp_buf, count, dtype, rank - 1, tag, comm, &status);
      // do the reduction on received data. since the
      // ordering is right, it doesn't matter whether
      // the operation is commutative or not.
      if(op!=MPI_OP_NULL) op->apply( tmp_buf, rbuff, &count, dtype);

      // change the rank
      newrank = rank / 2;
    }
  }

  else                          // rank >= 2 * rem
    newrank = rank - rem;

  // If op is user-defined or count is less than pof2, use
  // recursive doubling algorithm. Otherwise do a reduce-scatter
  // followed by allgather. (If op is user-defined,
  // derived datatypes are allowed and the user could pass basic
  // datatypes on one process and derived on another as long as
  // the type maps are the same. Breaking up derived
  // datatypes to do the reduce-scatter is tricky, therefore
  // using recursive doubling in that case.)

  if (newrank != -1) {
    mask = 0x1;
    while (mask < pof2) {
      newdst = newrank ^ mask;
      // find real rank of dest
      dst = (newdst < rem) ? newdst * 2 + 1 : newdst + rem;

      // Send the most current data, which is in recvbuf. Recv
      // into tmp_buf
      Request::sendrecv(rbuff, count, dtype, dst, tag, tmp_buf, count, dtype,
                   dst, tag, comm, &status);

      // tmp_buf contains data received in this step.
      // recvbuf contains data accumulated so far

      // op is commutative OR the order is already right
      // we assume it is commutative op
      //      if (op -> op_commute  || (dst < rank))
      if ((dst < rank)) {
        if(op!=MPI_OP_NULL) op->apply( tmp_buf, rbuff, &count, dtype);
      } else                    // op is noncommutative and the order is not right
      {
        if(op!=MPI_OP_NULL) op->apply( rbuff, tmp_buf, &count, dtype);

        // copy result back into recvbuf
        Request::sendrecv(tmp_buf, count, dtype, rank, tag, rbuff, count,
                     dtype, rank, tag, comm, &status);
      }
      mask <<= 1;
    }
  }
  // In the non-power-of-two case, all odd-numbered processes of
  // rank < 2 * rem send the result to (rank-1), the ranks who didn't
  // participate above.

  if (rank < 2 * rem) {
    if (rank % 2)               // odd
      Request::send(rbuff, count, dtype, rank - 1, tag, comm);
    else                        // even
      Request::recv(rbuff, count, dtype, rank + 1, tag, comm, &status);
  }

  smpi_free_tmp_buffer(tmp_buf);
  return MPI_SUCCESS;
}
}
}
