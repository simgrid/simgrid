/* Copyright (c) 2013-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"

static int bcast_NTSL_segment_size_in_byte = 8192;

#define HEADER_SIZE 1024
#define MAX_NODE 1024

namespace simgrid{
namespace smpi{
/* Non-topology-specific pipelined linear-bcast function */
int Coll_bcast_arrival_pattern_aware::bcast(void *buf, int count,
                                                MPI_Datatype datatype, int root,
                                                MPI_Comm comm)
{
  int tag = -COLL_TAG_BCAST;
  MPI_Status status;
  MPI_Request request;

  MPI_Status temp_status_array[MAX_NODE];

  int rank, size;
  int i, j;

  int sent_count;
  int header_index;
  int flag_array[MAX_NODE];
  int already_sent[MAX_NODE];
  int to_clean[MAX_NODE];
  int header_buf[HEADER_SIZE];
  char temp_buf[MAX_NODE];

  MPI_Aint extent;
  extent = datatype->get_extent();

  /* destination */
  int to;



  rank = comm->rank();
  size = comm->size();


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

  /* value == 0 means root has not send data (or header) to the node yet */
  for (i = 0; i < MAX_NODE; i++) {
    already_sent[i] = 0;
    to_clean[i] = 0;
  }

  /* when a message is smaller than a block size => no pipeline */
  if (count <= segment) {
    if (rank == 0) {
      sent_count = 0;

      while (sent_count < (size - 1)) {
        for (i = 1; i < size; i++) {
          Request::iprobe(i, MPI_ANY_TAG, comm, &flag_array[i],
                     MPI_STATUSES_IGNORE);
        }

        header_index = 0;
        /* recv 1-byte message */
        for (i = 1; i < size; i++) {

          /* message arrive */
          if ((flag_array[i] == 1) && (already_sent[i] == 0)) {
            Request::recv(temp_buf, 1, MPI_CHAR, i, tag, comm, &status);
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
          Request::send(header_buf, HEADER_SIZE, MPI_INT, to, tag, comm);
          Request::send(buf, count, datatype, to, tag, comm);
        }

        /* randomly MPI_Send to one */
        else {
          /* search for the first node that never received data before */
          for (i = 1; i < size; i++) {
            if (already_sent[i] == 0) {
              header_buf[0] = i;
              header_buf[1] = -1;
              Request::send(header_buf, HEADER_SIZE, MPI_INT, i, tag, comm);
              Request::send(buf, count, datatype, i, tag, comm);
              already_sent[i] = 1;
              sent_count++;
              break;
            }
          }
        }


      }                         /* while loop */
    }

    /* non-root */
    else {

      /* send 1-byte message to root */
      Request::send(temp_buf, 1, MPI_CHAR, 0, tag, comm);

      /* wait for header and data, forward when required */
      Request::recv(header_buf, HEADER_SIZE, MPI_INT, MPI_ANY_SOURCE, tag, comm,
               &status);
      Request::recv(buf, count, datatype, MPI_ANY_SOURCE, tag, comm, &status);

      /* search for where it is */
      int myordering = 0;
      while (rank != header_buf[myordering]) {
        myordering++;
      }

      /* send header followed by data */
      if (header_buf[myordering + 1] != -1) {
        Request::send(header_buf, HEADER_SIZE, MPI_INT, header_buf[myordering + 1],
                 tag, comm);
        Request::send(buf, count, datatype, header_buf[myordering + 1], tag, comm);
      }
    }
  }
  /* pipeline bcast */
  else {
    MPI_Request* send_request_array = new MPI_Request[size + pipe_length];
    MPI_Request* recv_request_array = new MPI_Request[size + pipe_length];
    MPI_Status* send_status_array   = new MPI_Status[size + pipe_length];
    MPI_Status* recv_status_array   = new MPI_Status[size + pipe_length];

    if (rank == 0) {
      //double start2 = MPI_Wtime();
      sent_count = 0;
      //int iteration = 0;
      while (sent_count < (size - 1)) {
        //iteration++;
        //start = MPI_Wtime();
        for (i = 1; i < size; i++) {
          Request::iprobe(i, MPI_ANY_TAG, comm, &flag_array[i],
                     &temp_status_array[i]);
        }
        //total = MPI_Wtime() - start;
        //total *= 1000;
        //printf("Iprobe time = %.2f\n",total);
        header_index = 0;

        MPI_Wtime();
        /* recv 1-byte message */
        for (i = 1; i < size; i++) {
          /* message arrive */
          if ((flag_array[i] == 1) && (already_sent[i] == 0)) {
            Request::recv(&temp_buf[i], 1, MPI_CHAR, i, tag, comm,
                     &status);
            header_buf[header_index] = i;
            header_index++;
            sent_count++;

            /* will send in the next step */
            already_sent[i] = 1;
          }
        }
        //total = MPI_Wtime() - start;
        //total *= 1000;
        //printf("Recv 1-byte time = %.2f\n",total);

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
          to = header_buf[0];

          //start = MPI_Wtime();

          /* send header */
          Request::send(header_buf, HEADER_SIZE, MPI_INT, to, tag, comm);

          //total = MPI_Wtime() - start;
          //total *= 1000;
          //printf("\tSend header to %d time = %.2f\n",to,total);

          //start = MPI_Wtime();

          /* send data - non-pipeline case */

          if (0 == 1) {
            //if (header_index == 1) {
            Request::send(buf, count, datatype, to, tag, comm);
          }


          /* send data - pipeline */
          else {
            for (i = 0; i < pipe_length; i++) {
              Request::send((char *)buf + (i * increment), segment, datatype, to, tag, comm);
            }
            //Request::waitall((pipe_length), send_request_array, send_status_array);
          }
          //total = MPI_Wtime() - start;
          //total *= 1000;
          //printf("\tSend data to %d time = %.2f\n",to,total);

        }



        /* randomly MPI_Send to one node */
        else {
          /* search for the first node that never received data before */
          for (i = 1; i < size; i++) {
            if (already_sent[i] == 0) {
              header_buf[0] = i;
              header_buf[1] = -1;
              to = i;

              //start = MPI_Wtime();
              Request::send(header_buf, HEADER_SIZE, MPI_INT, to, tag, comm);

              /* still need to chop data so that we can use the same non-root code */
              for (j = 0; j < pipe_length; j++) {
                Request::send((char *)buf + (j * increment), segment, datatype, to, tag,
                         comm);
              }

              //Request::send(buf,count,datatype,to,tag,comm);
              //Request::wait(&request,MPI_STATUS_IGNORE);

              //total = MPI_Wtime() - start;
              //total *= 1000;
              //printf("SEND TO SINGLE node %d time = %.2f\n",i,total);


              already_sent[i] = 1;
              to_clean[i]=1;
              sent_count++;
              break;
            }
          }
        }

      }                         /* while loop */

      for(i=0; i<size; i++)
        if(to_clean[i]!=0)Request::recv(&temp_buf[i], 1, MPI_CHAR, i, tag, comm,
                     &status);
      //total = MPI_Wtime() - start2;
      //total *= 1000;
      //printf("Node zero iter = %d time = %.2f\n",iteration,total);
    }

    /* rank 0 */
    /* none root */
    else {
      /* send 1-byte message to root */
      Request::send(temp_buf, 1, MPI_CHAR, 0, tag, comm);

      /* wait for header forward when required */
      request = Request::irecv(header_buf, HEADER_SIZE, MPI_INT, MPI_ANY_SOURCE, tag, comm);
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

      /* receive data */

      if (0 == -1) {
        //if (header_buf[1] == -1) {
        request = Request::irecv(buf, count, datatype, 0, tag, comm);
        Request::wait(&request, MPI_STATUS_IGNORE);
        //printf("\t\tnode %d ordering = %d receive data from root\n",rank,myordering);
      } else {
        for (i = 0; i < pipe_length; i++) {
          recv_request_array[i] = Request::irecv((char *)buf + (i * increment), segment, datatype, MPI_ANY_SOURCE,
                                                 tag, comm);
        }
      }

      /* send data */
      if (header_buf[myordering + 1] != -1) {
        for (i = 0; i < pipe_length; i++) {
          Request::wait(&recv_request_array[i], MPI_STATUS_IGNORE);
          send_request_array[i] = Request::isend((char *)buf + (i * increment), segment, datatype,
                    header_buf[myordering + 1], tag, comm);
        }
        Request::waitall((pipe_length), send_request_array, send_status_array);
      }else{
          Request::waitall(pipe_length, recv_request_array, recv_status_array);
          }

    }

    delete[] send_request_array;
    delete[] recv_request_array;
    delete[] send_status_array;
    delete[] recv_status_array;
  }                             /* end pipeline */

  /* when count is not divisible by block size, use default BCAST for the remainder */
  if ((remainder != 0) && (count > segment)) {
    XBT_WARN("MPI_bcast_arrival_pattern_aware use default MPI_bcast.");
    Colls::bcast((char *)buf + (pipe_length * increment), remainder, datatype, root, comm);
  }

  return MPI_SUCCESS;
}

}
}
