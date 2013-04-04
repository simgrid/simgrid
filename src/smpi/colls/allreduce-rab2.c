#include "colls.h"
//#include <star-reduction.c>

// this requires that count >= NP
int smpi_coll_tuned_allreduce_rab2(void *sbuff, void *rbuff,
                                   int count, MPI_Datatype dtype,
                                   MPI_Op op, MPI_Comm comm)
{
  MPI_Aint s_extent;
  int i, rank, nprocs;
  int nbytes, send_size, s_offset, r_offset;
  void *recv, *send, *tmp;
  /*
     #ifdef MPICH2_REDUCTION
     MPI_User_function * uop = MPIR_Op_table[op % 16 - 1];
     #else
     MPI_User_function *uop;
     struct MPIR_OP *op_ptr;
     op_ptr = MPIR_ToPointer(op);
     uop  = op_ptr->op;
     #endif
   */
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &nprocs);


  MPI_Type_extent(dtype, &s_extent);

  // uneven count
  if (count % nprocs) {
    if (count < nprocs)
      send_size = nprocs;
    else
      send_size = (count + nprocs) / nprocs;
    nbytes = send_size * s_extent;

    send = (void *) malloc(s_extent * send_size * nprocs);
    recv = (void *) malloc(s_extent * send_size * nprocs);
    tmp = (void *) malloc(nbytes);

    memcpy(send, sbuff, s_extent * count);

    MPI_Alltoall(send, send_size, dtype, recv, send_size, dtype, comm);

    memcpy(tmp, recv, nbytes);

    for (i = 1, s_offset = nbytes; i < nprocs; i++, s_offset = i * nbytes)
      star_reduction(op, (char *) recv + s_offset, tmp, &send_size, &dtype);

    MPI_Allgather(tmp, send_size, dtype, recv, send_size, dtype, comm);
    memcpy(rbuff, recv, count * s_extent);

    free(recv);
    free(tmp);
    free(send);
  } else {
    send = sbuff;
    send_size = count / nprocs;
    nbytes = send_size * s_extent;
    r_offset = rank * nbytes;

    recv = (void *) malloc(s_extent * send_size * nprocs);

    MPI_Alltoall(send, send_size, dtype, recv, send_size, dtype, comm);

    memcpy((char *) rbuff + r_offset, recv, nbytes);

    for (i = 1, s_offset = nbytes; i < nprocs; i++, s_offset = i * nbytes)
      star_reduction(op, (char *) recv + s_offset, (char *) rbuff + r_offset,
                     &send_size, &dtype);

    MPI_Allgather((char *) rbuff + r_offset, send_size, dtype, rbuff, send_size,
                  dtype, comm);
    free(recv);
  }

  return MPI_SUCCESS;
}
