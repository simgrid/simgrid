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

      int bblock = 4;//MPIR_PARAM_ALLTOALL_THROTTLE
      //if (bblock == 0) bblock = comm_size;


     // requests = xbt_new(MPI_Request, 2 * (bblock - 1));
      int ii, ss, dst;
      /* post only bblock isends/irecvs at a time as suggested by Tony Ladd */
      for (ii=0; ii<size; ii+=bblock) {
          requests = xbt_new(MPI_Request, 2 * (bblock ));

          ss = size-ii < bblock ? size-ii : bblock;
          count = 0;

          /* do the communication -- post ss sends and receives: */
          for ( i=0; i<ss; i++ ) {
            dst = (rank+i+ii) % size;
              if (dst == rank) {
                XBT_DEBUG("<%d> skip request creation [src = %d, recvcount = %d]",
                       rank, i, recvcounts[dst]);
                continue;
              }

              requests[count]=smpi_mpi_irecv((char *)recvbuf + recvdisps[dst] * recvext, recvcounts[dst],
                                  recvtype, dst, system_tag, comm );
              count++;
            }
            /* Now create all sends  */
          for ( i=0; i<ss; i++ ) {
              dst = (rank-i-ii+size) % size;
              if (dst == rank) {
                XBT_DEBUG("<%d> skip request creation [dst = %d, sendcount = %d]",
                       rank, i, sendcounts[dst]);
                continue;
              }
              requests[count]=smpi_mpi_isend((char *)sendbuf + senddisps[dst] * sendext, sendcounts[dst],
                                  sendtype, dst, system_tag, comm);
              count++;
            }
            /* Wait for them all. */
            //smpi_mpi_startall(count, requests);
            XBT_DEBUG("<%d> wait for %d requests", rank, count);
            smpi_mpi_waitall(count, requests, MPI_STATUSES_IGNORE);
            xbt_free(requests);

          }

  }
  return MPI_SUCCESS;
}
