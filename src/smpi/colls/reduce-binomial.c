#include "colls_private.h"

//#include <star-reduction.c>

int smpi_coll_tuned_reduce_binomial(void *sendbuf, void *recvbuf, int count,
                                    MPI_Datatype datatype, MPI_Op op, int root,
                                    MPI_Comm comm)
{
  MPI_Status status;
  int comm_size, rank;
  int mask, relrank, source;
  int dst;
  int tag = 4321;
  MPI_Aint extent;
  void *tmp_buf;

  if (count == 0)
    return 0;
  rank = smpi_comm_rank(comm);
  comm_size = smpi_comm_size(comm);

  extent = smpi_datatype_get_extent(datatype);

  tmp_buf = (void *) xbt_malloc(count * extent);

  smpi_mpi_sendrecv(sendbuf, count, datatype, rank, tag,
               recvbuf, count, datatype, rank, tag, comm, &status);
  mask = 1;
  relrank = (rank - root + comm_size) % comm_size;

  while (mask < comm_size) {
    /* Receive */
    if ((mask & relrank) == 0) {
      source = (relrank | mask);
      if (source < comm_size) {
        source = (source + root) % comm_size;
        smpi_mpi_recv(tmp_buf, count, datatype, source, tag, comm, &status);
        star_reduction(op, tmp_buf, recvbuf, &count, &datatype);
      }
    } else {
      dst = ((relrank & (~mask)) + root) % comm_size;
      smpi_mpi_send(recvbuf, count, datatype, dst, tag, comm);
      break;
    }
    mask <<= 1;
  }

  free(tmp_buf);

  return 0;
}
