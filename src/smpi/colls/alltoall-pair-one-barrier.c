#include "colls.h"
/*****************************************************************************

 * Function: alltoall_pair

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
           phases, nodes in pair communicate their data.

 * Auther: Ahmad Faraj

 ****************************************************************************/
int
smpi_coll_tuned_alltoall_pair_one_barrier(void *send_buff, int send_count,
                                          MPI_Datatype send_type,
                                          void *recv_buff, int recv_count,
                                          MPI_Datatype recv_type, MPI_Comm comm)
{

  MPI_Aint send_chunk, recv_chunk;
  MPI_Status s;
  int i, src, dst, rank, num_procs;
  int tag = 1, success = 1;     /*, failure = 0, pof2 = 1; */

  char *send_ptr = (char *) send_buff;
  char *recv_ptr = (char *) recv_buff;

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &num_procs);
  MPI_Type_extent(send_type, &send_chunk);
  MPI_Type_extent(recv_type, &recv_chunk);

  send_chunk *= send_count;
  recv_chunk *= recv_count;

  MPI_Barrier(comm);
  for (i = 0; i < num_procs; i++) {
    src = dst = rank ^ i;
    MPI_Sendrecv(send_ptr + dst * send_chunk, send_count, send_type, dst,
                 tag, recv_ptr + src * recv_chunk, recv_count, recv_type,
                 src, tag, comm, &s);
  }

  return success;
}
