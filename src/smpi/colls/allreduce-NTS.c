#include "colls_private.h"
/* IMPLEMENTED BY PITCH PATARASUK 
   Non-topoloty-specific all-reduce operation designed bandwidth optimally */

/* ** NOTE **
   Use -DMPICH2_REDUCTION if this code does not compile.
   MPICH1 code also work on MPICH2 on our cluster and the performance are similar.
   This code assume commutative and associative reduce operator (MPI_SUM, MPI_MAX, etc).
*/

//#include <star-reduction.c>

int
smpi_coll_tuned_allreduce_NTS(void *sbuf, void *rbuf, int rcount,
                              MPI_Datatype dtype, MPI_Op op, MPI_Comm comm)
{
  int tag = 5000;
  MPI_Status status;
  int rank, i, size, count;
  int send_offset, recv_offset;
  int remainder, remainder_flag, remainder_offset;

  rank = smpi_comm_rank(MPI_COMM_WORLD);
  size = smpi_comm_size(MPI_COMM_WORLD);

  /* make it compatible with all data type */
  MPI_Aint extent;
  extent = smpi_datatype_get_extent(dtype);

  /* when communication size is smaller than number of process (not support) */
  if (rcount < size) {
    return mpi_coll_allreduce_fun(sbuf, rbuf, rcount, dtype, op, comm);
  }

  /* when communication size is not divisible by number of process: 
     call the native implementation for the remain chunk at the end of the operation */
  else if (rcount % size != 0) {
    remainder = rcount % size;
    remainder_flag = 1;
    remainder_offset = (rcount / size) * size * extent;
  } else {
    remainder_flag = remainder_offset = 0;
  }

  /* size of each point-to-point communication is equal to the size of the whole message
     divided by number of processes
   */
  count = rcount / size;

  /* our ALL-REDUCE implementation
     1. copy (partial of)send_buf to recv_buf
     2. use logical ring reduce-scatter
     3. use logical ring all-gather 
   */

  // copy partial data
  send_offset = ((rank - 1 + size) % size) * count * extent;
  recv_offset = ((rank - 1 + size) % size) * count * extent;
  smpi_mpi_sendrecv((char *) sbuf + send_offset, count, dtype, rank, tag - 1,
               (char *) rbuf + recv_offset, count, dtype, rank, tag - 1, comm,
               &status);

  // reduce-scatter
  for (i = 0; i < (size - 1); i++) {
    send_offset = ((rank - 1 - i + size) % size) * count * extent;
    recv_offset = ((rank - 2 - i + size) % size) * count * extent;
    smpi_mpi_sendrecv((char *) rbuf + send_offset, count, dtype, ((rank + 1) % size),
                 tag + i, (char *) rbuf + recv_offset, count, dtype,
                 ((rank + size - 1) % size), tag + i, comm, &status);

    // compute result to rbuf+recv_offset
    star_reduction(op, (char *)sbuf + recv_offset, (char *)rbuf + recv_offset, &count, &dtype);
  }

  // all-gather
  for (i = 0; i < (size - 1); i++) {
    send_offset = ((rank - i + size) % size) * count * extent;
    recv_offset = ((rank - 1 - i + size) % size) * count * extent;
    smpi_mpi_sendrecv((char *) rbuf + send_offset, count, dtype, ((rank + 1) % size),
                 tag + i, (char *) rbuf + recv_offset, count, dtype,
                 ((rank + size - 1) % size), tag + i, comm, &status);
  }

  /* when communication size is not divisible by number of process: 
     call the native implementation for the remain chunk at the end of the operation */
  if (remainder_flag) {
    XBT_WARN("MPI_allreduce_NTS use default MPI_allreduce.");
    smpi_mpi_allreduce((char *) sbuf + remainder_offset,
                         (char *) rbuf + remainder_offset, remainder, dtype, op,
                         comm);
    return MPI_SUCCESS;    
  }

  return MPI_SUCCESS;
}
