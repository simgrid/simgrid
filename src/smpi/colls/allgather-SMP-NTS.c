#include "colls.h"
#ifndef NUM_CORE
#define NUM_CORE 8
#endif

int smpi_coll_tuned_allgather_SMP_NTS(void *sbuf, int scount,
                                      MPI_Datatype stype, void *rbuf,
                                      int rcount, MPI_Datatype rtype,
                                      MPI_Comm comm)
{
  int src, dst, comm_size, rank;
  MPI_Comm_size(comm, &comm_size);
  MPI_Comm_rank(comm, &rank);
  MPI_Aint rextent, sextent;
  MPI_Type_extent(rtype, &rextent);
  MPI_Type_extent(stype, &sextent);
  int tag = 50;
  MPI_Request request;
  MPI_Request rrequest_array[128];
  MPI_Request srequest_array[128];

  MPI_Status status;
  int i, send_offset, recv_offset;
  int intra_rank, inter_rank;
  intra_rank = rank % NUM_CORE;
  inter_rank = rank / NUM_CORE;
  int inter_comm_size = (comm_size + NUM_CORE - 1) / NUM_CORE;
  int num_core_in_current_smp = NUM_CORE;

  /* for too small number of processes, use default implementation */
  if (comm_size <= NUM_CORE) {
    return MPI_Allgather(sbuf, scount, stype, rbuf, rcount, rtype, comm);
  }
  // the last SMP node may have fewer number of running processes than all others
  if (inter_rank == (inter_comm_size - 1)) {
    num_core_in_current_smp = comm_size - (inter_rank * NUM_CORE);
  }
  //copy corresponding message from sbuf to rbuf
  recv_offset = rank * rextent * rcount;
  MPI_Sendrecv(sbuf, scount, stype, rank, tag,
               ((char *) rbuf + recv_offset), rcount, rtype, rank, tag, comm,
               &status);

  //gather to root of each SMP

  for (i = 1; i < num_core_in_current_smp; i++) {

    dst =
        (inter_rank * NUM_CORE) + (intra_rank + i) % (num_core_in_current_smp);
    src =
        (inter_rank * NUM_CORE) + (intra_rank - i +
                                   num_core_in_current_smp) %
        (num_core_in_current_smp);
    recv_offset = src * rextent * rcount;

    MPI_Sendrecv(sbuf, scount, stype, dst, tag,
                 ((char *) rbuf + recv_offset), rcount, rtype, src, tag, comm,
                 &status);

  }

  // INTER-SMP-ALLGATHER 
  // Every root of each SMP node post INTER-Sendrecv, then do INTRA-Bcast for each receiving message
  // Use logical ring algorithm

  // root of each SMP
  if (intra_rank == 0) {
    src = ((inter_rank - 1 + inter_comm_size) % inter_comm_size) * NUM_CORE;
    dst = ((inter_rank + 1) % inter_comm_size) * NUM_CORE;

    // post all inter Irecv
    for (i = 0; i < inter_comm_size - 1; i++) {
      recv_offset =
          ((inter_rank - i - 1 +
            inter_comm_size) % inter_comm_size) * NUM_CORE * sextent * scount;
      MPI_Irecv((char *) rbuf + recv_offset, rcount * NUM_CORE, rtype, src,
                tag + i, comm, &rrequest_array[i]);
    }

    // send first message
    send_offset =
        ((inter_rank +
          inter_comm_size) % inter_comm_size) * NUM_CORE * sextent * scount;
    MPI_Isend((char *) rbuf + send_offset, scount * NUM_CORE, stype, dst, tag,
              comm, &srequest_array[0]);

    // loop : recv-inter , send-inter, send-intra (linear-bcast)
    for (i = 0; i < inter_comm_size - 2; i++) {
      recv_offset =
          ((inter_rank - i - 1 +
            inter_comm_size) % inter_comm_size) * NUM_CORE * sextent * scount;
      MPI_Wait(&rrequest_array[i], &status);
      MPI_Isend((char *) rbuf + recv_offset, scount * NUM_CORE, stype, dst,
                tag + i + 1, comm, &srequest_array[i + 1]);
      if (num_core_in_current_smp > 1) {
        MPI_Isend((char *) rbuf + recv_offset, scount * NUM_CORE, stype,
                  (rank + 1), tag + i + 1, comm, &request);
      }
    }

    // recv last message and send_intra
    recv_offset =
        ((inter_rank - i - 1 +
          inter_comm_size) % inter_comm_size) * NUM_CORE * sextent * scount;
    //recv_offset = ((inter_rank + 1) % inter_comm_size) * NUM_CORE * sextent * scount;
    //i=inter_comm_size-2;
    MPI_Wait(&rrequest_array[i], &status);
    if (num_core_in_current_smp > 1) {
      MPI_Isend((char *) rbuf + recv_offset, scount * NUM_CORE, stype,
                (rank + 1), tag + i + 1, comm, &request);
    }
  }
  // last rank of each SMP
  else if (intra_rank == (num_core_in_current_smp - 1)) {
    for (i = 0; i < inter_comm_size - 1; i++) {
      recv_offset =
          ((inter_rank - i - 1 +
            inter_comm_size) % inter_comm_size) * NUM_CORE * sextent * scount;
      MPI_Irecv((char *) rbuf + recv_offset, (rcount * NUM_CORE), rtype,
                rank - 1, tag + i + 1, comm, &request);
      MPI_Wait(&request, &status);
    }
  }
  // intermediate rank of each SMP
  else {
    for (i = 0; i < inter_comm_size - 1; i++) {
      recv_offset =
          ((inter_rank - i - 1 +
            inter_comm_size) % inter_comm_size) * NUM_CORE * sextent * scount;
      MPI_Irecv((char *) rbuf + recv_offset, (rcount * NUM_CORE), rtype,
                rank - 1, tag + i + 1, comm, &request);
      MPI_Wait(&request, &status);
      MPI_Isend((char *) rbuf + recv_offset, (scount * NUM_CORE), stype,
                (rank + 1), tag + i + 1, comm, &request);
    }
  }

  return MPI_SUCCESS;
}
