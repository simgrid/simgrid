#include "colls.h"
//#include <star-reduction.c>

// NP pow of 2 for now
int smpi_coll_tuned_allreduce_rab1(void *sbuff, void *rbuff,
                                   int count, MPI_Datatype dtype,
                                   MPI_Op op, MPI_Comm comm)
{
  MPI_Status status;
  MPI_Aint extent;
  int tag = 4321, rank, nprocs, send_size, newcnt, share;
  int pof2 = 1, mask, send_idx, recv_idx, dst, send_cnt, recv_cnt;

  void *recv, *tmp_buf;

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &nprocs);

  MPI_Type_extent(dtype, &extent);

  pof2 = 1;
  while (pof2 <= nprocs)
    pof2 <<= 1;
  pof2 >>= 1;

  mask = 1;
  send_idx = recv_idx = 0;

  // uneven count
  if ((count % nprocs)) {
    send_size = (count + nprocs) / nprocs;
    newcnt = send_size * nprocs;

    recv = (void *) malloc(extent * newcnt);
    tmp_buf = (void *) malloc(extent * newcnt);
    memcpy(recv, sbuff, extent * count);


    mask = pof2 / 2;
    share = newcnt / pof2;
    while (mask > 0) {
      dst = rank ^ mask;
      send_cnt = recv_cnt = newcnt / (pof2 / mask);

      if (rank < dst)
        send_idx = recv_idx + (mask * share);
      else
        recv_idx = send_idx + (mask * share);

      MPI_Sendrecv((char *) recv + send_idx * extent, send_cnt, dtype, dst, tag,
                   tmp_buf, recv_cnt, dtype, dst, tag, comm, &status);

      star_reduction(op, tmp_buf, (char *) recv + recv_idx * extent, &recv_cnt,
                     &dtype);

      // update send_idx for next iteration 
      send_idx = recv_idx;
      mask >>= 1;
    }

    memcpy(tmp_buf, (char *) recv + recv_idx * extent, recv_cnt * extent);
    MPI_Allgather(tmp_buf, recv_cnt, dtype, recv, recv_cnt, dtype, comm);

    memcpy(rbuff, recv, count * extent);
    free(recv);
    free(tmp_buf);

  }

  else {
    tmp_buf = (void *) malloc(extent * count);
    memcpy(rbuff, sbuff, count * extent);
    mask = pof2 / 2;
    share = count / pof2;
    while (mask > 0) {
      dst = rank ^ mask;
      send_cnt = recv_cnt = count / (pof2 / mask);

      if (rank < dst)
        send_idx = recv_idx + (mask * share);
      else
        recv_idx = send_idx + (mask * share);

      MPI_Sendrecv((char *) rbuff + send_idx * extent, send_cnt, dtype, dst,
                   tag, tmp_buf, recv_cnt, dtype, dst, tag, comm, &status);

      star_reduction(op, tmp_buf, (char *) rbuff + recv_idx * extent, &recv_cnt,
                     &dtype);

      // update send_idx for next iteration 
      send_idx = recv_idx;
      mask >>= 1;
    }

    memcpy(tmp_buf, (char *) rbuff + recv_idx * extent, recv_cnt * extent);
    MPI_Allgather(tmp_buf, recv_cnt, dtype, rbuff, recv_cnt, dtype, comm);
    free(tmp_buf);
  }

  return MPI_SUCCESS;
}
