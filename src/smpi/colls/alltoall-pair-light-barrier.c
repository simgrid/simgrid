#include "colls_private.h"
/*****************************************************************************

 * Function: alltoall_pair_light_barrier

 * Return: int

 * Inputs:
    send_buff: send input buffer
    send_count: number of elements to send
    send_type: data type of elements being sent
    recv_buff: receive output buffer
    recv_count: number of elements to received
    recv_type: data type of elements being received
    comm: communicator

 * Descrp: Function works in P - 1 steps. In step i, node j exchanges data
           with node i ^ j. Light barriers are inserted between
           communications in different phases.

 * Auther: Ahmad Faraj

 ****************************************************************************/
int
smpi_coll_tuned_alltoall_pair_light_barrier(void *send_buff, int send_count,
                                            MPI_Datatype send_type,
                                            void *recv_buff, int recv_count,
                                            MPI_Datatype recv_type,
                                            MPI_Comm comm)
{
  MPI_Aint send_chunk, recv_chunk;
  MPI_Status s;
  int i, src, dst, rank, num_procs, next_partner;
  int tag = 1;     /*, failure = 0; */

  char send_sync = 'a', recv_sync = 'b';
  char *send_ptr = (char *) send_buff;
  char *recv_ptr = (char *) recv_buff;

  rank = smpi_comm_rank(comm);
  num_procs = smpi_comm_size(comm);
  send_chunk = smpi_datatype_get_extent(send_type);
  recv_chunk = smpi_datatype_get_extent(recv_type);

  send_chunk *= send_count;
  recv_chunk *= recv_count;

  smpi_mpi_sendrecv(send_ptr + rank * send_chunk, send_count, send_type, rank, tag,
               recv_ptr + rank * recv_chunk, recv_count, recv_type, rank, tag,
               comm, &s);

  for (i = 1; i < num_procs; i++) {
    src = dst = rank ^ i;

    smpi_mpi_sendrecv(send_ptr + dst * send_chunk, send_count, send_type,
                 dst, tag, recv_ptr + src * recv_chunk, recv_count,
                 recv_type, src, tag, comm, &s);

    if ((i + 1) < num_procs) {
      next_partner = rank ^ (i + 1);
      smpi_mpi_sendrecv(&send_sync, 1, MPI_CHAR, next_partner, tag,
                   &recv_sync, 1, MPI_CHAR, next_partner, tag, comm, &s);
    }
  }
  return MPI_SUCCESS;
}
