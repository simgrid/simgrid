#include "colls.h"

/*****************************************************************************

 * Function: alltoall_rdb

 * Return: int

 * inputs: 
    send_buff: send input buffer
    send_count: number of elements to send
    send_type: data type of elements being sent
    recv_buff: receive output buffer
    recv_count: number of elements to received
    recv_type: data type of elements being received
    comm: communicator

 * Descrp: Function realizes the allgather operation using the recursive
           doubling algorithm.

 * Auther: MPICH / slightly modified by Ahmad Faraj.  

 ****************************************************************************/
int smpi_coll_tuned_alltoall_rdb(void *send_buff, int send_count,
                                 MPI_Datatype send_type,
                                 void *recv_buff, int recv_count,
                                 MPI_Datatype recv_type,
                                 MPI_Comm comm)
{
  /* MPI variables */
  MPI_Status status;
  MPI_Aint send_increment, recv_increment, extent;

  int dst_tree_root, rank_tree_root, send_offset, recv_offset;
  int rank, num_procs, j, k, dst, curr_size, max_size;
  int last_recv_count, tmp_mask, tree_root, num_procs_completed;
  int tag = 1, mask = 1, success = 1, failure = 0, i = 0;

  char *tmp_buff;
  char *send_ptr = (char *) send_buff;
  char *recv_ptr = (char *) recv_buff;

  MPI_Comm_size(comm, &num_procs);
  MPI_Comm_rank(comm, &rank);
  MPI_Type_extent(send_type, &send_increment);
  MPI_Type_extent(recv_type, &recv_increment);
  MPI_Type_extent(recv_type, &extent);

  send_increment *= (send_count * num_procs);
  recv_increment *= (recv_count * num_procs);

  max_size = num_procs * recv_increment;

  tmp_buff = (char *) malloc(max_size);
  if (!tmp_buff) {
    printf("alltoall-rdb:56: cannot allocate memory\n");
    MPI_Finalize();
    exit(failure);
  }

  curr_size = send_count * num_procs;

  MPI_Sendrecv(send_ptr, curr_size, send_type, rank, tag,
               tmp_buff + (rank * recv_increment),
               curr_size, recv_type, rank, tag, comm, &status);

  while (mask < num_procs) {
    dst = rank ^ mask;
    dst_tree_root = dst >> i;
    dst_tree_root <<= i;
    rank_tree_root = rank >> i;
    rank_tree_root <<= i;
    send_offset = rank_tree_root * send_increment;
    recv_offset = dst_tree_root * recv_increment;

    if (dst < num_procs) {
      MPI_Sendrecv(tmp_buff + send_offset, curr_size, send_type, dst, tag,
                   tmp_buff + recv_offset, mask * recv_count * num_procs,
                   recv_type, dst, tag, comm, &status);

      MPI_Get_count(&status, recv_type, &last_recv_count);
      curr_size += last_recv_count;
    }


    if (dst_tree_root + mask > num_procs) {

      num_procs_completed = num_procs - rank_tree_root - mask;
      /* num_procs_completed is the number of processes in this
         subtree that have all the data. Send data to others
         in a tree fashion. First find root of current tree
         that is being divided into two. k is the number of
         least-significant bits in this process's rank that
         must be zeroed out to find the rank of the root */

      j = mask;
      k = 0;
      while (j) {
        j >>= 1;
        k++;
      }
      k--;

      tmp_mask = mask >> 1;

      while (tmp_mask) {
        dst = rank ^ tmp_mask;

        tree_root = rank >> k;
        tree_root <<= k;

        /* send only if this proc has data and destination
           doesn't have data. at any step, multiple processes
           can send if they have the data */

        if ((dst > rank)
            && (rank < tree_root + num_procs_completed)
            && (dst >= tree_root + num_procs_completed)) {
          MPI_Send(tmp_buff + dst_tree_root * send_increment,
                   last_recv_count, send_type, dst, tag, comm);

        }

        /* recv only if this proc. doesn't have data and sender
           has data */

        else if ((dst < rank)
                 && (dst < tree_root + num_procs_completed)
                 && (rank >= tree_root + num_procs_completed)) {
          MPI_Recv(tmp_buff + dst_tree_root * send_increment,
                   mask * num_procs * send_count, send_type, dst,
                   tag, comm, &status);

          MPI_Get_count(&status, send_type, &last_recv_count);
          curr_size += last_recv_count;
        }

        tmp_mask >>= 1;
        k--;
      }
    }

    mask <<= 1;
    i++;
  }

  for (i = 0; i < num_procs; i++)
    MPI_Sendrecv(tmp_buff + (rank + i * num_procs) * send_count * extent,
                 send_count, send_type, rank, tag,
                 recv_ptr + (i * recv_count * extent),
                 recv_count, recv_type, rank, tag, comm, &status);
  free(tmp_buff);
  return success;
}
