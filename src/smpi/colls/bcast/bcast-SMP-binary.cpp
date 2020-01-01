/* Copyright (c) 2013-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"

int bcast_SMP_binary_segment_byte = 8192;
namespace simgrid{
namespace smpi{
int bcast__SMP_binary(void *buf, int count,
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
  int host_num_core=1;
  if (comm->is_uniform()){
    host_num_core = comm->get_intra_comm()->size();
  }else{
    //implementation buggy in this case
    return bcast__mpich(buf , count, datatype, root, comm);
  }

  int segment = bcast_SMP_binary_segment_byte / extent;
  int pipe_length = count / segment;
  int remainder = count % segment;

  int to_intra_left = (rank / host_num_core) * host_num_core + (rank % host_num_core) * 2 + 1;
  int to_intra_right = (rank / host_num_core) * host_num_core + (rank % host_num_core) * 2 + 2;
  int to_inter_left = ((rank / host_num_core) * 2 + 1) * host_num_core;
  int to_inter_right = ((rank / host_num_core) * 2 + 2) * host_num_core;
  int from_inter = (((rank / host_num_core) - 1) / 2) * host_num_core;
  int from_intra = (rank / host_num_core) * host_num_core + ((rank % host_num_core) - 1) / 2;
  int increment = segment * extent;

  int base = (rank / host_num_core) * host_num_core;
  int num_core = host_num_core;
  if (((rank / host_num_core) * host_num_core) == ((size / host_num_core) * host_num_core))
    num_core = size - (rank / host_num_core) * host_num_core;

  // if root is not zero send to rank zero first
  if (root != 0) {
    if (rank == root)
      Request::send(buf, count, datatype, 0, tag, comm);
    else if (rank == 0)
      Request::recv(buf, count, datatype, root, tag, comm, &status);
  }
  // when a message is smaller than a block size => no pipeline
  if (count <= segment) {
    // case ROOT-of-each-SMP
    if (rank % host_num_core == 0) {
      // case ROOT
      if (rank == 0) {
        //printf("node %d left %d right %d\n",rank,to_inter_left,to_inter_right);
        if (to_inter_left < size)
          Request::send(buf, count, datatype, to_inter_left, tag, comm);
        if (to_inter_right < size)
          Request::send(buf, count, datatype, to_inter_right, tag, comm);
        if ((to_intra_left - base) < num_core)
          Request::send(buf, count, datatype, to_intra_left, tag, comm);
        if ((to_intra_right - base) < num_core)
          Request::send(buf, count, datatype, to_intra_right, tag, comm);
      }
      // case LEAVES ROOT-of-eash-SMP
      else if (to_inter_left >= size) {
        //printf("node %d from %d\n",rank,from_inter);
        request = Request::irecv(buf, count, datatype, from_inter, tag, comm);
        Request::wait(&request, &status);
        if ((to_intra_left - base) < num_core)
          Request::send(buf, count, datatype, to_intra_left, tag, comm);
        if ((to_intra_right - base) < num_core)
          Request::send(buf, count, datatype, to_intra_right, tag, comm);
      }
      // case INTERMEDIAT ROOT-of-each-SMP
      else {
        //printf("node %d left %d right %d from %d\n",rank,to_inter_left,to_inter_right,from_inter);
        request = Request::irecv(buf, count, datatype, from_inter, tag, comm);
        Request::wait(&request, &status);
        Request::send(buf, count, datatype, to_inter_left, tag, comm);
        if (to_inter_right < size)
          Request::send(buf, count, datatype, to_inter_right, tag, comm);
        if ((to_intra_left - base) < num_core)
          Request::send(buf, count, datatype, to_intra_left, tag, comm);
        if ((to_intra_right - base) < num_core)
          Request::send(buf, count, datatype, to_intra_right, tag, comm);
      }
    }
    // case non ROOT-of-each-SMP
    else {
      // case leaves
      if ((to_intra_left - base) >= num_core) {
        request = Request::irecv(buf, count, datatype, from_intra, tag, comm);
        Request::wait(&request, &status);
      }
      // case intermediate
      else {
        request = Request::irecv(buf, count, datatype, from_intra, tag, comm);
        Request::wait(&request, &status);
        Request::send(buf, count, datatype, to_intra_left, tag, comm);
        if ((to_intra_right - base) < num_core)
          Request::send(buf, count, datatype, to_intra_right, tag, comm);
      }
    }

    return MPI_SUCCESS;
  }

  // pipeline bcast
  else {
    MPI_Request* request_array = new MPI_Request[size + pipe_length];
    MPI_Status* status_array   = new MPI_Status[size + pipe_length];

    // case ROOT-of-each-SMP
    if (rank % host_num_core == 0) {
      // case ROOT
      if (rank == 0) {
        for (i = 0; i < pipe_length; i++) {
          //printf("node %d left %d right %d\n",rank,to_inter_left,to_inter_right);
          if (to_inter_left < size)
            Request::send((char *) buf + (i * increment), segment, datatype,
                     to_inter_left, (tag + i), comm);
          if (to_inter_right < size)
            Request::send((char *) buf + (i * increment), segment, datatype,
                     to_inter_right, (tag + i), comm);
          if ((to_intra_left - base) < num_core)
            Request::send((char *) buf + (i * increment), segment, datatype,
                     to_intra_left, (tag + i), comm);
          if ((to_intra_right - base) < num_core)
            Request::send((char *) buf + (i * increment), segment, datatype,
                     to_intra_right, (tag + i), comm);
        }
      }
      // case LEAVES ROOT-of-eash-SMP
      else if (to_inter_left >= size) {
        //printf("node %d from %d\n",rank,from_inter);
        for (i = 0; i < pipe_length; i++) {
          request_array[i] = Request::irecv((char *) buf + (i * increment), segment, datatype,
                    from_inter, (tag + i), comm);
        }
        for (i = 0; i < pipe_length; i++) {
          Request::wait(&request_array[i], &status);
          if ((to_intra_left - base) < num_core)
            Request::send((char *) buf + (i * increment), segment, datatype,
                     to_intra_left, (tag + i), comm);
          if ((to_intra_right - base) < num_core)
            Request::send((char *) buf + (i * increment), segment, datatype,
                     to_intra_right, (tag + i), comm);
        }
      }
      // case INTERMEDIAT ROOT-of-each-SMP
      else {
        //printf("node %d left %d right %d from %d\n",rank,to_inter_left,to_inter_right,from_inter);
        for (i = 0; i < pipe_length; i++) {
          request_array[i] = Request::irecv((char *) buf + (i * increment), segment, datatype,
                    from_inter, (tag + i), comm);
        }
        for (i = 0; i < pipe_length; i++) {
          Request::wait(&request_array[i], &status);
          Request::send((char *) buf + (i * increment), segment, datatype,
                   to_inter_left, (tag + i), comm);
          if (to_inter_right < size)
            Request::send((char *) buf + (i * increment), segment, datatype,
                     to_inter_right, (tag + i), comm);
          if ((to_intra_left - base) < num_core)
            Request::send((char *) buf + (i * increment), segment, datatype,
                     to_intra_left, (tag + i), comm);
          if ((to_intra_right - base) < num_core)
            Request::send((char *) buf + (i * increment), segment, datatype,
                     to_intra_right, (tag + i), comm);
        }
      }
    }
    // case non-ROOT-of-each-SMP
    else {
      // case leaves
      if ((to_intra_left - base) >= num_core) {
        for (i = 0; i < pipe_length; i++) {
          request_array[i] = Request::irecv((char *) buf + (i * increment), segment, datatype,
                    from_intra, (tag + i), comm);
        }
        Request::waitall((pipe_length), request_array, status_array);
      }
      // case intermediate
      else {
        for (i = 0; i < pipe_length; i++) {
          request_array[i] = Request::irecv((char *) buf + (i * increment), segment, datatype,
                    from_intra, (tag + i), comm);
        }
        for (i = 0; i < pipe_length; i++) {
          Request::wait(&request_array[i], &status);
          Request::send((char *) buf + (i * increment), segment, datatype,
                   to_intra_left, (tag + i), comm);
          if ((to_intra_right - base) < num_core)
            Request::send((char *) buf + (i * increment), segment, datatype,
                     to_intra_right, (tag + i), comm);
        }
      }
    }

    delete[] request_array;
    delete[] status_array;
  }

  // when count is not divisible by block size, use default BCAST for the remainder
  if ((remainder != 0) && (count > segment)) {
    XBT_WARN("MPI_bcast_SMP_binary use default MPI_bcast.");
    colls::bcast((char*)buf + (pipe_length * increment), remainder, datatype, root, comm);
  }

  return 1;
}

}
}
