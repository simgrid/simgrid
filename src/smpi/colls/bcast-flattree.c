#include "colls.h"

int
smpi_coll_tuned_bcast_flattree(void *buff, int count, MPI_Datatype data_type,
                               int root, MPI_Comm comm)
{
  MPI_Request *req_ptr;
  MPI_Request *reqs;

  int i, rank, num_procs;
  int tag = 1;

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &num_procs);

  if (rank != root) {
    MPI_Recv(buff, count, data_type, root, tag, comm, MPI_STATUS_IGNORE);
  }

  else {
    reqs = (MPI_Request *) malloc((num_procs - 1) * sizeof(MPI_Request));
    req_ptr = reqs;

    // Root sends data to all others
    for (i = 0; i < num_procs; i++) {
      if (i == rank)
        continue;
      MPI_Isend(buff, count, data_type, i, tag, comm, req_ptr++);
    }

    // wait on all requests
    MPI_Waitall(num_procs - 1, reqs, MPI_STATUSES_IGNORE);

    free(reqs);
  }
  return MPI_SUCCESS;
}
