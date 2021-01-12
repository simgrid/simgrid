/* Copyright (c) 2013-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"
#include <algorithm>

//#include <star-reduction.c>
namespace simgrid{
namespace smpi{
int reduce__binomial(const void *sendbuf, void *recvbuf, int count,
                     MPI_Datatype datatype, MPI_Op op, int root,
                     MPI_Comm comm)
{
  MPI_Status status;
  int comm_size, rank;
  int mask, relrank, source;
  int dst;
  int tag = COLL_TAG_REDUCE;
  MPI_Aint extent;
  MPI_Aint true_lb, true_extent;
  if (count == 0)
    return 0;
  rank = comm->rank();
  comm_size = comm->size();

  extent = datatype->get_extent();

  unsigned char* tmp_buf = smpi_get_tmp_sendbuffer(count * extent);
  bool is_commutative    = (op == MPI_OP_NULL || op->is_commutative());
  mask = 1;

  int lroot;
  if (is_commutative)
        lroot   = root;
  else
        lroot   = 0;
  relrank = (rank - lroot + comm_size) % comm_size;

  datatype->extent(&true_lb, &true_extent);

  /* adjust for potential negative lower bound in datatype */
  tmp_buf = tmp_buf - true_lb;

  /* If I'm not the root, then my recvbuf may not be valid, therefore
     I have to allocate a temporary one */
  if (rank != root) {
      recvbuf = (void*)smpi_get_tmp_recvbuffer(count * std::max(extent, true_extent));
      recvbuf = (void *)((char*)recvbuf - true_lb);
  }
   if ((rank != root) || (sendbuf != MPI_IN_PLACE)) {
      Datatype::copy(sendbuf, count, datatype, recvbuf,count, datatype);
  }

  while (mask < comm_size) {
    /* Receive */
    if ((mask & relrank) == 0) {
      source = (relrank | mask);
      if (source < comm_size) {
        source = (source + lroot) % comm_size;
        Request::recv(tmp_buf, count, datatype, source, tag, comm, &status);

        if (is_commutative) {
          if(op!=MPI_OP_NULL) op->apply( tmp_buf, recvbuf, &count, datatype);
        } else {
          if(op!=MPI_OP_NULL) op->apply( recvbuf, tmp_buf, &count, datatype);
          Datatype::copy(tmp_buf, count, datatype,recvbuf, count, datatype);
        }
      }
    } else {
      dst = ((relrank & (~mask)) + lroot) % comm_size;
      Request::send(recvbuf, count, datatype, dst, tag, comm);
      break;
    }
    mask <<= 1;
  }

  if (not is_commutative && (root != 0)) {
    if (rank == 0){
      Request::send(recvbuf, count, datatype, root,tag, comm);
    }else if (rank == root){
      Request::recv(recvbuf, count, datatype, 0, tag, comm, &status);
    }
  }

  if (rank != root) {
    smpi_free_tmp_buffer(static_cast<unsigned char*>(recvbuf));
  }
  smpi_free_tmp_buffer(tmp_buf);

  return 0;
}
}
}
