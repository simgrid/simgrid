/* Copyright (c) 2013-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"
namespace simgrid{
namespace smpi{
int bcast__SMP_binomial(void *buf, int count,
                        MPI_Datatype datatype, int root,
                        MPI_Comm comm)
{
  int mask = 1;
  int size;
  int rank;
  MPI_Status status;
  int tag = COLL_TAG_BCAST;

  size = comm->size();
  rank = comm->rank();

  if(comm->get_leaders_comm()==MPI_COMM_NULL){
    comm->init_smp();
  }
  int num_core=1;
  if (comm->is_uniform()){
    num_core = comm->get_intra_comm()->size();
  }else{
    //implementation buggy in this case
    return bcast__mpich(buf, count, datatype, root, comm);
  }

  int to_intra, to_inter;
  int from_intra, from_inter;
  int inter_rank = rank / num_core;
  int inter_size = (size - 1) / num_core + 1;
  int intra_rank = rank % num_core;
  int intra_size = num_core;
  if (((rank / num_core) * num_core) == ((size / num_core) * num_core))
    intra_size = size - (rank / num_core) * num_core;

  // if root is not zero send to rank zero first
  if (root != 0) {
    if (rank == root)
      Request::send(buf, count, datatype, 0, tag, comm);
    else if (rank == 0)
      Request::recv(buf, count, datatype, root, tag, comm, &status);
  }
  //FIRST STEP node 0 send to every root-of-each-SMP with binomial tree

  //printf("node %d inter_rank = %d, inter_size = %d\n",rank,inter_rank, inter_size);

  if (intra_rank == 0) {
    mask = 1;
    while (mask < inter_size) {
      if (inter_rank & mask) {
        from_inter = (inter_rank - mask) * num_core;
        //printf("Node %d recv from node %d when mask is %d\n", rank, from_inter, mask);
        Request::recv(buf, count, datatype, from_inter, tag, comm, &status);
        break;
      }
      mask <<= 1;
    }

    mask >>= 1;
    //printf("My rank = %d my mask = %d\n", rank,mask);

    while (mask > 0) {
      if (inter_rank < inter_size) {
        to_inter = (inter_rank + mask) * num_core;
        if (to_inter < size) {
          //printf("Node %d send to node %d when mask is %d\n", rank, to_inter, mask);
          Request::send(buf, count, datatype, to_inter, tag, comm);
        }
      }
      mask >>= 1;
    }
  }
  // SECOND STEP every root-of-each-SMP send to all children with binomial tree
  // base is a rank of root-of-each-SMP
  int base = (rank / num_core) * num_core;
  mask = 1;
  while (mask < intra_size) {
    if (intra_rank & mask) {
      from_intra = base + (intra_rank - mask);
      //printf("Node %d recv from node %d when mask is %d\n", rank, from_inter, mask);
      Request::recv(buf, count, datatype, from_intra, tag, comm, &status);
      break;
    }
    mask <<= 1;
  }

  mask >>= 1;

  //printf("My rank = %d my mask = %d\n", rank,mask);

  while (mask > 0) {
    if (intra_rank < intra_size) {
      to_intra = base + (intra_rank + mask);
      if (to_intra < size) {
        //printf("Node %d send to node %d when mask is %d\n", rank, to_inter, mask);
        Request::send(buf, count, datatype, to_intra, tag, comm);
      }
    }
    mask >>= 1;
  }

  return MPI_SUCCESS;
}

}
}
