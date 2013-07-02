#include "colls_private.h"

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
  int tag = COLL_TAG_ALLGATHER;

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  rextent = smpi_datatype_get_extent(rtype);
  sextent = smpi_datatype_get_extent(stype);

  // irregular case use default MPI fucntions
  if (scount * sextent != rcount * rextent) {
    XBT_WARN("MPI_allgather_NTSLR use default MPI_allgather.");  
    smpi_mpi_allgather(sbuf, scount, stype, rbuf, rcount, rtype, comm);
    return MPI_SUCCESS;    
  }

  // topo non-specific
  to = (rank + 1) % size;
  from = (rank + size - 1) % size;

  //copy a single segment from sbuf to rbuf
  send_offset = rank * scount * sextent;

  smpi_mpi_sendrecv(sbuf, scount, stype, rank, tag,
               (char *)rbuf + send_offset, rcount, rtype, rank, tag,
               comm, &status);


  //start sending logical ring message
  int increment = scount * sextent;
  for (i = 0; i < size - 1; i++) {
    send_offset = ((rank - i + size) % size) * increment;
    recv_offset = ((rank - i - 1 + size) % size) * increment;
    smpi_mpi_sendrecv((char *) rbuf + send_offset, scount, stype, to, tag + i,
                 (char *) rbuf + recv_offset, rcount, rtype, from, tag + i,
                 comm, &status);
  }

  return MPI_SUCCESS;
}
