#include "colls_private.h"
//#include <star-reduction.c>

int reduce_NTSL_segment_size_in_byte = 8192;

/* Non-topology-specific pipelined linear-bcast function 
   0->1, 1->2 ,2->3, ....., ->last node : in a pipeline fashion
*/
int smpi_coll_tuned_reduce_NTSL(void *buf, void *rbuf, int count,
                                MPI_Datatype datatype, MPI_Op op, int root,
                                MPI_Comm comm)
{
  int tag = COLL_TAG_REDUCE;
  MPI_Status status;
  MPI_Request *send_request_array;
  MPI_Request *recv_request_array;
  MPI_Status *send_status_array;
  MPI_Status *recv_status_array;
  int rank, size;
  int i;
  MPI_Aint extent;
  extent = smpi_datatype_get_extent(datatype);

  rank = smpi_comm_rank(MPI_COMM_WORLD);
  size = smpi_comm_size(MPI_COMM_WORLD);

  /* source node and destination nodes (same through out the functions) */
  int to = (rank - 1 + size) % size;
  int from = (rank + 1) % size;

  /* segment is segment size in number of elements (not bytes) */
  int segment = reduce_NTSL_segment_size_in_byte / extent;

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

  /*
     if (root != 0) {
     if (rank == root){
     smpi_mpi_send(buf,count,datatype,0,tag,comm);
     }
     else if (rank == 0) {
     smpi_mpi_recv(buf,count,datatype,root,tag,comm,&status);        
     }
     }
   */

  char *tmp_buf;
  tmp_buf = (char *) xbt_malloc(count * extent);

  smpi_mpi_sendrecv(buf, count, datatype, rank, tag, rbuf, count, datatype, rank,
               tag, comm, &status);

  /* when a message is smaller than a block size => no pipeline */
  if (count <= segment) {
    if (rank == root) {
      smpi_mpi_recv(tmp_buf, count, datatype, from, tag, comm, &status);
      smpi_op_apply(op, tmp_buf, rbuf, &count, &datatype);
    } else if (rank == ((root - 1 + size) % size)) {
      smpi_mpi_send(rbuf, count, datatype, to, tag, comm);
    } else {
      smpi_mpi_recv(tmp_buf, count, datatype, from, tag, comm, &status);
      smpi_op_apply(op, tmp_buf, rbuf, &count, &datatype);
      smpi_mpi_send(rbuf, count, datatype, to, tag, comm);
    }
    free(tmp_buf);
    return MPI_SUCCESS;
  }

  /* pipeline */
  else {
    send_request_array =
        (MPI_Request *) xbt_malloc((size + pipe_length) * sizeof(MPI_Request));
    recv_request_array =
        (MPI_Request *) xbt_malloc((size + pipe_length) * sizeof(MPI_Request));
    send_status_array =
        (MPI_Status *) xbt_malloc((size + pipe_length) * sizeof(MPI_Status));
    recv_status_array =
        (MPI_Status *) xbt_malloc((size + pipe_length) * sizeof(MPI_Status));

    /* root recv data */
    if (rank == root) {
      for (i = 0; i < pipe_length; i++) {
        recv_request_array[i] = smpi_mpi_irecv((char *) tmp_buf + (i * increment), segment, datatype, from,
                  (tag + i), comm);
      }
      for (i = 0; i < pipe_length; i++) {
        smpi_mpi_wait(&recv_request_array[i], &status);
        smpi_op_apply(op, tmp_buf + (i * increment), (char *)rbuf + (i * increment),
                       &segment, &datatype);
      }
    }

    /* last node only sends data */
    else if (rank == ((root - 1 + size) % size)) {
      for (i = 0; i < pipe_length; i++) {
        send_request_array[i] = smpi_mpi_isend((char *)rbuf + (i * increment), segment, datatype, to, (tag + i),
                  comm);
      }
      smpi_mpi_waitall((pipe_length), send_request_array, send_status_array);
    }

    /* intermediate nodes relay (receive, reduce, then send) data */
    else {
      for (i = 0; i < pipe_length; i++) {
        recv_request_array[i] = smpi_mpi_irecv((char *) tmp_buf + (i * increment), segment, datatype, from,
                  (tag + i), comm);
      }
      for (i = 0; i < pipe_length; i++) {
        smpi_mpi_wait(&recv_request_array[i], &status);
        smpi_op_apply(op, tmp_buf + (i * increment), (char *)rbuf + (i * increment),
                       &segment, &datatype);
        send_request_array[i] = smpi_mpi_isend((char *) rbuf + (i * increment), segment, datatype, to,
                  (tag + i), comm);
      }
      smpi_mpi_waitall((pipe_length), send_request_array, send_status_array);
    }

    free(send_request_array);
    free(recv_request_array);
    free(send_status_array);
    free(recv_status_array);
  }                             /* end pipeline */

  /* when count is not divisible by block size, use default BCAST for the remainder */
  if ((remainder != 0) && (count > segment)) {
    XBT_WARN("MPI_reduce_NTSL use default MPI_reduce.");	  
    smpi_mpi_reduce((char *)buf + (pipe_length * increment),
               (char *)rbuf + (pipe_length * increment), remainder, datatype, op, root,
               comm);
  }

  free(tmp_buf);
  return MPI_SUCCESS;
}
