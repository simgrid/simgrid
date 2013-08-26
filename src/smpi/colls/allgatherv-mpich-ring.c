#include "colls_private.h"

/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*****************************************************************************
 * Function: allgather_mpich_ring
 * return: int
 * inputs:
 *   send_buff: send input buffer
 *   send_count: number of elements to send
 *   send_type: data type of elements being sent
 *   recv_buff: receive output buffer
 *   recv_count: number of elements to received
 *   recv_type: data type of elements being received
 *   comm: communication
 ****************************************************************************/
int
smpi_coll_tuned_allgatherv_mpich_ring(void *sendbuf, int sendcount,
    MPI_Datatype send_type, void *recvbuf,
    int *recvcounts, int *displs, MPI_Datatype recvtype,
    MPI_Comm comm)
{

  char * sbuf = NULL, * rbuf = NULL;
  int soffset, roffset;
  int torecv=0, tosend=0, min, rank, comm_size;
  int sendnow, recvnow;
  int sidx, ridx;
  MPI_Status status;
  MPI_Aint recvtype_extent;
  int right, left, total_count, i;
  rank= smpi_comm_rank(comm);
  comm_size=smpi_comm_size(comm);

  recvtype_extent= smpi_datatype_get_extent( recvtype);
  total_count = 0;
  for (i=0; i<comm_size; i++)
    total_count += recvcounts[i];

  if (sendbuf != MPI_IN_PLACE) {
      /* First, load the "local" version in the recvbuf. */
      smpi_datatype_copy(sendbuf, sendcount, send_type,
          ((char *)recvbuf + displs[rank]*recvtype_extent),
          recvcounts[rank], recvtype);
  }

  left  = (comm_size + rank - 1) % comm_size;
  right = (rank + 1) % comm_size;

  torecv = total_count - recvcounts[rank];
  tosend = total_count - recvcounts[right];

  min = recvcounts[0];
  for (i = 1; i < comm_size; i++)
    if (min > recvcounts[i])
      min = recvcounts[i];
  if (min * recvtype_extent < 32768)
    min = 32768 / recvtype_extent;
  /* Handle the case where the datatype extent is larger than
   * the pipeline size. */
  if (!min)
    min = 1;

  sidx = rank;
  ridx = left;
  soffset = 0;
  roffset = 0;
  while (tosend || torecv) { /* While we have data to send or receive */
      sendnow = ((recvcounts[sidx] - soffset) > min) ? min : (recvcounts[sidx] - soffset);
      recvnow = ((recvcounts[ridx] - roffset) > min) ? min : (recvcounts[ridx] - roffset);
      sbuf = (char *)recvbuf + ((displs[sidx] + soffset) * recvtype_extent);
      rbuf = (char *)recvbuf + ((displs[ridx] + roffset) * recvtype_extent);

      /* Protect against wrap-around of indices */
      if (!tosend)
        sendnow = 0;
      if (!torecv)
        recvnow = 0;

      /* Communicate */
      if (!sendnow && !recvnow) {
          /* Don't do anything. This case is possible if two
           * consecutive processes contribute 0 bytes each. */
      }
      else if (!sendnow) { /* If there's no data to send, just do a recv call */
          smpi_mpi_recv(rbuf, recvnow, recvtype, left, COLL_TAG_ALLGATHERV, comm, &status);

          torecv -= recvnow;
      }
      else if (!recvnow) { /* If there's no data to receive, just do a send call */
          smpi_mpi_send(sbuf, sendnow, recvtype, right, COLL_TAG_ALLGATHERV, comm);

          tosend -= sendnow;
      }
      else { /* There's data to be sent and received */
          smpi_mpi_sendrecv(sbuf, sendnow, recvtype, right, COLL_TAG_ALLGATHERV,
              rbuf, recvnow, recvtype, left, COLL_TAG_ALLGATHERV,
              comm, &status);
          tosend -= sendnow;
          torecv -= recvnow;
      }

      soffset += sendnow;
      roffset += recvnow;
      if (soffset == recvcounts[sidx]) {
          soffset = 0;
          sidx = (sidx + comm_size - 1) % comm_size;
      }
      if (roffset == recvcounts[ridx]) {
          roffset = 0;
          ridx = (ridx + comm_size - 1) % comm_size;
      }
  }

  return MPI_SUCCESS;
}
