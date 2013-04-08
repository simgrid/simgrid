#include "colls_private.h"

int
smpi_coll_tuned_bcast_flattree(void *buff, int count, MPI_Datatype data_type,
                               int root, MPI_Comm comm)
{
  MPI_Request *req_ptr;
  MPI_Request *reqs;

  int i, rank, num_procs;
  int tag = 1;

  rank = smpi_comm_rank(comm);
  num_procs = smpi_comm_size(comm);

  if (rank != root) {
    smpi_mpi_recv(buff, count, data_type, root, tag, comm, MPI_STATUS_IGNORE);
  }

  else {
    reqs = (MPI_Request *) xbt_malloc((num_procs - 1) * sizeof(MPI_Request));
    req_ptr = reqs;

    // Root sends data to all others
    for (i = 0; i < num_procs; i++) {
      if (i == rank)
        continue;
      *(req_ptr++) = smpi_mpi_isend(buff, count, data_type, i, tag, comm);
    }

    // wait on all requests
    smpi_mpi_waitall(num_procs - 1, reqs, MPI_STATUSES_IGNORE);

    free(reqs);
  }
  return MPI_SUCCESS;
}
