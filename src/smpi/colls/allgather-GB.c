#include "colls.h"

// Allgather - gather/bcast algorithm
int smpi_coll_tuned_allgather_GB(void *send_buff, int send_count,
                                 MPI_Datatype send_type, void *recv_buff,
                                 int recv_count, MPI_Datatype recv_type,
                                 MPI_Comm comm)
{
  int num_procs;
  MPI_Comm_size(comm, &num_procs);
  MPI_Gather(send_buff, send_count, send_type, recv_buff, recv_count, recv_type,
             0, comm);
  MPI_Bcast(recv_buff, (recv_count * num_procs), recv_type, 0, comm);

  return MPI_SUCCESS;
}
