/* Copyright (c) 2013-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"

namespace simgrid{
namespace smpi{


int allgather__SMP_NTS(const void *sbuf, int scount,
                       MPI_Datatype stype, void *rbuf,
                       int rcount, MPI_Datatype rtype,
                       MPI_Comm comm)
{
  int src, dst, comm_size, rank;
  comm_size = comm->size();
  rank = comm->rank();
  MPI_Aint rextent, sextent;
  rextent = rtype->get_extent();
  sextent = stype->get_extent();
  int tag = COLL_TAG_ALLGATHER;

  int i, send_offset, recv_offset;
  int intra_rank, inter_rank;

  if(comm->get_leaders_comm()==MPI_COMM_NULL){
    comm->init_smp();
  }
  int num_core=1;
  if (comm->is_uniform()){
    num_core = comm->get_intra_comm()->size();
  }


  intra_rank = rank % num_core;
  inter_rank = rank / num_core;
  int inter_comm_size = (comm_size + num_core - 1) / num_core;
  int num_core_in_current_smp = num_core;

  if(comm_size%num_core)
    throw std::invalid_argument(xbt::string_printf(
        "allgather SMP NTS algorithm can't be used with non multiple of NUM_CORE=%d number of processes!", num_core));

  /* for too small number of processes, use default implementation */
  if (comm_size <= num_core) {
    XBT_WARN("MPI_allgather_SMP_NTS use default MPI_allgather.");
    allgather__default(sbuf, scount, stype, rbuf, rcount, rtype, comm);
    return MPI_SUCCESS;
  }

  // the last SMP node may have fewer number of running processes than all others
  if (inter_rank == (inter_comm_size - 1)) {
    num_core_in_current_smp = comm_size - (inter_rank * num_core);
  }
  //copy corresponding message from sbuf to rbuf
  recv_offset = rank * rextent * rcount;
  Request::sendrecv(sbuf, scount, stype, rank, tag,
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

    Request::sendrecv(sbuf, scount, stype, dst, tag,
                 ((char *) rbuf + recv_offset), rcount, rtype, src, tag, comm,
                 MPI_STATUS_IGNORE);

  }

  // INTER-SMP-ALLGATHER
  // Every root of each SMP node post INTER-Sendrecv, then do INTRA-Bcast for each receiving message
  // Use logical ring algorithm

  // root of each SMP
  if (intra_rank == 0) {
    auto* rrequest_array = new MPI_Request[inter_comm_size - 1];
    auto* srequest_array = new MPI_Request[inter_comm_size - 1];

    src = ((inter_rank - 1 + inter_comm_size) % inter_comm_size) * num_core;
    dst = ((inter_rank + 1) % inter_comm_size) * num_core;

    // post all inter Irecv
    for (i = 0; i < inter_comm_size - 1; i++) {
      recv_offset =
          ((inter_rank - i - 1 +
            inter_comm_size) % inter_comm_size) * num_core * sextent * scount;
      rrequest_array[i] = Request::irecv((char *)rbuf + recv_offset, rcount * num_core,
                                         rtype, src, tag + i, comm);
    }

    // send first message
    send_offset =
        ((inter_rank +
          inter_comm_size) % inter_comm_size) * num_core * sextent * scount;
    srequest_array[0] = Request::isend((char *)rbuf + send_offset, scount * num_core,
                                       stype, dst, tag, comm);

    // loop : recv-inter , send-inter, send-intra (linear-bcast)
    for (i = 0; i < inter_comm_size - 2; i++) {
      recv_offset =
          ((inter_rank - i - 1 +
            inter_comm_size) % inter_comm_size) * num_core * sextent * scount;
      Request::wait(&rrequest_array[i], MPI_STATUS_IGNORE);
      srequest_array[i + 1] = Request::isend((char *)rbuf + recv_offset, scount * num_core,
                                             stype, dst, tag + i + 1, comm);
      if (num_core_in_current_smp > 1) {
        Request::send((char *)rbuf + recv_offset, scount * num_core,
                      stype, (rank + 1), tag + i + 1, comm);
      }
    }

    // recv last message and send_intra
    recv_offset =
        ((inter_rank - i - 1 +
          inter_comm_size) % inter_comm_size) * num_core * sextent * scount;
    //recv_offset = ((inter_rank + 1) % inter_comm_size) * num_core * sextent * scount;
    //i=inter_comm_size-2;
    Request::wait(&rrequest_array[i], MPI_STATUS_IGNORE);
    if (num_core_in_current_smp > 1) {
      Request::send((char *)rbuf + recv_offset, scount * num_core,
                                  stype, (rank + 1), tag + i + 1, comm);
    }

    Request::waitall(inter_comm_size - 1, srequest_array, MPI_STATUSES_IGNORE);
    delete[] rrequest_array;
    delete[] srequest_array;
  }
  // last rank of each SMP
  else if (intra_rank == (num_core_in_current_smp - 1)) {
    for (i = 0; i < inter_comm_size - 1; i++) {
      recv_offset =
          ((inter_rank - i - 1 +
            inter_comm_size) % inter_comm_size) * num_core * sextent * scount;
      Request::recv((char *) rbuf + recv_offset, (rcount * num_core), rtype,
                    rank - 1, tag + i + 1, comm, MPI_STATUS_IGNORE);
    }
  }
  // intermediate rank of each SMP
  else {
    for (i = 0; i < inter_comm_size - 1; i++) {
      recv_offset =
          ((inter_rank - i - 1 +
            inter_comm_size) % inter_comm_size) * num_core * sextent * scount;
      Request::recv((char *) rbuf + recv_offset, (rcount * num_core), rtype,
                    rank - 1, tag + i + 1, comm, MPI_STATUS_IGNORE);
      Request::send((char *) rbuf + recv_offset, (scount * num_core), stype,
                    (rank + 1), tag + i + 1, comm);
    }
  }

  return MPI_SUCCESS;
}


}
}
