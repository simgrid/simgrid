/* Copyright (c) 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "colls_private.h"

int smpi_coll_tuned_allgather_SMP_NTS(void *sbuf, int scount,
                                      MPI_Datatype stype, void *rbuf,
                                      int rcount, MPI_Datatype rtype,
                                      MPI_Comm comm)
{
  int src, dst, comm_size, rank;
  comm_size = smpi_comm_size(comm);
  rank = smpi_comm_rank(comm);
  MPI_Aint rextent, sextent;
  rextent = smpi_datatype_get_extent(rtype);
  sextent = smpi_datatype_get_extent(stype);
  int tag = COLL_TAG_ALLGATHER;

  int i, send_offset, recv_offset;
  int intra_rank, inter_rank;

  if(smpi_comm_get_leaders_comm(comm)==MPI_COMM_NULL){
    smpi_comm_init_smp(comm);
  }
  int num_core=1;
  if (smpi_comm_is_uniform(comm)){
    num_core = smpi_comm_size(smpi_comm_get_intra_comm(comm));
  }


  intra_rank = rank % num_core;
  inter_rank = rank / num_core;
  int inter_comm_size = (comm_size + num_core - 1) / num_core;
  int num_core_in_current_smp = num_core;

  if(comm_size%num_core)
    THROWF(arg_error,0, "allgather SMP NTS algorithm can't be used with non multiple of NUM_CORE=%d number of processes ! ", num_core);

  /* for too small number of processes, use default implementation */
  if (comm_size <= num_core) {
    XBT_WARN("MPI_allgather_SMP_NTS use default MPI_allgather.");  	  
    smpi_mpi_allgather(sbuf, scount, stype, rbuf, rcount, rtype, comm);
    return MPI_SUCCESS;    
  }

  // the last SMP node may have fewer number of running processes than all others
  if (inter_rank == (inter_comm_size - 1)) {
    num_core_in_current_smp = comm_size - (inter_rank * num_core);
  }
  //copy corresponding message from sbuf to rbuf
  recv_offset = rank * rextent * rcount;
  smpi_mpi_sendrecv(sbuf, scount, stype, rank, tag,
               ((char *) rbuf + recv_offset), rcount, rtype, rank, tag, comm,
               MPI_STATUS_IGNORE);

  //gather to root of each SMP

  for (i = 1; i < num_core_in_current_smp; i++) {

    dst =
        (inter_rank * num_core) + (intra_rank + i) % (num_core_in_current_smp);
    src =
        (inter_rank * num_core) + (intra_rank - i +
                                   num_core_in_current_smp) %
        (num_core_in_current_smp);
    recv_offset = src * rextent * rcount;

    smpi_mpi_sendrecv(sbuf, scount, stype, dst, tag,
                 ((char *) rbuf + recv_offset), rcount, rtype, src, tag, comm,
                 MPI_STATUS_IGNORE);

  }

  // INTER-SMP-ALLGATHER 
  // Every root of each SMP node post INTER-Sendrecv, then do INTRA-Bcast for each receiving message
  // Use logical ring algorithm

  // root of each SMP
  if (intra_rank == 0) {
    MPI_Request *rrequest_array = xbt_new(MPI_Request, inter_comm_size - 1);
    MPI_Request *srequest_array = xbt_new(MPI_Request, inter_comm_size - 1);

    src = ((inter_rank - 1 + inter_comm_size) % inter_comm_size) * num_core;
    dst = ((inter_rank + 1) % inter_comm_size) * num_core;

    // post all inter Irecv
    for (i = 0; i < inter_comm_size - 1; i++) {
      recv_offset =
          ((inter_rank - i - 1 +
            inter_comm_size) % inter_comm_size) * num_core * sextent * scount;
      rrequest_array[i] = smpi_mpi_irecv((char *)rbuf + recv_offset, rcount * num_core,
                                         rtype, src, tag + i, comm);
    }

    // send first message
    send_offset =
        ((inter_rank +
          inter_comm_size) % inter_comm_size) * num_core * sextent * scount;
    srequest_array[0] = smpi_mpi_isend((char *)rbuf + send_offset, scount * num_core,
                                       stype, dst, tag, comm);

    // loop : recv-inter , send-inter, send-intra (linear-bcast)
    for (i = 0; i < inter_comm_size - 2; i++) {
      recv_offset =
          ((inter_rank - i - 1 +
            inter_comm_size) % inter_comm_size) * num_core * sextent * scount;
      smpi_mpi_wait(&rrequest_array[i], MPI_STATUS_IGNORE);
      srequest_array[i + 1] = smpi_mpi_isend((char *)rbuf + recv_offset, scount * num_core,
                                             stype, dst, tag + i + 1, comm);
      if (num_core_in_current_smp > 1) {
        smpi_mpi_send((char *)rbuf + recv_offset, scount * num_core,
                      stype, (rank + 1), tag + i + 1, comm);
      }
    }

    // recv last message and send_intra
    recv_offset =
        ((inter_rank - i - 1 +
          inter_comm_size) % inter_comm_size) * num_core * sextent * scount;
    //recv_offset = ((inter_rank + 1) % inter_comm_size) * num_core * sextent * scount;
    //i=inter_comm_size-2;
    smpi_mpi_wait(&rrequest_array[i], MPI_STATUS_IGNORE);
    if (num_core_in_current_smp > 1) {
      smpi_mpi_send((char *)rbuf + recv_offset, scount * num_core,
                                  stype, (rank + 1), tag + i + 1, comm);
    }

    smpi_mpi_waitall(inter_comm_size - 1, srequest_array, MPI_STATUSES_IGNORE);
    xbt_free(rrequest_array);
    xbt_free(srequest_array);
  }
  // last rank of each SMP
  else if (intra_rank == (num_core_in_current_smp - 1)) {
    for (i = 0; i < inter_comm_size - 1; i++) {
      recv_offset =
          ((inter_rank - i - 1 +
            inter_comm_size) % inter_comm_size) * num_core * sextent * scount;
      smpi_mpi_recv((char *) rbuf + recv_offset, (rcount * num_core), rtype,
                    rank - 1, tag + i + 1, comm, MPI_STATUS_IGNORE);
    }
  }
  // intermediate rank of each SMP
  else {
    for (i = 0; i < inter_comm_size - 1; i++) {
      recv_offset =
          ((inter_rank - i - 1 +
            inter_comm_size) % inter_comm_size) * num_core * sextent * scount;
      smpi_mpi_recv((char *) rbuf + recv_offset, (rcount * num_core), rtype,
                    rank - 1, tag + i + 1, comm, MPI_STATUS_IGNORE);
      smpi_mpi_send((char *) rbuf + recv_offset, (scount * num_core), stype,
                    (rank + 1), tag + i + 1, comm);
    }
  }

  return MPI_SUCCESS;
}
