#include "colls_private.h"
//#include <star-reduction.c>

int smpi_coll_tuned_allreduce_rab_rsag(void *sbuff, void *rbuff, int count,
                                       MPI_Datatype dtype, MPI_Op op,
                                       MPI_Comm comm)
{
  int nprocs, rank, tag = COLL_TAG_ALLREDUCE;
  int mask, dst, pof2, newrank, rem, newdst, i,
      send_idx, recv_idx, last_idx, send_cnt, recv_cnt, *cnts, *disps;
  MPI_Aint extent;
  MPI_Status status;
  void *tmp_buf = NULL;
  nprocs = smpi_comm_size(comm);
  rank = smpi_comm_rank(comm);

  extent = smpi_datatype_get_extent(dtype);
  tmp_buf = (void *) xbt_malloc(count * extent);

  smpi_mpi_sendrecv(sbuff, count, dtype, rank, tag, rbuff, count, dtype, rank, tag,
               comm, &status);

  // find nearest power-of-two less than or equal to comm_size
  pof2 = 1;
  while (pof2 <= nprocs)
    pof2 <<= 1;
  pof2 >>= 1;

  rem = nprocs - pof2;

  // In the non-power-of-two case, all even-numbered
  // processes of rank < 2*rem send their data to
  // (rank+1). These even-numbered processes no longer
  // participate in the algorithm until the very end. The
  // remaining processes form a nice power-of-two. 

  if (rank < 2 * rem) {
    // even       
    if (rank % 2 == 0) {

      smpi_mpi_send(rbuff, count, dtype, rank + 1, tag, comm);

      // temporarily set the rank to -1 so that this
      // process does not pariticipate in recursive
      // doubling
      newrank = -1;
    } else                      // odd
    {
      smpi_mpi_recv(tmp_buf, count, dtype, rank - 1, tag, comm, &status);
      // do the reduction on received data. since the
      // ordering is right, it doesn't matter whether
      // the operation is commutative or not.
      smpi_op_apply(op, tmp_buf, rbuff, &count, &dtype);

      // change the rank 
      newrank = rank / 2;
    }
  }

  else                          // rank >= 2 * rem 
    newrank = rank - rem;

  // If op is user-defined or count is less than pof2, use
  // recursive doubling algorithm. Otherwise do a reduce-scatter
  // followed by allgather. (If op is user-defined,
  // derived datatypes are allowed and the user could pass basic
  // datatypes on one process and derived on another as long as
  // the type maps are the same. Breaking up derived
  // datatypes to do the reduce-scatter is tricky, therefore
  // using recursive doubling in that case.) 

  if (newrank != -1) {
    // do a reduce-scatter followed by allgather. for the
    // reduce-scatter, calculate the count that each process receives
    // and the displacement within the buffer 

    cnts = (int *) xbt_malloc(pof2 * sizeof(int));
    disps = (int *) xbt_malloc(pof2 * sizeof(int));

    for (i = 0; i < (pof2 - 1); i++)
      cnts[i] = count / pof2;
    cnts[pof2 - 1] = count - (count / pof2) * (pof2 - 1);

    disps[0] = 0;
    for (i = 1; i < pof2; i++)
      disps[i] = disps[i - 1] + cnts[i - 1];

    mask = 0x1;
    send_idx = recv_idx = 0;
    last_idx = pof2;
    while (mask < pof2) {
      newdst = newrank ^ mask;
      // find real rank of dest 
      dst = (newdst < rem) ? newdst * 2 + 1 : newdst + rem;

      send_cnt = recv_cnt = 0;
      if (newrank < newdst) {
        send_idx = recv_idx + pof2 / (mask * 2);
        for (i = send_idx; i < last_idx; i++)
          send_cnt += cnts[i];
        for (i = recv_idx; i < send_idx; i++)
          recv_cnt += cnts[i];
      } else {
        recv_idx = send_idx + pof2 / (mask * 2);
        for (i = send_idx; i < recv_idx; i++)
          send_cnt += cnts[i];
        for (i = recv_idx; i < last_idx; i++)
          recv_cnt += cnts[i];
      }

      // Send data from recvbuf. Recv into tmp_buf 
      smpi_mpi_sendrecv((char *) rbuff + disps[send_idx] * extent, send_cnt,
                   dtype, dst, tag,
                   (char *) tmp_buf + disps[recv_idx] * extent, recv_cnt,
                   dtype, dst, tag, comm, &status);

      // tmp_buf contains data received in this step.
      // recvbuf contains data accumulated so far 

      // This algorithm is used only for predefined ops
      // and predefined ops are always commutative.
      smpi_op_apply(op, (char *) tmp_buf + disps[recv_idx] * extent,
                     (char *) rbuff + disps[recv_idx] * extent,
                     &recv_cnt, &dtype);

      // update send_idx for next iteration 
      send_idx = recv_idx;
      mask <<= 1;

      // update last_idx, but not in last iteration because the value
      // is needed in the allgather step below. 
      if (mask < pof2)
        last_idx = recv_idx + pof2 / mask;
    }

    // now do the allgather 

    mask >>= 1;
    while (mask > 0) {
      newdst = newrank ^ mask;
      // find real rank of dest
      dst = (newdst < rem) ? newdst * 2 + 1 : newdst + rem;

      send_cnt = recv_cnt = 0;
      if (newrank < newdst) {
        // update last_idx except on first iteration 
        if (mask != pof2 / 2)
          last_idx = last_idx + pof2 / (mask * 2);

        recv_idx = send_idx + pof2 / (mask * 2);
        for (i = send_idx; i < recv_idx; i++)
          send_cnt += cnts[i];
        for (i = recv_idx; i < last_idx; i++)
          recv_cnt += cnts[i];
      } else {
        recv_idx = send_idx - pof2 / (mask * 2);
        for (i = send_idx; i < last_idx; i++)
          send_cnt += cnts[i];
        for (i = recv_idx; i < send_idx; i++)
          recv_cnt += cnts[i];
      }

      smpi_mpi_sendrecv((char *) rbuff + disps[send_idx] * extent, send_cnt,
                   dtype, dst, tag,
                   (char *) rbuff + disps[recv_idx] * extent, recv_cnt,
                   dtype, dst, tag, comm, &status);

      if (newrank > newdst)
        send_idx = recv_idx;

      mask >>= 1;
    }

    free(cnts);
    free(disps);

  }
  // In the non-power-of-two case, all odd-numbered processes of
  // rank < 2 * rem send the result to (rank-1), the ranks who didn't
  // participate above.

  if (rank < 2 * rem) {
    if (rank % 2)               // odd 
      smpi_mpi_send(rbuff, count, dtype, rank - 1, tag, comm);
    else                        // even 
      smpi_mpi_recv(rbuff, count, dtype, rank + 1, tag, comm, &status);
  }

  free(tmp_buf);
  return MPI_SUCCESS;
}
