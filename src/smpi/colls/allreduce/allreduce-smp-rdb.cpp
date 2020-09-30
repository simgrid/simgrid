/* Copyright (c) 2013-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"
/* IMPLEMENTED BY PITCH PATARASUK
   Non-topology-specific (however, number of cores/node need to be changed)
   all-reduce operation designed for smp clusters
   It uses 2-layer communication: binomial for intra-communication
   and rdb for inter-communication*/

/* ** NOTE **
   Use -DMPICH2 if this code does not compile.
   MPICH1 code also work on MPICH2 on our cluster and the performance are similar.
   This code assume commutative and associative reduce operator (MPI_SUM, MPI_MAX, etc).
*/

//#include <star-reduction.c>

/*
This function performs all-reduce operation as follow.
1) binomial_tree reduce inside each SMP node
2) Recursive doubling intra-communication between root of each SMP node
3) binomial_tree bcast inside each SMP node
*/
namespace simgrid{
namespace smpi{
int allreduce__smp_rdb(const void *send_buf, void *recv_buf, int count,
                       MPI_Datatype dtype, MPI_Op op,
                       MPI_Comm comm)
{
  int comm_size, rank;
  int tag = COLL_TAG_ALLREDUCE;
  int mask, src, dst;
  MPI_Status status;
  if(comm->get_leaders_comm()==MPI_COMM_NULL){
    comm->init_smp();
  }
  int num_core=1;
  if (comm->is_uniform()){
    num_core = comm->get_intra_comm()->size();
  }
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
  comm_size = comm->size();
  rank = comm->rank();
  MPI_Aint extent;
  extent = dtype->get_extent();
  unsigned char* tmp_buf = smpi_get_tmp_sendbuffer(count * extent);

  /* compute intra and inter ranking */
  int intra_rank, inter_rank;
  intra_rank = rank % num_core;
  inter_rank = rank / num_core;

  /* size of processes participate in intra communications =>
     should be equal to number of machines */
  int inter_comm_size = (comm_size + num_core - 1) / num_core;

  /* copy input buffer to output buffer */
  Request::sendrecv(send_buf, count, dtype, rank, tag,
               recv_buf, count, dtype, rank, tag, comm, &status);

  /* start binomial reduce intra communication inside each SMP node */
  mask = 1;
  while (mask < num_core) {
    if ((mask & intra_rank) == 0) {
      src = (inter_rank * num_core) + (intra_rank | mask);
      if (src < comm_size) {
        Request::recv(tmp_buf, count, dtype, src, tag, comm, &status);
        if(op!=MPI_OP_NULL) op->apply( tmp_buf, recv_buf, &count, dtype);
      }
    } else {
      dst = (inter_rank * num_core) + (intra_rank & (~mask));
      Request::send(recv_buf, count, dtype, dst, tag, comm);
      break;
    }
    mask <<= 1;
  }                             /* end binomial reduce intra-communication */


  /* start rdb (recursive doubling) all-reduce inter-communication
     between each SMP nodes : each node only have one process that can communicate
     to other nodes */
  if (intra_rank == 0) {

    /* find nearest power-of-two less than or equal to inter_comm_size */
    int pof2, rem, newrank, newdst;
    pof2 = 1;
    while (pof2 <= inter_comm_size)
      pof2 <<= 1;
    pof2 >>= 1;
    rem = inter_comm_size - pof2;

    /* In the non-power-of-two case, all even-numbered
       processes of rank < 2*rem send their data to
       (rank+1). These even-numbered processes no longer
       participate in the algorithm until the very end.
     */
    if (inter_rank < 2 * rem) {
      if (inter_rank % 2 == 0) {
        dst = rank + num_core;
        Request::send(recv_buf, count, dtype, dst, tag, comm);
        newrank = -1;
      } else {
        src = rank - num_core;
        Request::recv(tmp_buf, count, dtype, src, tag, comm, &status);
        if(op!=MPI_OP_NULL) op->apply( tmp_buf, recv_buf, &count, dtype);
        newrank = inter_rank / 2;
      }
    } else {
      newrank = inter_rank - rem;
    }

    /* example inter-communication RDB rank change algorithm
       0,4,8,12..36 <= true rank (assume 4 core per SMP)
       0123 4567 89 <= inter_rank
       1 3 4567 89 (1,3 got data from 0,2 : 0,2 will be idle until the end)
       0 1 4567 89
       0 1 2345 67 => newrank
     */

    if (newrank != -1) {
      mask = 1;
      while (mask < pof2) {
        newdst = newrank ^ mask;
        /* find real rank of dest */
        dst = (newdst < rem) ? newdst * 2 + 1 : newdst + rem;
        dst *= num_core;

        /* exchange data in rdb manner */
        Request::sendrecv(recv_buf, count, dtype, dst, tag, tmp_buf, count, dtype,
                     dst, tag, comm, &status);
        if(op!=MPI_OP_NULL) op->apply( tmp_buf, recv_buf, &count, dtype);
        mask <<= 1;
      }
    }

    /* non pof2 case
       left-over processes (all even ranks: < 2 * rem) get the result
     */
    if (inter_rank < 2 * rem) {
      if (inter_rank % 2) {
        Request::send(recv_buf, count, dtype, rank - num_core, tag, comm);
      } else {
        Request::recv(recv_buf, count, dtype, rank + num_core, tag, comm, &status);
      }
    }
  }

  /* start binomial broadcast intra-communication inside each SMP nodes */
  int num_core_in_current_smp = num_core;
  if (inter_rank == (inter_comm_size - 1)) {
    num_core_in_current_smp = comm_size - (inter_rank * num_core);
  }
  mask = 1;
  while (mask < num_core_in_current_smp) {
    if (intra_rank & mask) {
      src = (inter_rank * num_core) + (intra_rank - mask);
      Request::recv(recv_buf, count, dtype, src, tag, comm, &status);
      break;
    }
    mask <<= 1;
  }
  mask >>= 1;

  while (mask > 0) {
    dst = (inter_rank * num_core) + (intra_rank + mask);
    if (dst < comm_size) {
      Request::send(recv_buf, count, dtype, dst, tag, comm);
    }
    mask >>= 1;
  }

  smpi_free_tmp_buffer(tmp_buf);
  return MPI_SUCCESS;
}
}
}
