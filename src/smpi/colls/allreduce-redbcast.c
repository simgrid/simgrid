#include "colls.h"

int smpi_coll_tuned_allreduce_redbcast(void *buf, void *buf2, int count,
                                       MPI_Datatype datatype, MPI_Op op,
                                       MPI_Comm comm)
{
  MPI_Reduce(buf, buf2, count, datatype, op, 0, comm);
  MPI_Bcast(buf2, count, datatype, 0, comm);
  return 0;
}
