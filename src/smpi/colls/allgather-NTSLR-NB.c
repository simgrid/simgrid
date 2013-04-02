#include "colls.h"

// Allgather-Non-Topoloty-Scecific-Logical-Ring algorithm
int
smpi_coll_tuned_allgather_NTSLR_NB(void *sbuf, int scount, MPI_Datatype stype,
                                   void *rbuf, int rcount, MPI_Datatype rtype,
                                   MPI_Comm comm)
{
  MPI_Aint rextent, sextent;
  MPI_Status status, status2;
  int i, to, from, rank, size;
  int send_offset, recv_offset;
  int tag = 500;

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);
  MPI_Type_extent(rtype, &rextent);
  MPI_Type_extent(stype, &sextent);
  MPI_Request *rrequest_array;
  MPI_Request *srequest_array;
  rrequest_array = (MPI_Request *) malloc(size * sizeof(MPI_Request));
  srequest_array = (MPI_Request *) malloc(size * sizeof(MPI_Request));

  // irregular case use default MPI fucntions
  if (scount * sextent != rcount * rextent)
    MPI_Allgather(sbuf, scount, stype, rbuf, rcount, rtype, comm);

  // topo non-specific
  to = (rank + 1) % size;
  from = (rank + size - 1) % size;

  //copy a single segment from sbuf to rbuf
  send_offset = rank * scount * sextent;

  MPI_Sendrecv(sbuf, scount, stype, rank, tag,
               (char *)rbuf + send_offset, rcount, rtype, rank, tag, comm, &status);


  //start sending logical ring message
  int increment = scount * sextent;

  //post all irecv first
  for (i = 0; i < size - 1; i++) {
    recv_offset = ((rank - i - 1 + size) % size) * increment;
    MPI_Irecv((char *)rbuf + recv_offset, rcount, rtype, from, tag + i, comm,
              &rrequest_array[i]);
  }


  for (i = 0; i < size - 1; i++) {
    send_offset = ((rank - i + size) % size) * increment;
    MPI_Isend((char *)rbuf + send_offset, scount, stype, to, tag + i, comm,
              &srequest_array[i]);
    MPI_Wait(&rrequest_array[i], &status);
    MPI_Wait(&srequest_array[i], &status2);
  }

  free(rrequest_array);
  free(srequest_array);

  return MPI_SUCCESS;
}
