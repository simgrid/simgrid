/* Copyright (c) 2013-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"
//#include <star-reduction.c>
namespace simgrid{
namespace smpi{
// NP pow of 2 for now
int allreduce__rab1(const void *sbuff, void *rbuff,
                    int count, MPI_Datatype dtype,
                    MPI_Op op, MPI_Comm comm)
{
  MPI_Status status;
  MPI_Aint extent;
  int tag = COLL_TAG_ALLREDUCE, send_size, newcnt, share;
  unsigned int pof2 = 1, mask;
  int send_idx, recv_idx, dst, send_cnt, recv_cnt;

  int rank = comm->rank();
  unsigned int nprocs = comm->size();

  if((nprocs&(nprocs-1)))
    throw std::invalid_argument("allreduce rab1 algorithm can't be used with non power of two number of processes!");

  extent = dtype->get_extent();

  pof2 = 1;
  while (pof2 <= nprocs)
    pof2 <<= 1;
  pof2 >>= 1;

  send_idx = recv_idx = 0;

  // uneven count
  if ((count % nprocs)) {
    send_size = (count + nprocs) / nprocs;
    newcnt = send_size * nprocs;

    unsigned char* recv    = smpi_get_tmp_recvbuffer(extent * newcnt);
    unsigned char* tmp_buf = smpi_get_tmp_sendbuffer(extent * newcnt);
    memcpy(recv, sbuff, extent * count);


    mask = pof2 / 2;
    share = newcnt / pof2;
    while (mask > 0) {
      dst = rank ^ mask;
      send_cnt = recv_cnt = newcnt / (pof2 / mask);

      if (rank < dst)
        send_idx = recv_idx + (mask * share);
      else
        recv_idx = send_idx + (mask * share);

      Request::sendrecv(recv + send_idx * extent, send_cnt, dtype, dst, tag, tmp_buf, recv_cnt, dtype, dst, tag, comm,
                        &status);

      if (op != MPI_OP_NULL)
        op->apply(tmp_buf, recv + recv_idx * extent, &recv_cnt, dtype);

      // update send_idx for next iteration
      send_idx = recv_idx;
      mask >>= 1;
    }

    memcpy(tmp_buf, recv + recv_idx * extent, recv_cnt * extent);
    Colls::allgather(tmp_buf, recv_cnt, dtype, recv, recv_cnt, dtype, comm);

    memcpy(rbuff, recv, count * extent);
    smpi_free_tmp_buffer(recv);
    smpi_free_tmp_buffer(tmp_buf);

  }

  else {
    unsigned char* tmp_buf = smpi_get_tmp_sendbuffer(extent * count);
    memcpy(rbuff, sbuff, count * extent);
    mask = pof2 / 2;
    share = count / pof2;
    while (mask > 0) {
      dst = rank ^ mask;
      send_cnt = recv_cnt = count / (pof2 / mask);

      if (rank < dst)
        send_idx = recv_idx + (mask * share);
      else
        recv_idx = send_idx + (mask * share);

      Request::sendrecv((char *) rbuff + send_idx * extent, send_cnt, dtype, dst,
                   tag, tmp_buf, recv_cnt, dtype, dst, tag, comm, &status);

      if(op!=MPI_OP_NULL) op->apply( tmp_buf, (char *) rbuff + recv_idx * extent, &recv_cnt,
                     dtype);

      // update send_idx for next iteration
      send_idx = recv_idx;
      mask >>= 1;
    }

    memcpy(tmp_buf, (char *) rbuff + recv_idx * extent, recv_cnt * extent);
    Colls::allgather(tmp_buf, recv_cnt, dtype, rbuff, recv_cnt, dtype, comm);
    smpi_free_tmp_buffer(tmp_buf);
  }

  return MPI_SUCCESS;
}
}
}
