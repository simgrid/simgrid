/* Copyright (c) 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "colls_private.h"

//#include <star-reduction.c>

int smpi_coll_tuned_reduce_binomial(void *sendbuf, void *recvbuf, int count,
                                    MPI_Datatype datatype, MPI_Op op, int root,
                                    MPI_Comm comm)
{
  MPI_Status status;
  int comm_size, rank;
  int mask, relrank, source;
  int dst;
  int tag = COLL_TAG_REDUCE;
  MPI_Aint extent;
  void *tmp_buf;
  MPI_Aint true_lb, true_extent;
  if (count == 0)
    return 0;
  rank = smpi_comm_rank(comm);
  comm_size = smpi_comm_size(comm);

  extent = smpi_datatype_get_extent(datatype);

  tmp_buf = (void *) smpi_get_tmp_sendbuffer(count * extent);
  int is_commutative = smpi_op_is_commute(op);
  mask = 1;
  
  int lroot;
  if (is_commutative) 
        lroot   = root;
  else
        lroot   = 0;
  relrank = (rank - lroot + comm_size) % comm_size;

  smpi_datatype_extent(datatype, &true_lb, &true_extent);

  /* adjust for potential negative lower bound in datatype */
  tmp_buf = (void *)((char*)tmp_buf - true_lb);
    
  /* If I'm not the root, then my recvbuf may not be valid, therefore
     I have to allocate a temporary one */
  if (rank != root) {
      recvbuf = (void *) smpi_get_tmp_recvbuffer(count*(MAX(extent,true_extent)));
      recvbuf = (void *)((char*)recvbuf - true_lb);
  }
   if ((rank != root) || (sendbuf != MPI_IN_PLACE)) {
      smpi_datatype_copy(sendbuf, count, datatype, recvbuf,count, datatype);
  }

  while (mask < comm_size) {
    /* Receive */
    if ((mask & relrank) == 0) {
      source = (relrank | mask);
      if (source < comm_size) {
        source = (source + lroot) % comm_size;
        smpi_mpi_recv(tmp_buf, count, datatype, source, tag, comm, &status);
        
        if (is_commutative) {
          smpi_op_apply(op, tmp_buf, recvbuf, &count, &datatype);
        } else {
          smpi_op_apply(op, recvbuf, tmp_buf, &count, &datatype);
          smpi_datatype_copy(tmp_buf, count, datatype,recvbuf, count, datatype);
        }
      }
    } else {
      dst = ((relrank & (~mask)) + lroot) % comm_size;
      smpi_mpi_send(recvbuf, count, datatype, dst, tag, comm);
      break;
    }
    mask <<= 1;
  }

  if (!is_commutative && (root != 0)){
    if (rank == 0){
      smpi_mpi_send(recvbuf, count, datatype, root,tag, comm);
    }else if (rank == root){
      smpi_mpi_recv(recvbuf, count, datatype, 0, tag, comm, &status);
    }
  }

  if (rank != root) {
	  smpi_free_tmp_buffer(recvbuf);
  }
  smpi_free_tmp_buffer(tmp_buf);

  return 0;
}
