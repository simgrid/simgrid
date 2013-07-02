#include "colls_private.h"

/*****************************************************************************

 * Function: alltoall_spreading_simple

 * Return: int

 *  Inputs:
    send_buff: send input buffer
    send_count: number of elements to send
    send_type: data type of elements being sent
    recv_buff: receive output buffer
    recv_count: number of elements to received
    recv_type: data type of elements being received
    comm: communicator

 * Descrp: Let i -> j denote the communication from node i to node j. The
          order of communications for node i is i -> i + 1, i -> i + 2, ...,
          i -> (i + p -1) % P.
 
 * Auther: Ahmad Faraj

 ****************************************************************************/
int smpi_coll_tuned_alltoall_simple(void *send_buff, int send_count,
                                    MPI_Datatype send_type,
                                    void *recv_buff, int recv_count,
                                    MPI_Datatype recv_type, MPI_Comm comm)
{
  int i, rank, size, nreqs, src, dst, tag = COLL_TAG_ALLTOALL;
  char *psnd;
  char *prcv;
  MPI_Aint sndinc;
  MPI_Aint rcvinc;
  MPI_Request *req;
  MPI_Request *preq;
  MPI_Request *qreq;
  MPI_Status s, *statuses;


  size = smpi_comm_size(comm);
  rank = smpi_comm_rank(comm);
  sndinc = smpi_datatype_get_extent(send_type);
  rcvinc = smpi_datatype_get_extent(recv_type);
  sndinc *= send_count;
  rcvinc *= recv_count;

  /* Allocate arrays of requests. */

  nreqs = 2 * (size - 1);
  if (nreqs > 0) {
    req = (MPI_Request *) xbt_malloc(nreqs * sizeof(MPI_Request));
    statuses = (MPI_Status *) xbt_malloc(nreqs * sizeof(MPI_Status));
  } else {
    req = NULL;
    statuses = NULL;
  }

  /* simple optimization */

  psnd = ((char *) send_buff) + (rank * sndinc);
  prcv = ((char *) recv_buff) + (rank * rcvinc);
  smpi_mpi_sendrecv(psnd, send_count, send_type, rank, tag,
               prcv, recv_count, recv_type, rank, tag, comm, &s);


  /* Initiate all send/recv to/from others. */

  preq = req;
  qreq = req + size - 1;
  prcv = (char *) recv_buff;
  psnd = (char *) send_buff;
  for (i = 0; i < size; i++) {
    src = dst = (rank + i) % size;
    if (src == rank)
      continue;
    if (dst == rank)
      continue;
    *(preq++) = smpi_mpi_recv_init(prcv + (src * rcvinc), recv_count, recv_type, src,
                  tag, comm);
    *(qreq++) = smpi_mpi_send_init(psnd + (dst * sndinc), send_count, send_type, dst,
                  tag, comm);
  }

  /* Start all the requests. */

  smpi_mpi_startall(nreqs, req);

  /* Wait for them all. */

  smpi_mpi_waitall(nreqs, req, statuses);


  /* All done */

  if (req)
    xbt_free((char *) req);
  if (statuses)
    xbt_free(statuses);
  return MPI_SUCCESS;
}
