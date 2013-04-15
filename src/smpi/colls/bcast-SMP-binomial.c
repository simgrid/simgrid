#include "colls_private.h"
#ifndef NUM_CORE
#define NUM_CORE 8
#endif

int smpi_coll_tuned_bcast_SMP_binomial(void *buf, int count,
                                       MPI_Datatype datatype, int root,
                                       MPI_Comm comm)
{
  int mask = 1;
  int size;
  int rank;
  MPI_Status status;
  int tag = 50;

  size = smpi_comm_size(comm);
  rank = smpi_comm_rank(comm);

  int to_intra, to_inter;
  int from_intra, from_inter;
  int inter_rank = rank / NUM_CORE;
  int inter_size = (size - 1) / NUM_CORE + 1;
  int intra_rank = rank % NUM_CORE;
  int intra_size = NUM_CORE;
  if (((rank / NUM_CORE) * NUM_CORE) == ((size / NUM_CORE) * NUM_CORE))
    intra_size = size - (rank / NUM_CORE) * NUM_CORE;

  // if root is not zero send to rank zero first
  if (root != 0) {
    if (rank == root)
      smpi_mpi_send(buf, count, datatype, 0, tag, comm);
    else if (rank == 0)
      smpi_mpi_recv(buf, count, datatype, root, tag, comm, &status);
  }
  //FIRST STEP node 0 send to every root-of-each-SMP with binomial tree

  //printf("node %d inter_rank = %d, inter_size = %d\n",rank,inter_rank, inter_size);

  if (intra_rank == 0) {
    mask = 1;
    while (mask < inter_size) {
      if (inter_rank & mask) {
        from_inter = (inter_rank - mask) * NUM_CORE;
        //printf("Node %d recv from node %d when mask is %d\n", rank, from_inter, mask);
        smpi_mpi_recv(buf, count, datatype, from_inter, tag, comm, &status);
        break;
      }
      mask <<= 1;
    }

    mask >>= 1;
    //printf("My rank = %d my mask = %d\n", rank,mask);

    while (mask > 0) {
      if (inter_rank < inter_size) {
        to_inter = (inter_rank + mask) * NUM_CORE;
        if (to_inter < size) {
          //printf("Node %d send to node %d when mask is %d\n", rank, to_inter, mask);
          smpi_mpi_send(buf, count, datatype, to_inter, tag, comm);
        }
      }
      mask >>= 1;
    }
  }
  // SECOND STEP every root-of-each-SMP send to all children with binomial tree
  // base is a rank of root-of-each-SMP
  int base = (rank / NUM_CORE) * NUM_CORE;
  mask = 1;
  while (mask < intra_size) {
    if (intra_rank & mask) {
      from_intra = base + (intra_rank - mask);
      //printf("Node %d recv from node %d when mask is %d\n", rank, from_inter, mask);
      smpi_mpi_recv(buf, count, datatype, from_intra, tag, comm, &status);
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
        smpi_mpi_send(buf, count, datatype, to_intra, tag, comm);
      }
    }
    mask >>= 1;
  }

  return MPI_SUCCESS;
}
