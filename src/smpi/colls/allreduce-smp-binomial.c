#include "colls.h"
/* IMPLEMENTED BY PITCH PATARASUK 
   Non-topoloty-specific (however, number of cores/node need to be changed) 
   all-reduce operation designed for smp clusters
   It uses 2-layer communication: binomial for both intra-communication 
   inter-communication*/

/* change number of core per smp-node
   we assume that number of core per process will be the same for all implementations */
#ifndef NUM_CORE
#define NUM_CORE 8
#endif

/* ** NOTE **
   Use -DMPICH2 if this code does not compile.
   MPICH1 code also work on MPICH2 on our cluster and the performance are similar.
   This code assume commutative and associative reduce operator (MPI_SUM, MPI_MAX, etc).
*/

//#include <star-reduction.c>

/*
This fucntion performs all-reduce operation as follow.
1) binomial_tree reduce inside each SMP node
2) binomial_tree reduce intra-communication between root of each SMP node
3) binomial_tree bcast intra-communication between root of each SMP node
4) binomial_tree bcast inside each SMP node
*/
int smpi_coll_tuned_allreduce_smp_binomial(void *send_buf, void *recv_buf,
                                           int count, MPI_Datatype dtype,
                                           MPI_Op op, MPI_Comm comm)
{
  int comm_size, rank;
  void *tmp_buf;
  int tag = 50;
  int mask, src, dst;
  int num_core = NUM_CORE;
  MPI_Status status;
  /*
     #ifdef MPICH2_REDUCTION
     MPI_User_function * uop = MPIR_Op_table[op % 16 - 1];
     #else
     MPI_User_function *uop;
     struct MPIR_OP *op_ptr;
     op_ptr = MPIR_ToPointer(op);
     uop  = op_ptr->op;
     #endif
   */

  comm_size=smpi_comm_size(comm);
  rank=smpi_comm_rank(comm);
  MPI_Aint extent, lb;
  smpi_datatype_extent(dtype, &lb, &extent);
  tmp_buf = (void *) malloc(count * extent);

  /* compute intra and inter ranking */
  int intra_rank, inter_rank;
  intra_rank = rank % num_core;
  inter_rank = rank / num_core;

  /* size of processes participate in intra communications =>
     should be equal to number of machines */
  int inter_comm_size = (comm_size + num_core - 1) / num_core;

  /* copy input buffer to output buffer */
  smpi_mpi_sendrecv(send_buf, count, dtype, rank, tag,
               recv_buf, count, dtype, rank, tag, comm, &status);

  /* start binomial reduce intra communication inside each SMP node */
  mask = 1;
  while (mask < num_core) {
    if ((mask & intra_rank) == 0) {
      src = (inter_rank * num_core) + (intra_rank | mask);
      if (src < comm_size) {
        smpi_mpi_recv(tmp_buf, count, dtype, src, tag, comm, &status);
        star_reduction(op, tmp_buf, recv_buf, &count, &dtype);
      }
    } else {
      dst = (inter_rank * num_core) + (intra_rank & (~mask));
      smpi_mpi_send(recv_buf, count, dtype, dst, tag, comm);
      break;
    }
    mask <<= 1;
  }

  /* start binomial reduce inter-communication between each SMP nodes: 
     each node only have one process that can communicate to other nodes */
  if (intra_rank == 0) {
    mask = 1;
    while (mask < inter_comm_size) {
      if ((mask & inter_rank) == 0) {
        src = (inter_rank | mask) * num_core;
        if (src < comm_size) {
          smpi_mpi_recv(tmp_buf, count, dtype, src, tag, comm, &status);
          star_reduction(op, tmp_buf, recv_buf, &count, &dtype);
        }
      } else {
        dst = (inter_rank & (~mask)) * num_core;
        smpi_mpi_send(recv_buf, count, dtype, dst, tag, comm);
        break;
      }
      mask <<= 1;
    }
  }

  /* start binomial broadcast inter-communication between each SMP nodes: 
     each node only have one process that can communicate to other nodes */
  if (intra_rank == 0) {
    mask = 1;
    while (mask < inter_comm_size) {
      if (inter_rank & mask) {
        src = (inter_rank - mask) * num_core;
        smpi_mpi_recv(recv_buf, count, dtype, src, tag, comm, &status);
        break;
      }
      mask <<= 1;
    }
    mask >>= 1;

    while (mask > 0) {
      if (inter_rank < inter_comm_size) {
        dst = (inter_rank + mask) * num_core;
        if (dst < comm_size) {
          smpi_mpi_send(recv_buf, count, dtype, dst, tag, comm);
        }
      }
      mask >>= 1;
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
      smpi_mpi_recv(recv_buf, count, dtype, src, tag, comm, &status);
      break;
    }
    mask <<= 1;
  }
  mask >>= 1;

  while (mask > 0) {
    dst = (inter_rank * num_core) + (intra_rank + mask);
    if (dst < comm_size) {
      smpi_mpi_send(recv_buf, count, dtype, dst, tag, comm);
    }
    mask >>= 1;
  }

  free(tmp_buf);
  return MPI_SUCCESS;
}
