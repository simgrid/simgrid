#include "colls_private.h"

int smpi_coll_tuned_allreduce_redbcast(void *buf, void *buf2, int count,
                                       MPI_Datatype datatype, MPI_Op op,
                                       MPI_Comm comm)
{
  mpi_coll_reduce_fun(buf, buf2, count, datatype, op, 0, comm);
  mpi_coll_bcast_fun(buf2, count, datatype, 0, comm);
  return MPI_SUCCESS;
}
