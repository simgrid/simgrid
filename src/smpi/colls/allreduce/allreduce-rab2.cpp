/* Copyright (c) 2013-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"
//#include <star-reduction.c>

namespace simgrid{
namespace smpi{
// this requires that count >= NP
int allreduce__rab2(const void *sbuff, void *rbuff,
                    int count, MPI_Datatype dtype,
                    MPI_Op op, MPI_Comm comm)
{
  MPI_Aint s_extent;
  int i, rank, nprocs;
  int nbytes, send_size, s_offset, r_offset;
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
  rank = comm->rank();
  nprocs = comm->size();


  s_extent = dtype->get_extent();

  // uneven count
  if (count % nprocs) {
    if (count < nprocs)
      send_size = nprocs;
    else
      send_size = (count + nprocs) / nprocs;
    nbytes = send_size * s_extent;

    unsigned char* send = smpi_get_tmp_sendbuffer(s_extent * send_size * nprocs);
    unsigned char* recv = smpi_get_tmp_recvbuffer(s_extent * send_size * nprocs);
    unsigned char* tmp  = smpi_get_tmp_sendbuffer(nbytes);

    memcpy(send, sbuff, s_extent * count);

    colls::alltoall(send, send_size, dtype, recv, send_size, dtype, comm);

    memcpy(tmp, recv, nbytes);

    for (i = 1, s_offset = nbytes; i < nprocs; i++, s_offset = i * nbytes)
      if (op != MPI_OP_NULL)
        op->apply(recv + s_offset, tmp, &send_size, dtype);

    colls::allgather(tmp, send_size, dtype, recv, send_size, dtype, comm);
    memcpy(rbuff, recv, count * s_extent);

    smpi_free_tmp_buffer(recv);
    smpi_free_tmp_buffer(tmp);
    smpi_free_tmp_buffer(send);
  } else {
    const void* send = sbuff;
    send_size = count / nprocs;
    nbytes = send_size * s_extent;
    r_offset = rank * nbytes;

    unsigned char* recv = smpi_get_tmp_recvbuffer(s_extent * send_size * nprocs);

    colls::alltoall(send, send_size, dtype, recv, send_size, dtype, comm);

    memcpy((char *) rbuff + r_offset, recv, nbytes);

    for (i = 1, s_offset = nbytes; i < nprocs; i++, s_offset = i * nbytes)
      if (op != MPI_OP_NULL)
        op->apply(recv + s_offset, static_cast<char*>(rbuff) + r_offset, &send_size, dtype);

    colls::allgather((char*)rbuff + r_offset, send_size, dtype, rbuff, send_size, dtype, comm);
    smpi_free_tmp_buffer(recv);
  }

  return MPI_SUCCESS;
}
}
}
