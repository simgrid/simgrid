#include "colls.h"
#include <math.h>

/*****************************************************************************

 * Function: alltoall_3dmesh_shoot

 * Return: int

 * Inputs:
    send_buff: send input buffer
    send_count: number of elements to send
    send_type: data type of elements being sent
    recv_buff: receive output buffer
    recv_count: number of elements to received
    recv_type: data type of elements being received
    comm: communicator

 * Descrp: Function realizes the alltoall operation using the 3dmesh
           algorithm. It actually performs allgather operation in x dimension,
           y dimension, then in z dimension. Each node then extracts the
           needed data. The communication in all dimension is simple.

 * Auther: Ahmad Faraj
****************************************************************************/

int alltoall_check_is_3dmesh(int num, int * i, int * j, int * k)
{
  int x, max = num / 3;
  x = cbrt(num);  
  * i = * j = * k = 0;
  while (x <= max)
    {
      if ((num % (x * x)) == 0)
	{
	  * i = * j = x;
	  * k = num / (x * x);
	  return 1;
	}
      x++;
    }
  return 0;
}

int smpi_coll_tuned_alltoall_3dmesh(void * send_buff, int send_count, MPI_Datatype send_type,
		      void * recv_buff, int recv_count, MPI_Datatype recv_type,
		      MPI_Comm comm)
{
  MPI_Request * reqs, * req_ptr;
  MPI_Aint extent;
  MPI_Status status, * statuses;
  int i, j, src, dst, rank, num_procs, num_reqs, X, Y, Z, block_size, count;
  int my_z, two_dsize, my_row_base, my_col_base, my_z_base, src_row_base;
  int src_z_base, send_offset, recv_offset, tag = 1, failure = 0, success = 1;

  char * tmp_buff1, * tmp_buff2;

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &num_procs);
  MPI_Type_extent(send_type, &extent);

  if (!alltoall_check_is_3dmesh(num_procs, &X, &Y, &Z))
     return failure;

  num_reqs = X;
  if (Y > X) num_reqs = Y;
  if (Z > Y) num_reqs = Z;

  two_dsize = X * Y;
  my_z = rank / two_dsize;  

  my_row_base = (rank / X) * X;
  my_col_base = (rank % Y) + (my_z * two_dsize);
  my_z_base = my_z * two_dsize;

  block_size = extent * send_count;

  tmp_buff1 =(char *) malloc(block_size * num_procs * two_dsize);
  if (!tmp_buff1)
    {
      printf("alltoall-3Dmesh:97: cannot allocate memory\n");
      MPI_Finalize();
      exit(failure);
    }
  
  tmp_buff2 =(char *) malloc(block_size *  two_dsize);
  if (!tmp_buff2)
    {
      printf("alltoall-3Dmesh:105: cannot allocate memory\n");
      MPI_Finalize();
      exit(failure);
    }
  
  statuses = (MPI_Status *) malloc(num_reqs * sizeof(MPI_Status));  
  reqs = (MPI_Request *) malloc(num_reqs * sizeof(MPI_Request));  
  if (!reqs)
    {
      printf("alltoall-3Dmesh:113: cannot allocate memory\n");
      MPI_Finalize();
      exit(failure);
    }
  
  req_ptr = reqs;
  
  send_offset = recv_offset = (rank % two_dsize) * block_size * num_procs;

  MPI_Sendrecv(send_buff, send_count * num_procs, send_type, rank, tag, 
	       tmp_buff1 + recv_offset, num_procs * recv_count,
	       recv_type, rank, tag, comm, &status);

  count = send_count * num_procs;

  for (i = 0; i < Y; i++)
    {
      src = i + my_row_base;
      if (src == rank) continue;
      recv_offset = (src % two_dsize) * block_size * num_procs;
      MPI_Irecv(tmp_buff1 + recv_offset, count, recv_type, src, tag, comm,
		req_ptr++);
    }

  for (i = 0; i < Y; i++)
    {
      dst = i + my_row_base;
      if (dst == rank) continue;
      MPI_Send(send_buff, count, send_type, dst, tag, comm);
    }  

  MPI_Waitall(Y - 1, reqs, statuses);
  req_ptr = reqs;
  

  for (i = 0; i < X; i++)
    {
      src = (i * Y + my_col_base);
      if (src == rank) continue;
     
      src_row_base = (src / X) * X;

      recv_offset = (src_row_base % two_dsize) * block_size * num_procs;
      MPI_Irecv(tmp_buff1 + recv_offset, recv_count * num_procs * Y,
		 recv_type, src, tag, comm, req_ptr++);
    }

  send_offset = (my_row_base % two_dsize) * block_size * num_procs;  
  for (i = 0; i < X; i++)
    {
      dst = (i * Y + my_col_base);
      if (dst == rank) continue;
      MPI_Send(tmp_buff1 + send_offset, send_count * num_procs * Y, send_type,
		dst, tag, comm);
    }
  
  MPI_Waitall(X - 1, reqs, statuses);
  req_ptr = reqs;

  for (i = 0; i < two_dsize; i++)
    {
      send_offset = (rank * block_size) + (i * block_size * num_procs);
      recv_offset = (my_z_base * block_size) + (i * block_size);
      MPI_Sendrecv(tmp_buff1 + send_offset, send_count, send_type, rank, tag,
		   recv_buff + recv_offset, recv_count, recv_type, rank, tag,
		   comm, &status);
    }  

  for (i = 1; i < Z; i++)
    {
      src = (rank + i * two_dsize) % num_procs;
      src_z_base = (src / two_dsize) * two_dsize;

      recv_offset = (src_z_base * block_size);

      MPI_Irecv(recv_buff + recv_offset, recv_count * two_dsize, recv_type,
		 src, tag, comm, req_ptr++);
  }

  for (i = 1; i < Z; i++)
    {
      dst = (rank + i * two_dsize) % num_procs;
     
      recv_offset = 0;
      for (j = 0; j < two_dsize; j++)
	{
	  send_offset = (dst + j * num_procs) * block_size;
	  MPI_Sendrecv(tmp_buff1 + send_offset, send_count, send_type,
		       rank, tag, tmp_buff2 + recv_offset, recv_count,
		       recv_type, rank, tag, comm, &status);
	  
	  recv_offset += block_size;
	}

      MPI_Send(tmp_buff2, send_count * two_dsize, send_type, dst, tag, comm);
      
    }
  
  MPI_Waitall(Z - 1, reqs, statuses);

  free(reqs);
  free(statuses);
  free(tmp_buff1);
  free(tmp_buff2);
  return success;
}
