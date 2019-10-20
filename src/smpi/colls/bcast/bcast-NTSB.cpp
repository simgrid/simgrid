/* Copyright (c) 2013-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"

int bcast_NTSB_segment_size_in_byte = 8192;
namespace simgrid{
namespace smpi{
int Coll_bcast_NTSB::bcast(void *buf, int count, MPI_Datatype datatype,
                               int root, MPI_Comm comm)
{
  int tag = COLL_TAG_BCAST;
  MPI_Status status;
  int rank, size;
  int i;

  MPI_Aint extent;
  extent = datatype->get_extent();

  rank = comm->rank();
  size = comm->size();

  /* source node and destination nodes (same through out the functions) */
  int from = (rank - 1) / 2;
  int to_left = rank * 2 + 1;
  int to_right = rank * 2 + 2;
  if (to_left >= size)
    to_left = -1;
  if (to_right >= size)
    to_right = -1;

  /* segment is segment size in number of elements (not bytes) */
  int segment = bcast_NTSB_segment_size_in_byte / extent;
  segment =  segment == 0 ? 1 :segment;
  /* pipeline length */
  int pipe_length = count / segment;

  /* use for buffer offset for sending and receiving data = segment size in byte */
  int increment = segment * extent;

  /* if the input size is not divisible by segment size =>
     the small remainder will be done with native implementation */
  int remainder = count % segment;

  /* if root is not zero send to rank zero first */
  if (root != 0) {
    if (rank == root) {
      Request::send(buf, count, datatype, 0, tag, comm);
    } else if (rank == 0) {
      Request::recv(buf, count, datatype, root, tag, comm, &status);
    }
  }

  /* when a message is smaller than a block size => no pipeline */
  if (count <= segment) {

    /* case: root */
    if (rank == 0) {
      /* case root has only a left child */
      if (to_right == -1) {
        Request::send(buf, count, datatype, to_left, tag, comm);
      }
      /* case root has both left and right children */
      else {
        Request::send(buf, count, datatype, to_left, tag, comm);
        Request::send(buf, count, datatype, to_right, tag, comm);
      }
    }

    /* case: leaf ==> receive only */
    else if (to_left == -1) {
      Request::recv(buf, count, datatype, from, tag, comm, &status);
    }

    /* case: intermediate node with only left child ==> relay message */
    else if (to_right == -1) {
      Request::recv(buf, count, datatype, from, tag, comm, &status);
      Request::send(buf, count, datatype, to_left, tag, comm);
    }

    /* case: intermediate node with both left and right children ==> relay message */
    else {
      Request::recv(buf, count, datatype, from, tag, comm, &status);
      Request::send(buf, count, datatype, to_left, tag, comm);
      Request::send(buf, count, datatype, to_right, tag, comm);
    }
    return MPI_SUCCESS;
  }
  // pipelining
  else {

    MPI_Request* send_request_array = new MPI_Request[2 * (size + pipe_length)];
    MPI_Request* recv_request_array = new MPI_Request[size + pipe_length];
    MPI_Status* send_status_array   = new MPI_Status[2 * (size + pipe_length)];
    MPI_Status* recv_status_array   = new MPI_Status[size + pipe_length];

    /* case: root */
    if (rank == 0) {
      /* case root has only a left child */
      if (to_right == -1) {
        for (i = 0; i < pipe_length; i++) {
          send_request_array[i] = Request::isend((char *) buf + (i * increment), segment, datatype, to_left,
                    tag + i, comm);
        }
        Request::waitall((pipe_length), send_request_array, send_status_array);
      }
      /* case root has both left and right children */
      else {
        for (i = 0; i < pipe_length; i++) {
          send_request_array[i] = Request::isend((char *) buf + (i * increment), segment, datatype, to_left,
                    tag + i, comm);
          send_request_array[i + pipe_length] = Request::isend((char *) buf + (i * increment), segment, datatype, to_right,
                    tag + i, comm);
        }
        Request::waitall((2 * pipe_length), send_request_array, send_status_array);
      }
    }

    /* case: leaf ==> receive only */
    else if (to_left == -1) {
      for (i = 0; i < pipe_length; i++) {
        recv_request_array[i] = Request::irecv((char *) buf + (i * increment), segment, datatype, from,
                  tag + i, comm);
      }
      Request::waitall((pipe_length), recv_request_array, recv_status_array);
    }

    /* case: intermediate node with only left child ==> relay message */
    else if (to_right == -1) {
      for (i = 0; i < pipe_length; i++) {
        recv_request_array[i] = Request::irecv((char *) buf + (i * increment), segment, datatype, from,
                  tag + i, comm);
      }
      for (i = 0; i < pipe_length; i++) {
        Request::wait(&recv_request_array[i], &status);
        send_request_array[i] = Request::isend((char *) buf + (i * increment), segment, datatype, to_left,
                  tag + i, comm);
      }
      Request::waitall(pipe_length, send_request_array, send_status_array);

    }
    /* case: intermediate node with both left and right children ==> relay message */
    else {
      for (i = 0; i < pipe_length; i++) {
        recv_request_array[i] = Request::irecv((char *) buf + (i * increment), segment, datatype, from,
                  tag + i, comm);
      }
      for (i = 0; i < pipe_length; i++) {
        Request::wait(&recv_request_array[i], &status);
        send_request_array[i] = Request::isend((char *) buf + (i * increment), segment, datatype, to_left,
                  tag + i, comm);
        send_request_array[i + pipe_length] = Request::isend((char *) buf + (i * increment), segment, datatype, to_right,
                  tag + i, comm);
      }
      Request::waitall((2 * pipe_length), send_request_array, send_status_array);
    }

    delete[] send_request_array;
    delete[] recv_request_array;
    delete[] send_status_array;
    delete[] recv_status_array;
  }                             /* end pipeline */

  /* when count is not divisible by block size, use default BCAST for the remainder */
  if ((remainder != 0) && (count > segment)) {
    XBT_WARN("MPI_bcast_NTSB use default MPI_bcast.");
    Colls::bcast((char *) buf + (pipe_length * increment), remainder, datatype,
              root, comm);
  }

  return MPI_SUCCESS;
}

}
}
