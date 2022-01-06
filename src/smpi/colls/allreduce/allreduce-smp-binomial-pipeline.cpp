/* Copyright (c) 2013-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"
/* IMPLEMENTED BY PITCH PATARASUK
   Non-topology-specific (however, number of cores/node need to be changed)
   all-reduce operation designed for smp clusters
   It uses 2-layer communication: binomial for both intra-communication
   inter-communication
   The communication are done in a pipeline fashion */

/* this is a default segment size for pipelining,
   but it is typically passed as a command line argument */
int allreduce_smp_binomial_pipeline_segment_size = 4096;

/* ** NOTE **
   This code is modified from allreduce-smp-binomial.c by wrapping the code with pipeline effect as follow
   for (loop over pipelength) {
     smp-binomial main code;
   }
*/

/* ** NOTE **
   Use -DMPICH2 if this code does not compile.
   MPICH1 code also work on MPICH2 on our cluster and the performance are similar.
   This code assume commutative and associative reduce operator (MPI_SUM, MPI_MAX, etc).
*/

/*
This function performs all-reduce operation as follow. ** in a pipeline fashion **
1) binomial_tree reduce inside each SMP node
2) binomial_tree reduce intra-communication between root of each SMP node
3) binomial_tree bcast intra-communication between root of each SMP node
4) binomial_tree bcast inside each SMP node
*/
namespace simgrid{
namespace smpi{
int allreduce__smp_binomial_pipeline(const void *send_buf,
                                     void *recv_buf, int count,
                                     MPI_Datatype dtype,
                                     MPI_Op op, MPI_Comm comm)
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

  comm_size = comm->size();
  rank = comm->rank();
  MPI_Aint extent;
  extent = dtype->get_extent();
  unsigned char* tmp_buf = smpi_get_tmp_sendbuffer(count * extent);

  int intra_rank, inter_rank;
  intra_rank = rank % num_core;
  inter_rank = rank / num_core;

  int phase;
  int send_offset;
  int recv_offset;
  int pcount = allreduce_smp_binomial_pipeline_segment_size;
  if (pcount > count) {
    pcount = count;
  }

  /* size of processes participate in intra communications =>
     should be equal to number of machines */
  int inter_comm_size = (comm_size + num_core - 1) / num_core;

  /* copy input buffer to output buffer */
  Request::sendrecv(send_buf, count, dtype, rank, tag,
               recv_buf, count, dtype, rank, tag, comm, &status);

  /* compute pipe length */
  int pipelength;
  pipelength = count / pcount;

  /* pipelining over pipelength (+3 is because we have 4 stages:
     reduce-intra, reduce-inter, bcast-inter, bcast-intra */
  for (phase = 0; phase < pipelength + 3; phase++) {

    /* start binomial reduce intra communication inside each SMP node */
    if (phase < pipelength) {
      mask = 1;
      while (mask < num_core) {
        if ((mask & intra_rank) == 0) {
          src = (inter_rank * num_core) + (intra_rank | mask);
          if (src < comm_size) {
            recv_offset = phase * pcount * extent;
            Request::recv(tmp_buf, pcount, dtype, src, tag, comm, &status);
            if(op!=MPI_OP_NULL) op->apply( tmp_buf, (char *)recv_buf + recv_offset, &pcount, dtype);
          }
        } else {
          send_offset = phase * pcount * extent;
          dst = (inter_rank * num_core) + (intra_rank & (~mask));
          Request::send((char *)recv_buf + send_offset, pcount, dtype, dst, tag, comm);
          break;
        }
        mask <<= 1;
      }
    }

    /* start binomial reduce inter-communication between each SMP nodes:
       each node only have one process that can communicate to other nodes */
    if ((phase > 0) && (phase < (pipelength + 1))) {
      if (intra_rank == 0) {

        mask = 1;
        while (mask < inter_comm_size) {
          if ((mask & inter_rank) == 0) {
            src = (inter_rank | mask) * num_core;
            if (src < comm_size) {
              recv_offset = (phase - 1) * pcount * extent;
              Request::recv(tmp_buf, pcount, dtype, src, tag, comm, &status);
              if(op!=MPI_OP_NULL) op->apply( tmp_buf, (char *)recv_buf + recv_offset, &pcount, dtype);
            }
          } else {
            dst = (inter_rank & (~mask)) * num_core;
            send_offset = (phase - 1) * pcount * extent;
            Request::send((char *)recv_buf + send_offset, pcount, dtype, dst, tag, comm);
            break;
          }
          mask <<= 1;
        }
      }
    }

    /* start binomial broadcast inter-communication between each SMP nodes:
       each node only have one process that can communicate to other nodes */
    if ((phase > 1) && (phase < (pipelength + 2))) {
      if (intra_rank == 0) {
        mask = 1;
        while (mask < inter_comm_size) {
          if (inter_rank & mask) {
            src = (inter_rank - mask) * num_core;
            recv_offset = (phase - 2) * pcount * extent;
            Request::recv((char *)recv_buf + recv_offset, pcount, dtype, src, tag, comm,
                     &status);
            break;
          }
          mask <<= 1;
        }
        mask >>= 1;

        while (mask > 0) {
          if (inter_rank < inter_comm_size) {
            dst = (inter_rank + mask) * num_core;
            if (dst < comm_size) {
              //printf("Node %d send to node %d when mask is %d\n", rank, dst, mask);
              send_offset = (phase - 2) * pcount * extent;
              Request::send((char *)recv_buf + send_offset, pcount, dtype, dst, tag, comm);
            }
          }
          mask >>= 1;
        }
      }
    }

    /* start binomial broadcast intra-communication inside each SMP nodes */
    if (phase > 2) {
      int num_core_in_current_smp = num_core;
      if (inter_rank == (inter_comm_size - 1)) {
        num_core_in_current_smp = comm_size - (inter_rank * num_core);
      }
      mask = 1;
      while (mask < num_core_in_current_smp) {
        if (intra_rank & mask) {
          src = (inter_rank * num_core) + (intra_rank - mask);
          recv_offset = (phase - 3) * pcount * extent;
          Request::recv((char *)recv_buf + recv_offset, pcount, dtype, src, tag, comm,
                   &status);
          break;
        }
        mask <<= 1;
      }
      mask >>= 1;

      while (mask > 0) {
        dst = (inter_rank * num_core) + (intra_rank + mask);
        if (dst < comm_size) {
          send_offset = (phase - 3) * pcount * extent;
          Request::send((char *)recv_buf + send_offset, pcount, dtype, dst, tag, comm);
        }
        mask >>= 1;
      }
    }
  }                             // for phase

  smpi_free_tmp_buffer(tmp_buf);
  return MPI_SUCCESS;
}
}
}
