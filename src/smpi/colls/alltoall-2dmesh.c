#include "colls.h"
#include <math.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_colls, smpi,
                                "Logging specific to SMPI collectives");

/*****************************************************************************

 * Function: alltoall_2dmesh_shoot

 * Return: int

 * Inputs:
    send_buff: send input buffer
    send_count: number of elements to send
    send_type: data type of elements being sent
    recv_buff: receive output buffer
    recv_count: number of elements to received
    recv_type: data type of elements being received
    comm: communicator

 * Descrp: Function realizes the alltoall operation using the 2dmesh
           algorithm. It actually performs allgather operation in x dimension
           then in the y dimension. Each node then extracts the needed data.
           The communication in each dimension follows "simple."
 
 * Auther: Ahmad Faraj

****************************************************************************/
int alltoall_check_is_2dmesh(int num, int *i, int *j)
{
  int x, max = num / 2;
  x = sqrt(num);

  while (x <= max) {
    if ((num % x) == 0) {
      *i = x;
      *j = num / x;

      if (*i > *j) {
        x = *i;
        *i = *j;
        *j = x;
      }

      return 1;
    }
    x++;
  }
  return 0;
}

int smpi_coll_tuned_alltoall_2dmesh(void *send_buff, int send_count,
                                    MPI_Datatype send_type,
                                    void *recv_buff, int recv_count,
                                    MPI_Datatype recv_type,
                                    MPI_Comm comm)
{
  MPI_Status *statuses, s;
  MPI_Request *reqs, *req_ptr;;
  MPI_Aint extent;

  char *tmp_buff1, *tmp_buff2;
  int i, j, src, dst, rank, num_procs, count, num_reqs;
  int rows, cols, my_row, my_col, X, Y, send_offset, recv_offset;
  int two_dsize, my_row_base, my_col_base, src_row_base, block_size;
  int tag = 1, failure = 0, success = 1;

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &num_procs);
  MPI_Type_extent(send_type, &extent);

  if (!alltoall_check_is_2dmesh(num_procs, &X, &Y))
    return failure;

  two_dsize = X * Y;

  my_row_base = (rank / Y) * Y;
  my_col_base = rank % Y;

  block_size = extent * send_count;

  tmp_buff1 = (char *) malloc(block_size * num_procs * Y);
  if (!tmp_buff1) {
    XBT_DEBUG("alltoall-2dmesh_shoot.c:88: cannot allocate memory");
    MPI_Finalize();
    exit(failure);
  }

  tmp_buff2 = (char *) malloc(block_size * Y);
  if (!tmp_buff2) {
    XBT_WARN("alltoall-2dmesh_shoot.c:88: cannot allocate memory");
    MPI_Finalize();
    exit(failure);
  }



  num_reqs = X;
  if (Y > X)
    num_reqs = Y;

  statuses = (MPI_Status *) malloc(num_reqs * sizeof(MPI_Status));
  reqs = (MPI_Request *) malloc(num_reqs * sizeof(MPI_Request));
  if (!reqs) {
    XBT_WARN("alltoall-2dmesh_shoot.c:88: cannot allocate memory");
    MPI_Finalize();
    exit(failure);
  }

  req_ptr = reqs;

  send_offset = recv_offset = (rank % Y) * block_size * num_procs;

  count = send_count * num_procs;

  for (i = 0; i < Y; i++) {
    src = i + my_row_base;
    if (src == rank)
      continue;

    recv_offset = (src % Y) * block_size * num_procs;
    MPI_Irecv(tmp_buff1 + recv_offset, count, recv_type, src, tag, comm,
              req_ptr++);
  }

  for (i = 0; i < Y; i++) {
    dst = i + my_row_base;
    if (dst == rank)
      continue;
    MPI_Send(send_buff, count, send_type, dst, tag, comm);
  }

  MPI_Waitall(Y - 1, reqs, statuses);
  req_ptr = reqs;

  for (i = 0; i < Y; i++) {
    send_offset = (rank * block_size) + (i * block_size * num_procs);
    recv_offset = (my_row_base * block_size) + (i * block_size);

    if (i + my_row_base == rank)
      MPI_Sendrecv(send_buff + recv_offset, send_count, send_type,
                   rank, tag, recv_buff + recv_offset, recv_count,
                   recv_type, rank, tag, comm, &s);

    else
      MPI_Sendrecv(tmp_buff1 + send_offset, send_count, send_type,
                   rank, tag,
                   recv_buff + recv_offset, recv_count, recv_type,
                   rank, tag, comm, &s);
  }


  for (i = 0; i < X; i++) {
    src = (i * Y + my_col_base);
    if (src == rank)
      continue;
    src_row_base = (src / Y) * Y;

    MPI_Irecv(recv_buff + src_row_base * block_size, recv_count * Y,
              recv_type, src, tag, comm, req_ptr++);
  }

  for (i = 0; i < X; i++) {
    dst = (i * Y + my_col_base);
    if (dst == rank)
      continue;

    recv_offset = 0;
    for (j = 0; j < Y; j++) {
      send_offset = (dst + j * num_procs) * block_size;

      if (j + my_row_base == rank)
        MPI_Sendrecv(send_buff + dst * block_size, send_count, send_type,
                     rank, tag,
                     tmp_buff2 + recv_offset, recv_count, recv_type,
                     rank, tag, comm, &s);
      else
        MPI_Sendrecv(tmp_buff1 + send_offset, send_count, send_type,
                     rank, tag,
                     tmp_buff2 + recv_offset, recv_count, recv_type,
                     rank, tag, comm, &s);

      recv_offset += block_size;
    }

    MPI_Send(tmp_buff2, send_count * Y, send_type, dst, tag, comm);
  }
  MPI_Waitall(X - 1, reqs, statuses);
  free(reqs);
  free(statuses);
  free(tmp_buff1);
  free(tmp_buff2);
  return success;
}
