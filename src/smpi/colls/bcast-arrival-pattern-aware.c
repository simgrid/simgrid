#include "colls.h"

static int bcast_NTSL_segment_size_in_byte = 8192;

#define HEADER_SIZE 1024
#define MAX_NODE 1024

/* Non-topology-specific pipelined linear-bcast function */
int smpi_coll_tuned_bcast_arrival_pattern_aware(void *buf, int count,
                                                MPI_Datatype datatype, int root,
                                                MPI_Comm comm)
{
  int tag = 50;
  MPI_Status status;
  MPI_Request request;
  MPI_Request *send_request_array;
  MPI_Request *recv_request_array;
  MPI_Status *send_status_array;
  MPI_Status *recv_status_array;

  MPI_Status temp_status_array[MAX_NODE];

  int rank, size;
  int i, j;

  int sent_count;
  int header_index;
  int flag_array[MAX_NODE];
  int already_sent[MAX_NODE];

  int header_buf[HEADER_SIZE];
  char temp_buf[MAX_NODE];

  MPI_Aint extent;
  MPI_Type_extent(datatype, &extent);

  /* destination */
  int to;



  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);


  /* segment is segment size in number of elements (not bytes) */
  int segment = bcast_NTSL_segment_size_in_byte / extent;

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
      MPI_Send(buf, count, datatype, 0, tag, comm);
    } else if (rank == 0) {
      MPI_Recv(buf, count, datatype, root, tag, comm, &status);
    }
  }

  /* value == 0 means root has not send data (or header) to the node yet */
  for (i = 0; i < MAX_NODE; i++) {
    already_sent[i] = 0;
  }

  /* when a message is smaller than a block size => no pipeline */
  if (count <= segment) {
    if (rank == 0) {
      sent_count = 0;

      while (sent_count < (size - 1)) {
        for (i = 1; i < size; i++) {
          MPI_Iprobe(i, MPI_ANY_TAG, MPI_COMM_WORLD, &flag_array[i],
                     MPI_STATUSES_IGNORE);
        }

        header_index = 0;
        /* recv 1-byte message */
        for (i = 1; i < size; i++) {

          /* message arrive */
          if ((flag_array[i] == 1) && (already_sent[i] == 0)) {
            MPI_Recv(temp_buf, 1, MPI_CHAR, i, tag, MPI_COMM_WORLD, &status);
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
          MPI_Send(header_buf, HEADER_SIZE, MPI_INT, to, tag, comm);
          MPI_Send(buf, count, datatype, to, tag, comm);
        }

        /* randomly MPI_Send to one */
        else {
          /* search for the first node that never received data before */
          for (i = 1; i < size; i++) {
            if (already_sent[i] == 0) {
              header_buf[0] = i;
              header_buf[1] = -1;
              MPI_Send(header_buf, HEADER_SIZE, MPI_INT, i, tag, comm);
              MPI_Send(buf, count, datatype, i, tag, comm);
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
      MPI_Send(temp_buf, 1, MPI_CHAR, 0, tag, comm);

      /* wait for header and data, forward when required */
      MPI_Recv(header_buf, HEADER_SIZE, MPI_INT, MPI_ANY_SOURCE, tag, comm,
               &status);
      MPI_Recv(buf, count, datatype, MPI_ANY_SOURCE, tag, comm, &status);

      /* search for where it is */
      int myordering = 0;
      while (rank != header_buf[myordering]) {
        myordering++;
      }

      /* send header followed by data */
      if (header_buf[myordering + 1] != -1) {
        MPI_Send(header_buf, HEADER_SIZE, MPI_INT, header_buf[myordering + 1],
                 tag, comm);
        MPI_Send(buf, count, datatype, header_buf[myordering + 1], tag, comm);
      }
    }
  }
  /* pipeline bcast */
  else {
    send_request_array =
        (MPI_Request *) malloc((size + pipe_length) * sizeof(MPI_Request));
    recv_request_array =
        (MPI_Request *) malloc((size + pipe_length) * sizeof(MPI_Request));
    send_status_array =
        (MPI_Status *) malloc((size + pipe_length) * sizeof(MPI_Status));
    recv_status_array =
        (MPI_Status *) malloc((size + pipe_length) * sizeof(MPI_Status));

    if (rank == 0) {
      //double start2 = MPI_Wtime();
      sent_count = 0;
      //int iteration = 0;
      while (sent_count < (size - 1)) {
        //iteration++;
        //start = MPI_Wtime();
        for (i = 1; i < size; i++) {
          MPI_Iprobe(i, MPI_ANY_TAG, MPI_COMM_WORLD, &flag_array[i],
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
            MPI_Recv(&temp_buf[i], 1, MPI_CHAR, i, tag, MPI_COMM_WORLD,
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
          MPI_Send(header_buf, HEADER_SIZE, MPI_INT, to, tag, comm);

          //total = MPI_Wtime() - start;
          //total *= 1000;
          //printf("\tSend header to %d time = %.2f\n",to,total);

          //start = MPI_Wtime();

          /* send data - non-pipeline case */

          if (0 == 1) {
            //if (header_index == 1) {
            MPI_Send(buf, count, datatype, to, tag, comm);
          }


          /* send data - pipeline */
          else {
            for (i = 0; i < pipe_length; i++) {
              MPI_Send((char *)buf + (i * increment), segment, datatype, to, tag, comm);
            }
            //MPI_Waitall((pipe_length), send_request_array, send_status_array);
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
              MPI_Send(header_buf, HEADER_SIZE, MPI_INT, to, tag, comm);

              /* still need to chop data so that we can use the same non-root code */
              for (j = 0; j < pipe_length; j++) {
                MPI_Send((char *)buf + (j * increment), segment, datatype, to, tag,
                         comm);
              }

              //MPI_Send(buf,count,datatype,to,tag,comm);
              //MPI_Wait(&request,MPI_STATUS_IGNORE);

              //total = MPI_Wtime() - start;
              //total *= 1000;
              //printf("SEND TO SINGLE node %d time = %.2f\n",i,total);


              already_sent[i] = 1;
              sent_count++;
              break;
            }
          }
        }

      }                         /* while loop */

      //total = MPI_Wtime() - start2;
      //total *= 1000;
      //printf("Node zero iter = %d time = %.2f\n",iteration,total);
    }

    /* rank 0 */
    /* none root */
    else {
      /* send 1-byte message to root */
      MPI_Send(temp_buf, 1, MPI_CHAR, 0, tag, comm);

      /* wait for header forward when required */
      MPI_Irecv(header_buf, HEADER_SIZE, MPI_INT, MPI_ANY_SOURCE, tag, comm,
                &request);
      MPI_Wait(&request, MPI_STATUS_IGNORE);

      /* search for where it is */
      int myordering = 0;
      while (rank != header_buf[myordering]) {
        myordering++;
      }

      /* send header when required */
      if (header_buf[myordering + 1] != -1) {
        MPI_Send(header_buf, HEADER_SIZE, MPI_INT, header_buf[myordering + 1],
                 tag, comm);
      }

      /* receive data */

      if (0 == -1) {
        //if (header_buf[1] == -1) {
        MPI_Irecv(buf, count, datatype, 0, tag, comm, &request);
        MPI_Wait(&request, MPI_STATUS_IGNORE);
        //printf("\t\tnode %d ordering = %d receive data from root\n",rank,myordering);
      } else {
        for (i = 0; i < pipe_length; i++) {
          MPI_Irecv((char *)buf + (i * increment), segment, datatype, MPI_ANY_SOURCE,
                    tag, comm, &recv_request_array[i]);
        }
      }

      /* send data */
      if (header_buf[myordering + 1] != -1) {
        for (i = 0; i < pipe_length; i++) {
          MPI_Wait(&recv_request_array[i], MPI_STATUS_IGNORE);
          MPI_Isend((char *)buf + (i * increment), segment, datatype,
                    header_buf[myordering + 1], tag, comm,
                    &send_request_array[i]);
        }
        MPI_Waitall((pipe_length), send_request_array, send_status_array);
      }

    }

    free(send_request_array);
    free(recv_request_array);
    free(send_status_array);
    free(recv_status_array);
  }                             /* end pipeline */

  /* when count is not divisible by block size, use default BCAST for the remainder */
  if ((remainder != 0) && (count > segment)) {
    MPI_Bcast((char *)buf + (pipe_length * increment), remainder, datatype, root, comm);
  }

  return MPI_SUCCESS;
}
