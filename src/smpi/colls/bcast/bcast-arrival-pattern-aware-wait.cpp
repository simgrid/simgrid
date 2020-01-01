/* Copyright (c) 2013-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"

int bcast_arrival_pattern_aware_wait_segment_size_in_byte = 8192;

#ifndef BCAST_ARRIVAL_PATTERN_AWARE_HEADER_SIZE
#define BCAST_ARRIVAL_PATTERN_AWARE_HEADER_SIZE 1024
#endif

#ifndef BCAST_ARRIVAL_PATTERN_AWARE_MAX_NODE
#define BCAST_ARRIVAL_PATTERN_AWARE_MAX_NODE 128
#endif
namespace simgrid{
namespace smpi{
/* Non-topology-specific pipelined linear-bcast function */
int bcast__arrival_pattern_aware_wait(void *buf, int count,
                                      MPI_Datatype datatype,
                                      int root, MPI_Comm comm)
{
  MPI_Status status;
  MPI_Request request;

  MPI_Status temp_status_array[BCAST_ARRIVAL_PATTERN_AWARE_MAX_NODE];

  int rank, size;
  int i, j, k;
  int tag = -COLL_TAG_BCAST;
  int will_send[BCAST_ARRIVAL_PATTERN_AWARE_MAX_NODE];

  int sent_count;
  int header_index;
  int flag_array[BCAST_ARRIVAL_PATTERN_AWARE_MAX_NODE];
  int already_sent[BCAST_ARRIVAL_PATTERN_AWARE_MAX_NODE];

  int header_buf[BCAST_ARRIVAL_PATTERN_AWARE_HEADER_SIZE];
  char temp_buf[BCAST_ARRIVAL_PATTERN_AWARE_MAX_NODE];

  int max_node = BCAST_ARRIVAL_PATTERN_AWARE_MAX_NODE;
  int header_size = BCAST_ARRIVAL_PATTERN_AWARE_HEADER_SIZE;

  MPI_Aint extent;
  extent = datatype->get_extent();

  /* source and destination */
  int to, from;



  rank = comm->rank();
  size = comm->size();


  /* segment is segment size in number of elements (not bytes) */
  int segment = bcast_arrival_pattern_aware_wait_segment_size_in_byte / extent;
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


  /* value == 0 means root has not send data (or header) to the node yet */
  for (i = 0; i < max_node; i++) {
    already_sent[i] = 0;
  }

  /* when a message is smaller than a block size => no pipeline */
  if (count <= segment) {
    segment = count;
    pipe_length = 1;
  }

  /* start pipeline bcast */

  MPI_Request* send_request_array = new MPI_Request[size + pipe_length];
  MPI_Request* recv_request_array = new MPI_Request[size + pipe_length];
  MPI_Status* send_status_array   = new MPI_Status[size + pipe_length];
  MPI_Status* recv_status_array   = new MPI_Status[size + pipe_length];

  /* root */
  if (rank == 0) {
    sent_count = 0;
    int iteration = 0;

    for (i = 0; i < BCAST_ARRIVAL_PATTERN_AWARE_MAX_NODE; i++)
      will_send[i] = 0;
    while (sent_count < (size - 1)) {
      iteration++;

      /* loop k times to let more processes arrive before start sending data */
      for (k = 0; k < 3; k++) {
        for (i = 1; i < size; i++) {
          if ((already_sent[i] == 0) && (will_send[i] == 0)) {
            Request::iprobe(i, MPI_ANY_TAG, comm, &flag_array[i],
                       &temp_status_array[i]);
            if (flag_array[i] == 1) {
              will_send[i] = 1;
              Request::recv(&temp_buf[i], 1, MPI_CHAR, i, tag, comm,
                       &status);
              i = 0;
            }
          }
        }
      }

      header_index = 0;

      /* recv 1-byte message */
      for (i = 1; i < size; i++) {
        /* message arrive */
        if ((will_send[i] == 1) && (already_sent[i] == 0)) {
          header_buf[header_index] = i;
          header_index++;
          sent_count++;

          /* will send in the next step */
          already_sent[i] = 1;
        }
      }

      /* send header followed by data */
      if (header_index != 0) {
        header_buf[header_index] = -1;
        to = header_buf[0];

        /* send header */
        Request::send(header_buf, header_size, MPI_INT, to, tag, comm);

        /* send data - pipeline */
        for (i = 0; i < pipe_length; i++) {
          send_request_array[i] = Request::isend((char *)buf + (i * increment), segment, datatype, to, tag, comm);
        }
        Request::waitall((pipe_length), send_request_array, send_status_array);
      }


      /* end - send header followed by data */
      /* randomly MPI_Send to one node */
      /* this part has been commented out - performance-wise */
      else if (2 == 3) {
        /* search for the first node that never received data before */
        for (i = 0; i < size; i++) {
          if (i == root)
            continue;
          if (already_sent[i] == 0) {
            header_buf[0] = i;
            header_buf[1] = -1;
            to = i;

            Request::send(header_buf, header_size, MPI_INT, to, tag, comm);

            /* still need to chop data so that we can use the same non-root code */
            for (j = 0; j < pipe_length; j++) {
              Request::send((char *)buf + (j * increment), segment, datatype, to, tag, comm);
            }
          }
        }
      }
    }                           /* end - while (send_count < size-1) loop */
  }

  /* end - root */
  /* none root */
  else {

    /* send 1-byte message to root */
    Request::send(temp_buf, 1, MPI_CHAR, 0, tag, comm);

    /* wait for header forward when required */
    request = Request::irecv(header_buf, header_size, MPI_INT, MPI_ANY_SOURCE, tag, comm);
    Request::wait(&request, MPI_STATUS_IGNORE);

    /* search for where it is */
    int myordering = 0;
    while (rank != header_buf[myordering]) {
      myordering++;
    }

    to = header_buf[myordering + 1];
    if (myordering == 0) {
      from = 0;
    } else {
      from = header_buf[myordering - 1];
    }

    /* send header when required */
    if (to != -1) {
      Request::send(header_buf, header_size, MPI_INT, to, tag, comm);
    }

    /* receive data */

    for (i = 0; i < pipe_length; i++) {
      recv_request_array[i] = Request::irecv((char *)buf + (i * increment), segment, datatype, from, tag, comm);
    }

    /* forward data */
    if (to != -1) {
      for (i = 0; i < pipe_length; i++) {
        Request::wait(&recv_request_array[i], MPI_STATUS_IGNORE);
        send_request_array[i] = Request::isend((char *)buf + (i * increment), segment, datatype, to, tag, comm);
      }
      Request::waitall((pipe_length), send_request_array, send_status_array);
    }

    /* recv only */
    else {
      Request::waitall((pipe_length), recv_request_array, recv_status_array);
    }
  }

  delete[] send_request_array;
  delete[] recv_request_array;
  delete[] send_status_array;
  delete[] recv_status_array;
  /* end pipeline */

  /* when count is not divisible by block size, use default BCAST for the remainder */
  if ((remainder != 0) && (count > segment)) {
    XBT_WARN("MPI_bcast_arrival_pattern_aware_wait use default MPI_bcast.");
    colls::bcast((char*)buf + (pipe_length * increment), remainder, datatype, root, comm);
  }

  return MPI_SUCCESS;
}

}
}
