#include "colls_private.h"

/*****************************************************************************

Copyright (c) 2006, Ahmad Faraj & Xin Yuan,
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

  * Neither the name of the Florida State University nor the names of its
    contributors may be used to endorse or promote products derived from this
    software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  *************************************************************************
  *     Any results obtained from executing this software require the     *
  *     acknowledgment and citation of the software and its owners.       *
  *     The full citation is given below:                                 *
  *                                                                       *
  *     A. Faraj and X. Yuan. "Automatic Generation and Tuning of MPI     *
  *     Collective Communication Routines." The 19th ACM International    *
  *     Conference on Supercomputing (ICS), Cambridge, Massachusetts,     *
  *     June 20-22, 2005.                                                 *
  *************************************************************************

*****************************************************************************/

/*****************************************************************************
 * Function: allgather_spreading_simple
 * return: int
 *  inputs:
 *   send_buff: send input buffer
 *   send_count: number of elements to send
 *   send_type: data type of elements being sent
 *   recv_buff: receive output buffer
 *   recv_count: number of elements to received
 *   recv_type: data type of elements being received
 *   comm: communication
 * Descrp: Let i -> j denote the communication from node i to node j. The
 *         order of communications for node i is i -> i + 1, i -> i + 2, ...,
 *         i -> (i + p -1) % P.
 *
 * Auther: Ahmad Faraj
 ****************************************************************************/
int
smpi_coll_tuned_allgather_spreading_simple(void *send_buff, int send_count,
                                           MPI_Datatype send_type,
                                           void *recv_buff, int recv_count,
                                           MPI_Datatype recv_type,
                                           MPI_Comm comm)
{
  MPI_Request *reqs, *req_ptr;
  MPI_Aint extent;
  int i, src, dst, rank, num_procs, num_reqs;
  int tag = COLL_TAG_ALLGATHER;
  MPI_Status status;
  char *recv_ptr = (char *) recv_buff;

  rank = smpi_comm_rank(comm);
  num_procs = smpi_comm_size(comm);
  extent = smpi_datatype_get_extent(send_type);

  num_reqs = (2 * num_procs) - 2;
  reqs = (MPI_Request *) xbt_malloc(num_reqs * sizeof(MPI_Request));
  if (!reqs) {
    printf("allgather-spreading-simple.c:40: cannot allocate memory\n");
    MPI_Finalize();
    exit(0);
  }

  req_ptr = reqs;
  smpi_mpi_sendrecv(send_buff, send_count, send_type, rank, tag,
               (char *) recv_buff + rank * recv_count * extent, recv_count,
               recv_type, rank, tag, comm, &status);

  for (i = 0; i < num_procs; i++) {
    src = (rank + i) % num_procs;
    if (src == rank)
      continue;
    *(req_ptr++) = smpi_mpi_irecv(recv_ptr + src * recv_count * extent, recv_count, recv_type,
              src, tag, comm);
  }

  for (i = 0; i < num_procs; i++) {
    dst = (rank + i) % num_procs;
    if (dst == rank)
      continue;
    *(req_ptr++) = smpi_mpi_isend(send_buff, send_count, send_type, dst, tag, comm);
  }

  smpi_mpi_waitall(num_reqs, reqs, MPI_STATUSES_IGNORE);
  free(reqs);

  return MPI_SUCCESS;
}
