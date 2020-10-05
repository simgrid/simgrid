/* Copyright (c) 2013-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"
//#include <star-reduction.c>

int reduce_arrival_pattern_aware_segment_size_in_byte = 8192;

#ifndef HEADER_SIZE
#define HEADER_SIZE 1024
#endif

#ifndef MAX_NODE
#define MAX_NODE 1024
#endif
namespace simgrid{
namespace smpi{
/* Non-topology-specific pipelined linear-reduce function */
int reduce__arrival_pattern_aware(const void *buf, void *rbuf,
                                  int count,
                                  MPI_Datatype datatype,
                                  MPI_Op op, int root,
                                  MPI_Comm comm)
{
  int rank = comm->rank();
  int tag = -COLL_TAG_REDUCE;
  MPI_Status status;
  MPI_Request request;

  MPI_Status temp_status_array[MAX_NODE];

  int size = comm->size();
  int i;

  int sent_count;
  int header_index;
  int flag_array[MAX_NODE];
  int already_received[MAX_NODE];

  int header_buf[HEADER_SIZE];
  char temp_buf[MAX_NODE];

  MPI_Aint extent, lb;
  datatype->extent(&lb, &extent);

  /* source and destination */
  int to, from;

  /* segment is segment size in number of elements (not bytes) */
  int segment = reduce_arrival_pattern_aware_segment_size_in_byte / extent;

  /* pipeline length */
  int pipe_length = count / segment;

  /* use for buffer offset for sending and receiving data = segment size in byte */
  int increment = segment * extent;

  /* if the input size is not divisible by segment size =>
     the small remainder will be done with native implementation */
  int remainder = count % segment;


  /* value == 0 means root has not send data (or header) to the node yet */
  for (i = 0; i < MAX_NODE; i++) {
    already_received[i] = 0;
  }

  unsigned char* tmp_buf = smpi_get_tmp_sendbuffer(count * extent);

  Request::sendrecv(buf, count, datatype, rank, tag, rbuf, count, datatype, rank,
               tag, comm, &status);



  /* when a message is smaller than a block size => no pipeline */
  if (count <= segment) {

    if (rank == 0) {
      sent_count = 0;

      while (sent_count < (size - 1)) {

        for (i = 1; i < size; i++) {
          if (already_received[i] == 0) {
            Request::iprobe(i, MPI_ANY_TAG, comm, &flag_array[i], MPI_STATUSES_IGNORE);
            simgrid::s4u::this_actor::sleep_for(0.0001);
          }
            }

        header_index = 0;
        /* recv 1-byte message */
        for (i = 0; i < size; i++) {
          if (i == rank)
            continue;

          /* 1-byte message arrive */
          if ((flag_array[i] == 1) && (already_received[i] == 0)) {
            Request::recv(temp_buf, 1, MPI_CHAR, i, tag, comm, &status);
            header_buf[header_index] = i;
            header_index++;
            sent_count++;


            //printf("root send to %d recv from %d : data = ",to,from);
            /*
               for (i=0;i<=header_index;i++) {
               printf("%d ",header_buf[i]);
               }
               printf("\n");
             */
            /* will receive in the next step */
            already_received[i] = 1;
          }
        }

        /* send header followed by receive and reduce data */
        if (header_index != 0) {
          header_buf[header_index] = -1;
          to = header_buf[0];
          from = header_buf[header_index - 1];

          Request::send(header_buf, HEADER_SIZE, MPI_INT, to, tag, comm);
          Request::recv(tmp_buf, count, datatype, from, tag, comm, &status);
          if(op!=MPI_OP_NULL) op->apply( tmp_buf, rbuf, &count, datatype);
        }
      }                         /* while loop */
    }

    /* root */
    /* non-root */
    else {

      /* send 1-byte message to root */
      Request::send(temp_buf, 1, MPI_CHAR, 0, tag, comm);

      /* wait for header and data, forward when required */
      Request::recv(header_buf, HEADER_SIZE, MPI_INT, MPI_ANY_SOURCE, tag, comm,
               &status);
      //      Request::recv(buf,count,datatype,MPI_ANY_SOURCE,tag,comm,&status);

      /* search for where it is */
      int myordering = 0;
      while (rank != header_buf[myordering]) {
        myordering++;
      }

      /* forward header */
      if (header_buf[myordering + 1] != -1) {
          Request::send(header_buf, HEADER_SIZE, MPI_INT, header_buf[myordering + 1],
                 tag, comm);
      }
      //printf("node %d ordering %d\n",rank,myordering);

      /* receive, reduce, and forward data */

      /* send only */
      if (myordering == 0) {
        if (header_buf[myordering + 1] == -1) {
          to = 0;
        } else {
          to = header_buf[myordering + 1];
        }
        Request::send(rbuf, count, datatype, to, tag, comm);
      }

      /* recv, reduce, send */
      else {
        if (header_buf[myordering + 1] == -1) {
          to = 0;
        } else {
          to = header_buf[myordering + 1];
        }
        from = header_buf[myordering - 1];
        Request::recv(tmp_buf, count, datatype, from, tag, comm, &status);
        if(op!=MPI_OP_NULL) op->apply( tmp_buf, rbuf, &count, datatype);
        Request::send(rbuf, count, datatype, to, tag, comm);
      }
    }                           /* non-root */
  }
  /* pipeline bcast */
  else {
    //    printf("node %d start\n",rank);

    auto* send_request_array = new MPI_Request[size + pipe_length];
    auto* recv_request_array = new MPI_Request[size + pipe_length];
    auto* send_status_array  = new MPI_Status[size + pipe_length];
    auto* recv_status_array  = new MPI_Status[size + pipe_length];

    if (rank == 0) {
      sent_count = 0;

      int will_send[MAX_NODE];
      for (i = 0; i < MAX_NODE; i++)
        will_send[i] = 0;

      /* loop until all data are received (sent) */
      while (sent_count < (size - 1)) {
        int k;
        for (k = 0; k < 1; k++) {
          for (i = 1; i < size; i++) {
            //if (i == rank)
            //continue;
            if ((already_received[i] == 0) && (will_send[i] == 0)) {
                Request::iprobe(i, MPI_ANY_TAG, comm, &flag_array[i],
                         &temp_status_array[i]);
              if (flag_array[i] == 1) {
                will_send[i] = 1;
                Request::recv(&temp_buf[i], 1, MPI_CHAR, i, tag, comm,
                         &status);
                //printf("recv from %d\n",i);
                i = 1;
              }
            }
          }
        }                       /* end of probing */

        header_index = 0;

        /* recv 1-byte message */
        for (i = 1; i < size; i++) {
          //if (i==rank)
          //continue;
          /* message arrived in this round (put in the header) */
          if ((will_send[i] == 1) && (already_received[i] == 0)) {
            header_buf[header_index] = i;
            header_index++;
            sent_count++;

            /* will send in the next step */
            already_received[i] = 1;
          }
        }

        /* send header followed by data */
        if (header_index != 0) {
          header_buf[header_index] = -1;
          to = header_buf[0];

          /* send header */
          Request::send(header_buf, HEADER_SIZE, MPI_INT, to, tag, comm);

          /* recv data - pipeline */
          from = header_buf[header_index - 1];
          for (i = 0; i < pipe_length; i++) {
            Request::recv(tmp_buf + (i * increment), segment, datatype, from, tag,
                     comm, &status);
            if(op!=MPI_OP_NULL) op->apply( tmp_buf + (i * increment),
                           (char *)rbuf + (i * increment), &segment, datatype);
          }
        }
      }                         /* while loop (sent_count < size-1 ) */
    }

    /* root */
    /* none root */
    else {
      /* send 1-byte message to root */
      Request::send(temp_buf, 1, MPI_CHAR, 0, tag, comm);


      /* wait for header forward when required */
      request=Request::irecv(header_buf, HEADER_SIZE, MPI_INT, MPI_ANY_SOURCE, tag, comm);
      Request::wait(&request, MPI_STATUS_IGNORE);

      /* search for where it is */
      int myordering = 0;

      while (rank != header_buf[myordering]) {
        myordering++;
      }

      /* send header when required */
      if (header_buf[myordering + 1] != -1) {
          Request::send(header_buf, HEADER_SIZE, MPI_INT, header_buf[myordering + 1],
                 tag, comm);
      }

      /* (receive, reduce), and send data */
      if (header_buf[myordering + 1] == -1) {
        to = 0;
      } else {
        to = header_buf[myordering + 1];
      }

      /* send only */
      if (myordering == 0) {
        for (i = 0; i < pipe_length; i++) {
            send_request_array[i]= Request::isend((char *)rbuf + (i * increment), segment, datatype, to, tag, comm);
        }
        Request::waitall((pipe_length), send_request_array, send_status_array);
      }

      /* receive, reduce, and send */
      else {
        from = header_buf[myordering - 1];
        for (i = 0; i < pipe_length; i++) {
          recv_request_array[i]=Request::irecv(tmp_buf + (i * increment), segment, datatype, from, tag, comm);
        }
        for (i = 0; i < pipe_length; i++) {
          Request::wait(&recv_request_array[i], MPI_STATUS_IGNORE);
          if(op!=MPI_OP_NULL) op->apply( tmp_buf + (i * increment), (char *)rbuf + (i * increment),
                         &segment, datatype);
          send_request_array[i]=Request::isend((char *)rbuf + (i * increment), segment, datatype, to, tag, comm);
        }
        Request::waitall((pipe_length), send_request_array, send_status_array);
      }
    }                           /* non-root */

    delete[] send_request_array;
    delete[] recv_request_array;
    delete[] send_status_array;
    delete[] recv_status_array;

    //printf("node %d done\n",rank);
  }                             /* end pipeline */


  /* if root is not zero send root after finished
     this can be modified to make it faster by using logical src, dst.
   */
  if (root != 0) {
    if (rank == 0) {
      Request::send(rbuf, count, datatype, root, tag, comm);
    } else if (rank == root) {
      Request::recv(rbuf, count, datatype, 0, tag, comm, &status);
    }
  }


  /* when count is not divisible by block size, use default BCAST for the remainder */
  if ((remainder != 0) && (count > segment)) {
    reduce__default((char*)buf + (pipe_length * increment), (char*)rbuf + (pipe_length * increment),
                    remainder, datatype, op, root, comm);
  }

  smpi_free_tmp_buffer(tmp_buf);

  return MPI_SUCCESS;
}
}
}
