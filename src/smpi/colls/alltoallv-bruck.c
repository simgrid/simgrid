#include "colls_private.h"

/**
 * Alltoall Bruck
 *
 * Openmpi calls this routine when the message size sent to each rank < 2000 bytes and size < 12
 * FIXME: uh, check smpi_pmpi again, but this routine is called for > 12, not
 * less...
 **/
int smpi_coll_tuned_alltoallv_bruck(void *sendbuf, int *sendcounts, int *senddisps,
                                   MPI_Datatype sendtype, void *recvbuf,
                                   int *recvcounts, int *recvdisps, MPI_Datatype recvtype,
                                   MPI_Comm comm)
{
  int system_tag = COLL_TAG_ALLTOALLV;
  int i, rank, size, err, count;
  MPI_Aint lb;
  MPI_Aint sendext = 0;
  MPI_Aint recvext = 0;
  MPI_Request *requests;

  // FIXME: check implementation
  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  XBT_DEBUG("<%d> algorithm alltoall_bruck() called.", rank);

  err = smpi_datatype_extent(sendtype, &lb, &sendext);
  err = smpi_datatype_extent(recvtype, &lb, &recvext);
  /* Local copy from self */
  err =
      smpi_datatype_copy((char *)sendbuf + senddisps[rank] * sendext, 
                         sendcounts[rank], sendtype, 
                         (char *)recvbuf + recvdisps[rank] * recvext,
                         recvcounts[rank], recvtype);
  if (err == MPI_SUCCESS && size > 1) {
    /* Initiate all send/recv to/from others. */
    requests = xbt_new(MPI_Request, 2 * (size - 1));
    count = 0;
    /* Create all receives that will be posted first */
    for (i = 0; i < size; ++i) {
      if (i == rank) {
        XBT_DEBUG("<%d> skip request creation [src = %d, recvcount = %d]",
               rank, i, recvcounts[i]);
        continue;
      }
      requests[count] =
          smpi_irecv_init((char *)recvbuf + recvdisps[i] * recvext, recvcounts[i],
                          recvtype, i, system_tag, comm);
      count++;
    }
    /* Now create all sends  */
    for (i = 0; i < size; ++i) {
      if (i == rank) {
        XBT_DEBUG("<%d> skip request creation [dst = %d, sendcount = %d]",
               rank, i, sendcounts[i]);
        continue;
      }
      requests[count] =
          smpi_isend_init((char *)sendbuf + senddisps[i] * sendext, sendcounts[i],
                          sendtype, i, system_tag, comm);
      count++;
    }
    /* Wait for them all. */
    smpi_mpi_startall(count, requests);
    XBT_DEBUG("<%d> wait for %d requests", rank, count);
    smpi_mpi_waitall(count, requests, MPI_STATUS_IGNORE);
    xbt_free(requests);
  }
  return MPI_SUCCESS;
}
