#include "colls.h"

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
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &comm_size);

  MPI_Type_extent(datatype, &extent);

  tmp_buf = (void *) malloc(count * extent);

  MPI_Sendrecv(sendbuf, count, datatype, rank, tag,
               recvbuf, count, datatype, rank, tag, comm, &status);
  mask = 1;
  relrank = (rank - root + comm_size) % comm_size;

  while (mask < comm_size) {
    /* Receive */
    if ((mask & relrank) == 0) {
      source = (relrank | mask);
      if (source < comm_size) {
        source = (source + root) % comm_size;
        MPI_Recv(tmp_buf, count, datatype, source, tag, comm, &status);
        star_reduction(op, tmp_buf, recvbuf, &count, &datatype);
      }
    } else {
      dst = ((relrank & (~mask)) + root) % comm_size;
      MPI_Send(recvbuf, count, datatype, dst, tag, comm);
      break;
    }
    mask <<= 1;
  }

  free(tmp_buf);

  return 0;
}
