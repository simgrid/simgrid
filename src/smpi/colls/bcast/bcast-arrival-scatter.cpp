/* Copyright (c) 2013-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"

#ifndef BCAST_ARRIVAL_PATTERN_AWARE_HEADER_SIZE
#define BCAST_ARRIVAL_PATTERN_AWARE_HEADER_SIZE 128
#endif

#ifndef BCAST_ARRIVAL_PATTERN_AWARE_MAX_NODE
#define BCAST_ARRIVAL_PATTERN_AWARE_MAX_NODE 128
#endif
namespace simgrid{
namespace smpi{
/* Non-topology-specific pipelined linear-bcast function */
int bcast__arrival_scatter(void *buf, int count,
                           MPI_Datatype datatype, int root,
                           MPI_Comm comm)
{
  int tag = -COLL_TAG_BCAST;//in order to use ANY_TAG, make this one positive
  int header_tag = -10;
  MPI_Status status;

  int curr_remainder;
  int curr_size;
  int curr_increment;
  int send_offset;
  int recv_offset;
  int send_count;
  int recv_count;

  MPI_Status temp_status_array[BCAST_ARRIVAL_PATTERN_AWARE_MAX_NODE];

  int rank, size;
  int i, k;

  int sent_count;
  int header_index;
  int flag_array[BCAST_ARRIVAL_PATTERN_AWARE_MAX_NODE];
  int already_sent[BCAST_ARRIVAL_PATTERN_AWARE_MAX_NODE];
  int header_buf[BCAST_ARRIVAL_PATTERN_AWARE_HEADER_SIZE];
  char temp_buf[BCAST_ARRIVAL_PATTERN_AWARE_MAX_NODE];
  int will_send[BCAST_ARRIVAL_PATTERN_AWARE_MAX_NODE];
  int max_node = BCAST_ARRIVAL_PATTERN_AWARE_MAX_NODE;
  int header_size = BCAST_ARRIVAL_PATTERN_AWARE_HEADER_SIZE;

  MPI_Aint extent;
  extent = datatype->get_extent();


  /* source and destination */
  int to, from;

  rank = comm->rank();
  size = comm->size();

  /* message too small */
  if (count < size) {
    XBT_WARN("MPI_bcast_arrival_scatter use default MPI_bcast.");
    colls::bcast(buf, count, datatype, root, comm);
    return MPI_SUCCESS;
  }



  /* if root is not zero send to rank zero first
     this can be modified to make it faster by using logical src, dst.
   */
  if (root != 0) {
    if (rank == root) {
      Request::send(buf, count, datatype, 0, tag - 1, comm);
    } else if (rank == 0) {
      Request::recv(buf, count, datatype, root, tag - 1, comm, &status);
    }
  }


  /* value == 0 means root has not send data (or header) to the node yet */
  for (i = 0; i < max_node; i++) {
    already_sent[i] = 0;
  }

  /* start bcast */

  /* root */
  if (rank == 0) {

    for (i = 0; i < max_node; i++)
      will_send[i] = 0;

    sent_count = 0;
    while (sent_count < (size - 1)) {

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

      /* recv 1-byte message in this round */
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

      /*
         if (header_index != 0) {
         printf("header index = %d node = ",header_index);
         for (i=0;i<header_index;i++) {
         printf("%d ",header_buf[i]);
         }
         printf("\n");
         }
       */

      /* send header followed by data */
      if (header_index != 0) {
        header_buf[header_index] = -1;

        /* send header */
        for (i = 0; i < header_index; i++) {
          to = header_buf[i];
          Request::send(header_buf, header_size, MPI_INT, to, header_tag, comm);
        }

        curr_remainder = count % header_index;
        curr_size = (count / header_index);
        curr_increment = curr_size * extent;

        /* send data */

        for (i = 0; i < header_index; i++) {
          to = header_buf[i];
          if ((i == (header_index - 1)) || (curr_size == 0))
            curr_size += curr_remainder;
          //printf("Root send to %d index %d\n",to,(i*curr_increment));
          Request::send((char *) buf + (i * curr_increment), curr_size, datatype, to,
                   tag, comm);
        }
      }
    }                           /* while (sent_count < size-1) */
  }

  /* rank 0 */
  /* none root */
  else {
    /* send 1-byte message to root */
    Request::send(temp_buf, 1, MPI_CHAR, 0, tag, comm);

    /* wait for header forward when required */
    Request::recv(header_buf, header_size, MPI_INT, 0, header_tag, comm, &status);

    /* search for where it is */
    int myordering = 0;
    while (rank != header_buf[myordering]) {
      myordering++;
    }

    int total_nodes = 0;
    while (header_buf[total_nodes] != -1) {
      total_nodes++;
    }

    curr_remainder = count % total_nodes;
    curr_size = (count / total_nodes);
    curr_increment = curr_size * extent;
    int recv_size = curr_size;

    /* receive data */
    if (myordering == (total_nodes - 1))
      recv_size += curr_remainder;
    Request::recv((char *) buf + (myordering * curr_increment), recv_size, datatype,
             0, tag, comm, &status);

    /* at this point all nodes in this set perform all-gather operation */

    to = (myordering == (total_nodes - 1)) ? header_buf[0] : header_buf[myordering + 1];
    from = (myordering == 0) ? header_buf[total_nodes - 1] : header_buf[myordering - 1];


    /* last segment may have a larger size since it also include the remainder */
    int last_segment_ptr = (total_nodes - 1) * (count / total_nodes) * extent;


    /* allgather */
    for (i = 0; i < total_nodes - 1; i++) {
      send_offset =
          ((myordering - i + total_nodes) % total_nodes) * curr_increment;
      recv_offset =
          ((myordering - i - 1 + total_nodes) % total_nodes) * curr_increment;

      /* adjust size */
      if (send_offset != last_segment_ptr)
        send_count = curr_size;
      else
        send_count = curr_size + curr_remainder;

      if (recv_offset != last_segment_ptr)
        recv_count = curr_size;
      else
        recv_count = curr_size + curr_remainder;

      //printf("\t\tnode %d sent_to %d recv_from %d send_size %d recv_size %d\n",rank,to,from,send_count,recv_count);
      //printf("\tnode %d sent_offset %d send_count %d\n",rank,send_offset,send_count);


      Request::sendrecv((char *) buf + send_offset, send_count, datatype, to,
                   tag + i, (char *) buf + recv_offset, recv_count, datatype,
                   from, tag + i, comm, &status);
    }
  }                             /* non-root */

  return MPI_SUCCESS;
}

}
}
