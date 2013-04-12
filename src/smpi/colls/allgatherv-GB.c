#include "colls_private.h"

// Allgather - gather/bcast algorithm
int smpi_coll_tuned_allgatherv_GB(void *send_buff, int send_count,
                                 MPI_Datatype send_type, void *recv_buff,
                                 int *recv_counts, int *recv_disps, MPI_Datatype recv_type,
                                 MPI_Comm comm)
{
  smpi_mpi_gatherv(send_buff, send_count, send_type, recv_buff, recv_counts,
		   recv_disps, recv_type, 0, comm);
  int num_procs, i, current, max = 0;
  num_procs = smpi_comm_size(comm);
  for (i = 0; i < num_procs; i++) {
    current = recv_disps[i] + recv_counts[i];
    if (current > max) max = current;
  }
  mpi_coll_bcast_fun(recv_buff, current, recv_type, 0, comm);

  return MPI_SUCCESS;
}
