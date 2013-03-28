#include "colls.h"
//#include <star-reduction.c>

int
smpi_coll_tuned_reduce_flat_tree(void *sbuf, void *rbuf, int count,
                                 MPI_Datatype dtype, MPI_Op op,
                                 int root, MPI_Comm comm)
{
  int i, tag = 4321;
  int size;
  int rank;
  MPI_Aint extent;
  char *origin = 0;
  char *inbuf;
  MPI_Status status;

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);

  /* If not root, send data to the root. */
  MPI_Type_extent(dtype, &extent);

  if (rank != root) {
    MPI_Send(sbuf, count, dtype, root, tag, comm);
    return 0;
  }

  /* Root receives and reduces messages.  Allocate buffer to receive
     messages. */

  if (size > 1)
    origin = (char *) malloc(count * extent);


  /* Initialize the receive buffer. */
  if (rank == (size - 1))
    MPI_Sendrecv(sbuf, count, dtype, rank, tag,
                 rbuf, count, dtype, rank, tag, comm, &status);
  else
    MPI_Recv(rbuf, count, dtype, size - 1, tag, comm, &status);

  /* Loop receiving and calling reduction function (C or Fortran). */

  for (i = size - 2; i >= 0; --i) {
    if (rank == i)
      inbuf = sbuf;
    else {
      MPI_Recv(origin, count, dtype, i, tag, comm, &status);
      inbuf = origin;
    }

    /* Call reduction function. */
    star_reduction(op, inbuf, rbuf, &count, &dtype);

  }

  if (origin)
    free(origin);

  /* All done */
  return 0;
}
