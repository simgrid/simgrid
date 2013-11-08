#include "colls_private.h"
/* 
 * implemented by Pitch Patarasuk, 07/01/2007
 */
//#include <star-reduction.c>

/* change number of core per smp-node
   we assume that number of core per process will be the same for all implementations */
#ifndef NUM_CORE
#define NUM_CORE 8
#endif

/*
This fucntion performs all-reduce operation as follow.
1) binomial_tree reduce inside each SMP node
2) reduce-scatter -inter between root of each SMP node
3) allgather - inter between root of each SMP node
4) binomial_tree bcast inside each SMP node
*/
int smpi_coll_tuned_allreduce_smp_rsag_rab(void *sbuf, void *rbuf, int count,
                                           MPI_Datatype dtype, MPI_Op op,
                                           MPI_Comm comm)
{
  int comm_size, rank;
  void *tmp_buf;
  int tag = COLL_TAG_ALLREDUCE;
  int mask, src, dst;
  MPI_Status status;
  int num_core = NUM_CORE;

  comm_size = smpi_comm_size(comm);
  rank = smpi_comm_rank(comm);
  MPI_Aint extent;
  extent = smpi_datatype_get_extent(dtype);
  tmp_buf = (void *) xbt_malloc(count * extent);

  int intra_rank, inter_rank;
  intra_rank = rank % num_core;
  inter_rank = rank / num_core;

  //printf("node %d intra_rank = %d, inter_rank = %d\n", rank, intra_rank, inter_rank);

  int inter_comm_size = (comm_size + num_core - 1) / num_core;

  smpi_mpi_sendrecv(sbuf, count, dtype, rank, tag,
               rbuf, count, dtype, rank, tag, comm, &status);

  // SMP_binomial_reduce
  mask = 1;
  while (mask < num_core) {
    if ((mask & intra_rank) == 0) {
      src = (inter_rank * num_core) + (intra_rank | mask);
      //      if (src < ((inter_rank + 1) * num_core)) {
      if (src < comm_size) {
        smpi_mpi_recv(tmp_buf, count, dtype, src, tag, comm, &status);
        smpi_op_apply(op, tmp_buf, rbuf, &count, &dtype);
        //printf("Node %d recv from node %d when mask is %d\n", rank, src, mask);
      }
    } else {

      dst = (inter_rank * num_core) + (intra_rank & (~mask));
      smpi_mpi_send(rbuf, count, dtype, dst, tag, comm);
      //printf("Node %d send to node %d when mask is %d\n", rank, dst, mask);
      break;
    }
    mask <<= 1;
  }


  // INTER: reduce-scatter
  if (intra_rank == 0) {

    int dst, base_offset, send_base_offset, recv_base_offset, recv_chunk;
    int curr_count, i, recv_offset, send_offset;

    // reduce-scatter

    recv_chunk = extent * count / (comm_size / num_core);

    mask = 1;
    i = 0;
    curr_count = count / 2;
    int phase = 0;
    base_offset = 0;
    send_base_offset = 0;
    recv_base_offset = 0;

    while (mask < (comm_size / num_core)) {
      dst = inter_rank ^ mask;

      // compute offsets
      send_base_offset = base_offset;

      // right-handside
      if (inter_rank & mask) {
        recv_base_offset = base_offset + curr_count;
        send_base_offset = base_offset;
        base_offset = recv_base_offset;
      }
      // left-handside
      else {
        recv_base_offset = base_offset;
        send_base_offset = base_offset + curr_count;
      }
      send_offset = send_base_offset * extent;
      recv_offset = recv_base_offset * extent;

      //      if (rank==7)
      //      printf("node %d send to %d in phase %d s_offset = %d r_offset = %d count = %d\n",rank,dst,phase, send_offset, recv_offset, curr_count);

      smpi_mpi_sendrecv((char *)rbuf + send_offset, curr_count, dtype, (dst * num_core), tag,
                   tmp_buf, curr_count, dtype, (dst * num_core), tag,
                   comm, &status);

      smpi_op_apply(op, tmp_buf, (char *)rbuf + recv_offset, &curr_count, &dtype);

      mask *= 2;
      curr_count /= 2;
      phase++;
    }


    // INTER: allgather

    int size = (comm_size / num_core) / 2;
    base_offset = 0;
    mask = 1;
    while (mask < (comm_size / num_core)) {
      if (inter_rank & mask) {
        base_offset += size;
      }
      mask <<= 1;
      size /= 2;
    }

    curr_count *= 2;
    mask >>= 1;
    i = 1;
    phase = 0;
    while (mask >= 1) {
      // destination pair for both send and recv
      dst = inter_rank ^ mask;

      // compute offsets
      send_base_offset = base_offset;
      if (inter_rank & mask) {
        recv_base_offset = base_offset - i;
        base_offset -= i;
      } else {
        recv_base_offset = base_offset + i;
      }
      send_offset = send_base_offset * recv_chunk;
      recv_offset = recv_base_offset * recv_chunk;

      //      if (rank==7)
      //printf("node %d send to %d in phase %d s_offset = %d r_offset = %d count = %d\n",rank,dst,phase, send_offset, recv_offset, curr_count);

      smpi_mpi_sendrecv((char *)rbuf + send_offset, curr_count, dtype, (dst * num_core), tag,
                   (char *)rbuf + recv_offset, curr_count, dtype, (dst * num_core), tag,
                   comm, &status);


      curr_count *= 2;
      i *= 2;
      mask >>= 1;
      phase++;
    }


  }                             // INTER

  // intra SMP binomial bcast

  int num_core_in_current_smp = num_core;
  if (inter_rank == (inter_comm_size - 1)) {
    num_core_in_current_smp = comm_size - (inter_rank * num_core);
  }
  //  printf("Node %d num_core = %d\n",rank, num_core_in_current_smp);
  mask = 1;
  while (mask < num_core_in_current_smp) {
    if (intra_rank & mask) {
      src = (inter_rank * num_core) + (intra_rank - mask);
      //printf("Node %d recv from node %d when mask is %d\n", rank, src, mask);
      smpi_mpi_recv(rbuf, count, dtype, src, tag, comm, &status);
      break;
    }
    mask <<= 1;
  }

  mask >>= 1;
  //printf("My rank = %d my mask = %d\n", rank,mask);

  while (mask > 0) {
    dst = (inter_rank * num_core) + (intra_rank + mask);
    if (dst < comm_size) {
      //printf("Node %d send to node %d when mask is %d\n", rank, dst, mask);
      smpi_mpi_send(rbuf, count, dtype, dst, tag, comm);
    }
    mask >>= 1;
  }


  free(tmp_buf);
  return MPI_SUCCESS;
}
