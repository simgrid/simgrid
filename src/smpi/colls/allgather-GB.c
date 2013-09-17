#include "colls_private.h"

// Allgather - gather/bcast algorithm
int smpi_coll_tuned_allgather_GB(void *send_buff, int send_count,
                                 MPI_Datatype send_type, void *recv_buff,
                                 int recv_count, MPI_Datatype recv_type,
                                 MPI_Comm comm)
{
  int num_procs;
  num_procs = smpi_comm_size(comm);
  mpi_coll_gather_fun(send_buff, send_count, send_type, recv_buff, recv_count, recv_type,
             0, comm);
  mpi_coll_bcast_fun(recv_buff, (recv_count * num_procs), recv_type, 0, comm);

  return MPI_SUCCESS;
}
