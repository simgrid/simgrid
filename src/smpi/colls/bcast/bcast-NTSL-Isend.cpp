/* Copyright (c) 2013-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"

static int bcast_NTSL_segment_size_in_byte = 8192;

/* Non-topology-specific pipelined linear-bcast function
   0->1, 1->2 ,2->3, ....., ->last node : in a pipeline fashion
*/
namespace simgrid{
namespace smpi{
int Coll_bcast_NTSL_Isend::bcast(void *buf, int count, MPI_Datatype datatype,
                               int root, MPI_Comm comm)
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

  /* source node and destination nodes (same through out the functions) */
  int to = (rank + 1) % size;
  int from = (rank + size - 1) % size;

  /* segment is segment size in number of elements (not bytes) */
  int segment = bcast_NTSL_segment_size_in_byte / extent;
  segment =  segment == 0 ? 1 :segment;
  /* pipeline length */
  int pipe_length = count / segment;

  /* use for buffer offset for sending and receiving data = segment size in byte */
  int increment = segment * extent;

  /* if the input size is not divisible by segment size =>
     the small remainder will be done with native implementation */
  int remainder = count % segment;

  /* if root is not zero send to rank zero first
     this can be modified to make it faster by using logical src, dst.
   */
  if (root != 0) {
    if (rank == root) {
      Request::send(buf, count, datatype, 0, tag, comm);
    } else if (rank == 0) {
      Request::recv(buf, count, datatype, root, tag, comm, &status);
    }
  }

  /* when a message is smaller than a block size => no pipeline */
  if (count <= segment) {
    if (rank == 0) {
      Request::send(buf, count, datatype, to, tag, comm);
    } else if (rank == (size - 1)) {
      request = Request::irecv(buf, count, datatype, from, tag, comm);
      Request::wait(&request, &status);
    } else {
      request = Request::irecv(buf, count, datatype, from, tag, comm);
      Request::wait(&request, &status);
      Request::send(buf, count, datatype, to, tag, comm);
    }
    return MPI_SUCCESS;
  }

  /* pipeline bcast */
  else {
    MPI_Request* send_request_array = new MPI_Request[size + pipe_length];
    MPI_Request* recv_request_array = new MPI_Request[size + pipe_length];
    MPI_Status* send_status_array   = new MPI_Status[size + pipe_length];
    MPI_Status* recv_status_array   = new MPI_Status[size + pipe_length];

    /* root send data */
    if (rank == 0) {
      for (i = 0; i < pipe_length; i++) {
        send_request_array[i] = Request::isend((char *) buf + (i * increment), segment, datatype, to,
                  (tag + i), comm);
      }
      Request::waitall((pipe_length), send_request_array, send_status_array);
    }

    /* last node only receive data */
    else if (rank == (size - 1)) {
      for (i = 0; i < pipe_length; i++) {
        recv_request_array[i] = Request::irecv((char *) buf + (i * increment), segment, datatype, from,
                  (tag + i), comm);
      }
      Request::waitall((pipe_length), recv_request_array, recv_status_array);
    }

    /* intermediate nodes relay (receive, then send) data */
    else {
      for (i = 0; i < pipe_length; i++) {
        recv_request_array[i] = Request::irecv((char *) buf + (i * increment), segment, datatype, from,
                  (tag + i), comm);
      }
      for (i = 0; i < pipe_length; i++) {
        Request::wait(&recv_request_array[i], &status);
        send_request_array[i] = Request::isend((char *) buf + (i * increment), segment, datatype, to,
                  (tag + i), comm);
      }
      Request::waitall((pipe_length), send_request_array, send_status_array);
    }

    delete[] send_request_array;
    delete[] recv_request_array;
    delete[] send_status_array;
    delete[] recv_status_array;
  }                             /* end pipeline */

  /* when count is not divisible by block size, use default BCAST for the remainder */
  if ((remainder != 0) && (count > segment)) {
    XBT_WARN("MPI_bcast_NTSL_Isend_nb use default MPI_bcast.");
    Colls::bcast((char *) buf + (pipe_length * increment), remainder, datatype,
              root, comm);
  }

  return MPI_SUCCESS;
}

}
}
