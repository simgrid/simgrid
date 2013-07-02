#include "colls_private.h"

int bcast_NTSB_segment_size_in_byte = 8192;

int smpi_coll_tuned_bcast_NTSB(void *buf, int count, MPI_Datatype datatype,
                               int root, MPI_Comm comm)
{
  int tag = COLL_TAG_BCAST;
  MPI_Status status;
  int rank, size;
  int i;

  MPI_Request *send_request_array;
  MPI_Request *recv_request_array;
  MPI_Status *send_status_array;
  MPI_Status *recv_status_array;

  MPI_Aint extent;
  extent = smpi_datatype_get_extent(datatype);

  rank = smpi_comm_rank(MPI_COMM_WORLD);
  size = smpi_comm_size(MPI_COMM_WORLD);

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
      smpi_mpi_send(buf, count, datatype, 0, tag, comm);
    } else if (rank == 0) {
      smpi_mpi_recv(buf, count, datatype, root, tag, comm, &status);
    }
  }

  /* when a message is smaller than a block size => no pipeline */
  if (count <= segment) {

    /* case: root */
    if (rank == 0) {
      /* case root has only a left child */
      if (to_right == -1) {
        smpi_mpi_send(buf, count, datatype, to_left, tag, comm);
      }
      /* case root has both left and right children */
      else {
        smpi_mpi_send(buf, count, datatype, to_left, tag, comm);
        smpi_mpi_send(buf, count, datatype, to_right, tag, comm);
      }
    }

    /* case: leaf ==> receive only */
    else if (to_left == -1) {
      smpi_mpi_recv(buf, count, datatype, from, tag, comm, &status);
    }

    /* case: intermidiate node with only left child ==> relay message */
    else if (to_right == -1) {
      smpi_mpi_recv(buf, count, datatype, from, tag, comm, &status);
      smpi_mpi_send(buf, count, datatype, to_left, tag, comm);
    }

    /* case: intermidiate node with both left and right children ==> relay message */
    else {
      smpi_mpi_recv(buf, count, datatype, from, tag, comm, &status);
      smpi_mpi_send(buf, count, datatype, to_left, tag, comm);
      smpi_mpi_send(buf, count, datatype, to_right, tag, comm);
    }
    return MPI_SUCCESS;
  }
  // pipelining
  else {

    send_request_array =
        (MPI_Request *) xbt_malloc(2 * (size + pipe_length) * sizeof(MPI_Request));
    recv_request_array =
        (MPI_Request *) xbt_malloc((size + pipe_length) * sizeof(MPI_Request));
    send_status_array =
        (MPI_Status *) xbt_malloc(2 * (size + pipe_length) * sizeof(MPI_Status));
    recv_status_array =
        (MPI_Status *) xbt_malloc((size + pipe_length) * sizeof(MPI_Status));



    /* case: root */
    if (rank == 0) {
      /* case root has only a left child */
      if (to_right == -1) {
        for (i = 0; i < pipe_length; i++) {
          send_request_array[i] = smpi_mpi_isend((char *) buf + (i * increment), segment, datatype, to_left,
                    tag + i, comm);
        }
        smpi_mpi_waitall((pipe_length), send_request_array, send_status_array);
      }
      /* case root has both left and right children */
      else {
        for (i = 0; i < pipe_length; i++) {
          send_request_array[i] = smpi_mpi_isend((char *) buf + (i * increment), segment, datatype, to_left,
                    tag + i, comm);
          send_request_array[i + pipe_length] = smpi_mpi_isend((char *) buf + (i * increment), segment, datatype, to_right,
                    tag + i, comm);
        }
        smpi_mpi_waitall((2 * pipe_length), send_request_array, send_status_array);
      }
    }

    /* case: leaf ==> receive only */
    else if (to_left == -1) {
      for (i = 0; i < pipe_length; i++) {
        recv_request_array[i] = smpi_mpi_irecv((char *) buf + (i * increment), segment, datatype, from,
                  tag + i, comm);
      }
      smpi_mpi_waitall((pipe_length), recv_request_array, recv_status_array);
    }

    /* case: intermidiate node with only left child ==> relay message */
    else if (to_right == -1) {
      for (i = 0; i < pipe_length; i++) {
        recv_request_array[i] = smpi_mpi_irecv((char *) buf + (i * increment), segment, datatype, from,
                  tag + i, comm);
      }
      for (i = 0; i < pipe_length; i++) {
        smpi_mpi_wait(&recv_request_array[i], &status);
        send_request_array[i] = smpi_mpi_isend((char *) buf + (i * increment), segment, datatype, to_left,
                  tag + i, comm);
      }
      smpi_mpi_waitall(pipe_length, send_request_array, send_status_array);

    }
    /* case: intermidiate node with both left and right children ==> relay message */
    else {
      for (i = 0; i < pipe_length; i++) {
        recv_request_array[i] = smpi_mpi_irecv((char *) buf + (i * increment), segment, datatype, from,
                  tag + i, comm);
      }
      for (i = 0; i < pipe_length; i++) {
        smpi_mpi_wait(&recv_request_array[i], &status);
        send_request_array[i] = smpi_mpi_isend((char *) buf + (i * increment), segment, datatype, to_left,
                  tag + i, comm);
        send_request_array[i + pipe_length] = smpi_mpi_isend((char *) buf + (i * increment), segment, datatype, to_right,
                  tag + i, comm);
      }
      smpi_mpi_waitall((2 * pipe_length), send_request_array, send_status_array);
    }

    free(send_request_array);
    free(recv_request_array);
    free(send_status_array);
    free(recv_status_array);
  }                             /* end pipeline */

  /* when count is not divisible by block size, use default BCAST for the remainder */
  if ((remainder != 0) && (count > segment)) {
    XBT_WARN("MPI_bcast_NTSB use default MPI_bcast.");	  	  
    smpi_mpi_bcast((char *) buf + (pipe_length * increment), remainder, datatype,
              root, comm);
  }

  return MPI_SUCCESS;
}
