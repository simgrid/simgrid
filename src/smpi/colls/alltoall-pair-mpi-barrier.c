#include "colls_private.h"
/*****************************************************************************

 * Function: alltoall_pair_mpi_barrier

 * Return: int

 * Inputs:
    send_buff: send input buffer
    send_count: number of elements to send
    send_type: data type of elements being sent
    recv_buff: receive output buffer
    recv_count: number of elements to received
    recv_type: data type of elements being received
    comm: communicator

 * Descrp: Function works when P is power of two. In each phase of P - 1
           phases, nodes in pair communicate their data. MPI barriers are
           inserted between each two phases.

 * Auther: Ahmad Faraj

 ****************************************************************************/
int
smpi_coll_tuned_alltoall_pair_mpi_barrier(void *send_buff, int send_count,
                                          MPI_Datatype send_type,
                                          void *recv_buff, int recv_count,
                                          MPI_Datatype recv_type, MPI_Comm comm)
{
  MPI_Status s;
  MPI_Aint send_chunk, recv_chunk;
  int i, src, dst, rank, num_procs;
  int tag = 101;
  char *send_ptr = (char *) send_buff;
  char *recv_ptr = (char *) recv_buff;

  rank = smpi_comm_rank(comm);
  num_procs = smpi_comm_size(comm);
  send_chunk = smpi_datatype_get_extent(send_type);
  recv_chunk = smpi_datatype_get_extent(recv_type);

  send_chunk *= send_count;
  recv_chunk *= recv_count;

  for (i = 0; i < num_procs; i++) {
    src = dst = rank ^ i;
    smpi_mpi_barrier(comm);
    smpi_mpi_sendrecv(send_ptr + dst * send_chunk, send_count, send_type, dst,
                 tag, recv_ptr + src * recv_chunk, recv_count, recv_type,
                 src, tag, comm, &s);
  }
  return MPI_SUCCESS;
}
