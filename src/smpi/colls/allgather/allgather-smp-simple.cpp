/* Copyright (c) 2013-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"

namespace simgrid{
namespace smpi{


int allgather__smp_simple(const void *send_buf, int scount,
                          MPI_Datatype stype, void *recv_buf,
                          int rcount, MPI_Datatype rtype,
                          MPI_Comm comm)
{
  int src, dst, comm_size, rank;
  comm_size = comm->size();

  if(comm->get_leaders_comm()==MPI_COMM_NULL){
    comm->init_smp();
  }
  int num_core=1;
  if (comm->is_uniform()){
    num_core = comm->get_intra_comm()->size();
  }

  if(comm_size%num_core)
    throw std::invalid_argument(xbt::string_printf(
        "allgather SMP simple algorithm can't be used with non multiple of NUM_CORE=%d number of processes!",
        num_core));

  rank = comm->rank();
  MPI_Aint rextent, sextent;
  rextent = rtype->get_extent();
  sextent = stype->get_extent();
  int tag = COLL_TAG_ALLGATHER;
  MPI_Status status;
  int i, send_offset, recv_offset;
  int intra_rank, inter_rank;
  intra_rank = rank % num_core;
  inter_rank = rank / num_core;
  int inter_comm_size = (comm_size + num_core - 1) / num_core;
  int num_core_in_current_smp = num_core;

  // the last SMP node may have fewer number of running processes than all others
  if (inter_rank == (inter_comm_size - 1)) {
    num_core_in_current_smp = comm_size - (inter_rank * num_core);
  }
  //INTRA-SMP-ALLGATHER
  recv_offset = rank * rextent * rcount;
  Request::sendrecv(send_buf, scount, stype, rank, tag,
               ((char *) recv_buf + recv_offset), rcount, rtype, rank, tag,
               comm, &status);
  for (i = 1; i < num_core_in_current_smp; i++) {

    dst =
        (inter_rank * num_core) + (intra_rank + i) % (num_core_in_current_smp);
    src =
        (inter_rank * num_core) + (intra_rank - i +
                                   num_core_in_current_smp) %
        (num_core_in_current_smp);
    recv_offset = src * rextent * rcount;

    Request::sendrecv(send_buf, scount, stype, dst, tag,
                 ((char *) recv_buf + recv_offset), rcount, rtype, src, tag,
                 comm, &status);

  }

  // INTER-SMP-ALLGATHER
  // Every root of each SMP node post INTER-Sendrecv, then do INTRA-Bcast for each receiving message



  if (intra_rank == 0) {
    int num_req = (inter_comm_size - 1) * 2;
    MPI_Request* reqs    = new MPI_Request[num_req];
    MPI_Request* req_ptr = reqs;
    MPI_Status* stat     = new MPI_Status[num_req];

    for (i = 1; i < inter_comm_size; i++) {

      //dst = ((inter_rank+i)%inter_comm_size) * num_core;
      src = ((inter_rank - i + inter_comm_size) % inter_comm_size) * num_core;
      //send_offset = (rank * sextent * scount);
      recv_offset = (src * sextent * scount);
      //      Request::sendrecv((recv_buf+send_offset), (scount * num_core), stype, dst, tag,
      //             (recv_buf+recv_offset), (rcount * num_core), rtype, src, tag, comm, &status);
      //MPIC_Isend((recv_buf+send_offset), (scount * num_core), stype, dst, tag, comm, req_ptr++);
      *(req_ptr++) = Request::irecv(((char *) recv_buf + recv_offset), (rcount * num_core), rtype,
                src, tag, comm);
    }
    for (i = 1; i < inter_comm_size; i++) {

      dst = ((inter_rank + i) % inter_comm_size) * num_core;
      //src = ((inter_rank-i+inter_comm_size)%inter_comm_size) * num_core;
      send_offset = (rank * sextent * scount);
      //recv_offset = (src * sextent * scount);
      //      Request::sendrecv((recv_buf+send_offset), (scount * num_core), stype, dst, tag,
      //             (recv_buf+recv_offset), (rcount * num_core), rtype, src, tag, comm, &status);
      *(req_ptr++) = Request::isend(((char *) recv_buf + send_offset), (scount * num_core), stype,
                dst, tag, comm);
      //MPIC_Irecv((recv_buf+recv_offset), (rcount * num_core), rtype, src, tag, comm, req_ptr++);
    }
    Request::waitall(num_req, reqs, stat);
    delete[] reqs;
    delete[] stat;
  }
  //INTRA-BCAST (use flat tree)

  if (intra_rank == 0) {
    for (i = 1; i < num_core_in_current_smp; i++) {
      //printf("rank = %d, num = %d send to %d\n",rank, num_core_in_current_smp, (rank + i));
      Request::send(recv_buf, (scount * comm_size), stype, (rank + i), tag, comm);
    }
  } else {
    //printf("rank = %d recv from %d\n",rank, (inter_rank * num_core));
    Request::recv(recv_buf, (rcount * comm_size), rtype, (inter_rank * num_core),
             tag, comm, &status);
  }


  return MPI_SUCCESS;
}


}
}
