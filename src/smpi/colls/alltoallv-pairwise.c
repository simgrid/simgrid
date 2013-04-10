#include "colls_private.h"

int smpi_coll_tuned_alltoallv_pairwise(void *sendbuf, int *sendcounts, int *senddisps,
                                      MPI_Datatype sendtype, void *recvbuf,
                                      int *recvcounts, int *recvdisps, MPI_Datatype recvtype,
                                      MPI_Comm comm)
{
  int system_tag = 999;
  int rank, size, step, sendto, recvfrom, sendsize, recvsize;

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  XBT_DEBUG("<%d> algorithm alltoallv_pairwise() called.", rank);
  sendsize = smpi_datatype_size(sendtype);
  recvsize = smpi_datatype_size(recvtype);
  /* Perform pairwise exchange - starting from 1 so the local copy is last */
  for (step = 1; step < size + 1; step++) {
    /* who do we talk to in this step? */
    sendto = (rank + step) % size;
    recvfrom = (rank + size - step) % size;
    /* send and receive */
    smpi_mpi_sendrecv(&((char *) sendbuf)[senddisps[sendto] * sendsize],
                      sendcounts[sendto], sendtype, sendto, system_tag,
                      &((char *) recvbuf)[recvdisps[recvfrom] * recvsize],
                      recvcounts[recvfrom], recvtype, recvfrom, system_tag, comm,
                      MPI_STATUS_IGNORE);
  }
  return MPI_SUCCESS;
}

