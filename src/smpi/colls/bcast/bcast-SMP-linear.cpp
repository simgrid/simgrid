/* Copyright (c) 2013-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"

int bcast_SMP_linear_segment_byte = 8192;
namespace simgrid{
namespace smpi{
int bcast__SMP_linear(void *buf, int count,
                      MPI_Datatype datatype, int root,
                      MPI_Comm comm)
{
  int tag = COLL_TAG_BCAST;
  MPI_Status status;
  MPI_Request request;
  int rank, size;
  int i;
  MPI_Aint extent;
  extent = datatype->get_extent();

  rank = comm->rank();
  size = comm->size();
  if(comm->get_leaders_comm()==MPI_COMM_NULL){
    comm->init_smp();
  }
  int num_core=1;
  if (comm->is_uniform()){
    num_core = comm->get_intra_comm()->size();
  }else{
    //implementation buggy in this case
    return bcast__mpich(buf, count, datatype, root, comm);
  }

  int segment = bcast_SMP_linear_segment_byte / extent;
  segment =  segment == 0 ? 1 :segment;
  int pipe_length = count / segment;
  int remainder = count % segment;
  int increment = segment * extent;


  /* leader of each SMP do inter-communication
     and act as a root for intra-communication */
  int to_inter = (rank + num_core) % size;
  int to_intra = (rank + 1) % size;
  int from_inter = (rank - num_core + size) % size;
  int from_intra = (rank + size - 1) % size;

  // call native when MPI communication size is too small
  if (size <= num_core) {
    XBT_WARN("MPI_bcast_SMP_linear use default MPI_bcast.");
    bcast__default(buf, count, datatype, root, comm);
    return MPI_SUCCESS;
  }
  // if root is not zero send to rank zero first
  if (root != 0) {
    if (rank == root)
      Request::send(buf, count, datatype, 0, tag, comm);
    else if (rank == 0)
      Request::recv(buf, count, datatype, root, tag, comm, &status);
  }
  // when a message is smaller than a block size => no pipeline
  if (count <= segment) {
    // case ROOT
    if (rank == 0) {
      Request::send(buf, count, datatype, to_inter, tag, comm);
      Request::send(buf, count, datatype, to_intra, tag, comm);
    }
    // case last ROOT of each SMP
    else if (rank == (((size - 1) / num_core) * num_core)) {
      request = Request::irecv(buf, count, datatype, from_inter, tag, comm);
      Request::wait(&request, &status);
      Request::send(buf, count, datatype, to_intra, tag, comm);
    }
    // case intermediate ROOT of each SMP
    else if (rank % num_core == 0) {
      request = Request::irecv(buf, count, datatype, from_inter, tag, comm);
      Request::wait(&request, &status);
      Request::send(buf, count, datatype, to_inter, tag, comm);
      Request::send(buf, count, datatype, to_intra, tag, comm);
    }
    // case last non-ROOT of each SMP
    else if (((rank + 1) % num_core == 0) || (rank == (size - 1))) {
      request = Request::irecv(buf, count, datatype, from_intra, tag, comm);
      Request::wait(&request, &status);
    }
    // case intermediate non-ROOT of each SMP
    else {
      request = Request::irecv(buf, count, datatype, from_intra, tag, comm);
      Request::wait(&request, &status);
      Request::send(buf, count, datatype, to_intra, tag, comm);
    }
    return MPI_SUCCESS;
  }
  // pipeline bcast
  else {
    MPI_Request* request_array = new MPI_Request[size + pipe_length];
    MPI_Status* status_array   = new MPI_Status[size + pipe_length];

    // case ROOT of each SMP
    if (rank % num_core == 0) {
      // case real root
      if (rank == 0) {
        for (i = 0; i < pipe_length; i++) {
          Request::send((char *) buf + (i * increment), segment, datatype, to_inter,
                   (tag + i), comm);
          Request::send((char *) buf + (i * increment), segment, datatype, to_intra,
                   (tag + i), comm);
        }
      }
      // case last ROOT of each SMP
      else if (rank == (((size - 1) / num_core) * num_core)) {
        for (i = 0; i < pipe_length; i++) {
          request_array[i] = Request::irecv((char *) buf + (i * increment), segment, datatype,
                    from_inter, (tag + i), comm);
        }
        for (i = 0; i < pipe_length; i++) {
          Request::wait(&request_array[i], &status);
          Request::send((char *) buf + (i * increment), segment, datatype, to_intra,
                   (tag + i), comm);
        }
      }
      // case intermediate ROOT of each SMP
      else {
        for (i = 0; i < pipe_length; i++) {
          request_array[i] = Request::irecv((char *) buf + (i * increment), segment, datatype,
                    from_inter, (tag + i), comm);
        }
        for (i = 0; i < pipe_length; i++) {
          Request::wait(&request_array[i], &status);
          Request::send((char *) buf + (i * increment), segment, datatype, to_inter,
                   (tag + i), comm);
          Request::send((char *) buf + (i * increment), segment, datatype, to_intra,
                   (tag + i), comm);
        }
      }
    } else {                    // case last non-ROOT of each SMP
      if (((rank + 1) % num_core == 0) || (rank == (size - 1))) {
        for (i = 0; i < pipe_length; i++) {
          request_array[i] = Request::irecv((char *) buf + (i * increment), segment, datatype,
                    from_intra, (tag + i), comm);
        }
        for (i = 0; i < pipe_length; i++) {
          Request::wait(&request_array[i], &status);
        }
      }
      // case intermediate non-ROOT of each SMP
      else {
        for (i = 0; i < pipe_length; i++) {
          request_array[i] = Request::irecv((char *) buf + (i * increment), segment, datatype,
                    from_intra, (tag + i), comm);
        }
        for (i = 0; i < pipe_length; i++) {
          Request::wait(&request_array[i], &status);
          Request::send((char *) buf + (i * increment), segment, datatype, to_intra,
                   (tag + i), comm);
        }
      }
    }
    delete[] request_array;
    delete[] status_array;
  }

  // when count is not divisible by block size, use default BCAST for the remainder
  if ((remainder != 0) && (count > segment)) {
    XBT_WARN("MPI_bcast_SMP_linear use default MPI_bcast.");
    Colls::bcast((char *) buf + (pipe_length * increment), remainder, datatype,
              root, comm);
  }

  return MPI_SUCCESS;
}

}
}
