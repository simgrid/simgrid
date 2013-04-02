#include "colls.h"

#ifndef NUM_CORE
#define NUM_CORE 4
#endif

int smpi_coll_tuned_allgather_loosely_lr(void *sbuf, int scount,
                                         MPI_Datatype stype, void *rbuf,
                                         int rcount, MPI_Datatype rtype,
                                         MPI_Comm comm)
{
  int comm_size, rank;
  int tag = 50;
  int i, j, send_offset, recv_offset;
  int intra_rank, inter_rank, inter_comm_size, intra_comm_size;
  int inter_dst, inter_src;

  MPI_Comm_size(comm, &comm_size);
  MPI_Comm_rank(comm, &rank);
  MPI_Aint rextent, sextent;
  MPI_Type_extent(rtype, &rextent);
  MPI_Type_extent(stype, &sextent);
  MPI_Request inter_rrequest;
  MPI_Request rrequest_array[128];
  MPI_Request srequest_array[128];
  MPI_Request inter_srequest_array[128];


  int rrequest_count = 0;
  int srequest_count = 0;
  int inter_srequest_count = 0;

  MPI_Status status;

  intra_rank = rank % NUM_CORE;
  inter_rank = rank / NUM_CORE;
  inter_comm_size = (comm_size + NUM_CORE - 1) / NUM_CORE;
  intra_comm_size = NUM_CORE;

  int src_seg, dst_seg;

  //copy corresponding message from sbuf to rbuf
  recv_offset = rank * rextent * rcount;
  MPI_Sendrecv(sbuf, scount, stype, rank, tag,
               (char *)rbuf + recv_offset, rcount, rtype, rank, tag, comm, &status);

  int dst, src;
  int inter_send_offset, inter_recv_offset;

  rrequest_count = 0;
  srequest_count = 0;
  inter_srequest_count = 0;

  for (i = 0; i < inter_comm_size; i++) {

    // inter_communication

    inter_dst = (rank + intra_comm_size) % comm_size;
    inter_src = (rank - intra_comm_size + comm_size) % comm_size;

    src_seg =
        ((inter_rank - 1 - i +
          inter_comm_size) % inter_comm_size) * intra_comm_size + intra_rank;
    dst_seg =
        ((inter_rank - i +
          inter_comm_size) % inter_comm_size) * intra_comm_size + intra_rank;

    inter_send_offset = dst_seg * sextent * scount;
    inter_recv_offset = src_seg * rextent * rcount;

    for (j = 0; j < intra_comm_size; j++) {

      // inter communication
      if (intra_rank == j) {
        if (i != inter_comm_size - 1) {

          MPI_Irecv((char *)rbuf + inter_recv_offset, rcount, rtype, inter_src, tag,
                    comm, &inter_rrequest);
          MPI_Isend((char *)rbuf + inter_send_offset, scount, stype, inter_dst, tag,
                    comm, &inter_srequest_array[inter_srequest_count++]);

        }
      }
      //intra_communication
      src = inter_rank * intra_comm_size + j;
      dst = inter_rank * intra_comm_size + j;

      src_seg =
          ((inter_rank - i +
            inter_comm_size) % inter_comm_size) * intra_comm_size + j;
      dst_seg =
          ((inter_rank - i +
            inter_comm_size) % inter_comm_size) * intra_comm_size + intra_rank;

      send_offset = dst_seg * sextent * scount;
      recv_offset = src_seg * rextent * rcount;


      if (j != intra_rank) {

        MPI_Irecv((char *)rbuf + recv_offset, rcount, rtype, src, tag, comm,
                  &rrequest_array[rrequest_count++]);
        MPI_Isend((char *)rbuf + send_offset, scount, stype, dst, tag, comm,
                  &srequest_array[srequest_count++]);

      }
    }                           // intra loop


    // wait for inter communication to finish for these rounds (# of round equals NUM_CORE)
    if (i != inter_comm_size - 1) {
      MPI_Wait(&inter_rrequest, &status);
    }

  }                             //inter loop

  MPI_Waitall(rrequest_count, rrequest_array, MPI_STATUSES_IGNORE);
  MPI_Waitall(srequest_count, srequest_array, MPI_STATUSES_IGNORE);
  MPI_Waitall(inter_srequest_count, inter_srequest_array, MPI_STATUSES_IGNORE);

  return MPI_SUCCESS;
}
