#include "colls_private.h"

int smpi_coll_tuned_allreduce_redbcast(void *buf, void *buf2, int count,
                                       MPI_Datatype datatype, MPI_Op op,
                                       MPI_Comm comm)
{
  smpi_mpi_reduce(buf, buf2, count, datatype, op, 0, comm);
  smpi_mpi_bcast(buf2, count, datatype, 0, comm);
  return MPI_SUCCESS;
}
