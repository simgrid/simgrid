#include "colls_private.h"

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

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  rextent = smpi_datatype_get_extent(rtype);
  sextent = smpi_datatype_get_extent(stype);
  MPI_Request *rrequest_array;
  MPI_Request *srequest_array;
  rrequest_array = (MPI_Request *) xbt_malloc(size * sizeof(MPI_Request));
  srequest_array = (MPI_Request *) xbt_malloc(size * sizeof(MPI_Request));

  // irregular case use default MPI fucntions
  if (scount * sextent != rcount * rextent) {
    XBT_WARN("MPI_allgather_NTSLR_NB use default MPI_allgather.");  
    smpi_mpi_allgather(sbuf, scount, stype, rbuf, rcount, rtype, comm);
    return MPI_SUCCESS;    
  }

  // topo non-specific
  to = (rank + 1) % size;
  from = (rank + size - 1) % size;

  //copy a single segment from sbuf to rbuf
  send_offset = rank * scount * sextent;

  smpi_mpi_sendrecv(sbuf, scount, stype, rank, tag,
               (char *)rbuf + send_offset, rcount, rtype, rank, tag, comm, &status);


  //start sending logical ring message
  int increment = scount * sextent;

  //post all irecv first
  for (i = 0; i < size - 1; i++) {
    recv_offset = ((rank - i - 1 + size) % size) * increment;
    rrequest_array[i] = smpi_mpi_irecv((char *)rbuf + recv_offset, rcount, rtype, from, tag + i, comm);
  }


  for (i = 0; i < size - 1; i++) {
    send_offset = ((rank - i + size) % size) * increment;
    srequest_array[i] = smpi_mpi_isend((char *)rbuf + send_offset, scount, stype, to, tag + i, comm);
    smpi_mpi_wait(&rrequest_array[i], &status);
    smpi_mpi_wait(&srequest_array[i], &status2);
  }

  free(rrequest_array);
  free(srequest_array);

  return MPI_SUCCESS;
}
