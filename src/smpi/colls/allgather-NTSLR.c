#include "colls.h"

// Allgather-Non-Topoloty-Scecific-Logical-Ring algorithm
int
smpi_coll_tuned_allgather_NTSLR(void *sbuf, int scount, MPI_Datatype stype,
                                void *rbuf, int rcount, MPI_Datatype rtype,
                                MPI_Comm comm)
{
  MPI_Aint rextent, sextent;
  MPI_Status status;
  int i, to, from, rank, size;
  int send_offset, recv_offset;
  int tag = 500;

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);
  MPI_Type_extent(rtype, &rextent);
  MPI_Type_extent(stype, &sextent);

  // irregular case use default MPI fucntions
  if (scount * sextent != rcount * rextent)
    MPI_Allgather(sbuf, scount, stype, rbuf, rcount, rtype, comm);

  // topo non-specific
  to = (rank + 1) % size;
  from = (rank + size - 1) % size;

  //copy a single segment from sbuf to rbuf
  send_offset = rank * scount * sextent;

  MPI_Sendrecv(sbuf, scount, stype, rank, tag,
               (char *)rbuf + send_offset, rcount, rtype, rank, tag,
               comm, &status);


  //start sending logical ring message
  int increment = scount * sextent;
  for (i = 0; i < size - 1; i++) {
    send_offset = ((rank - i + size) % size) * increment;
    recv_offset = ((rank - i - 1 + size) % size) * increment;
    MPI_Sendrecv((char *) rbuf + send_offset, scount, stype, to, tag + i,
                 (char *) rbuf + recv_offset, rcount, rtype, from, tag + i,
                 comm, &status);
  }

  return MPI_SUCCESS;
}
