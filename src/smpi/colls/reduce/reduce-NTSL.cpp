/* Copyright (c) 2013-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"
//#include <star-reduction.c>

int reduce_NTSL_segment_size_in_byte = 8192;

/* Non-topology-specific pipelined linear-bcast function
   0->1, 1->2 ,2->3, ....., ->last node : in a pipeline fashion
*/
namespace simgrid {
namespace smpi {
int reduce__NTSL(const void *buf, void *rbuf, int count,
                 MPI_Datatype datatype, MPI_Op op, int root,
                 MPI_Comm comm)
{
  int tag = COLL_TAG_REDUCE;
  MPI_Status status;
  int rank, size;
  int i;
  MPI_Aint extent;
  extent = datatype->get_extent();

  rank = comm->rank();
  size = comm->size();

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
     Request::send(buf,count,datatype,0,tag,comm);
     }
     else if (rank == 0) {
     Request::recv(buf,count,datatype,root,tag,comm,&status);
     }
     }
   */

  unsigned char* tmp_buf = smpi_get_tmp_sendbuffer(count * extent);

  Request::sendrecv(buf, count, datatype, rank, tag, rbuf, count, datatype, rank,
               tag, comm, &status);

  /* when a message is smaller than a block size => no pipeline */
  if (count <= segment) {
    if (rank == root) {
      Request::recv(tmp_buf, count, datatype, from, tag, comm, &status);
      if(op!=MPI_OP_NULL) op->apply( tmp_buf, rbuf, &count, datatype);
    } else if (rank == ((root - 1 + size) % size)) {
      Request::send(rbuf, count, datatype, to, tag, comm);
    } else {
      Request::recv(tmp_buf, count, datatype, from, tag, comm, &status);
      if(op!=MPI_OP_NULL) op->apply( tmp_buf, rbuf, &count, datatype);
      Request::send(rbuf, count, datatype, to, tag, comm);
    }
    smpi_free_tmp_buffer(tmp_buf);
    return MPI_SUCCESS;
  }

  /* pipeline */
  else {
    auto* send_request_array = new MPI_Request[size + pipe_length];
    auto* recv_request_array = new MPI_Request[size + pipe_length];
    auto* send_status_array  = new MPI_Status[size + pipe_length];
    auto* recv_status_array  = new MPI_Status[size + pipe_length];

    /* root recv data */
    if (rank == root) {
      for (i = 0; i < pipe_length; i++) {
        recv_request_array[i] = Request::irecv(tmp_buf + (i * increment), segment, datatype, from, (tag + i), comm);
      }
      for (i = 0; i < pipe_length; i++) {
        Request::wait(&recv_request_array[i], &status);
        if(op!=MPI_OP_NULL) op->apply( tmp_buf + (i * increment), (char *)rbuf + (i * increment),
                       &segment, datatype);
      }
    }

    /* last node only sends data */
    else if (rank == ((root - 1 + size) % size)) {
      for (i = 0; i < pipe_length; i++) {
        send_request_array[i] = Request::isend((char *)rbuf + (i * increment), segment, datatype, to, (tag + i),
                  comm);
      }
      Request::waitall((pipe_length), send_request_array, send_status_array);
    }

    /* intermediate nodes relay (receive, reduce, then send) data */
    else {
      for (i = 0; i < pipe_length; i++) {
        recv_request_array[i] = Request::irecv(tmp_buf + (i * increment), segment, datatype, from, (tag + i), comm);
      }
      for (i = 0; i < pipe_length; i++) {
        Request::wait(&recv_request_array[i], &status);
        if(op!=MPI_OP_NULL) op->apply( tmp_buf + (i * increment), (char *)rbuf + (i * increment),
                       &segment, datatype);
        send_request_array[i] = Request::isend((char *) rbuf + (i * increment), segment, datatype, to,
                  (tag + i), comm);
      }
      Request::waitall((pipe_length), send_request_array, send_status_array);
    }

    delete[] send_request_array;
    delete[] recv_request_array;
    delete[] send_status_array;
    delete[] recv_status_array;
  }                             /* end pipeline */

  if ((remainder != 0) && (count > segment)) {
    XBT_INFO("MPI_reduce_NTSL: count is not divisible by block size, use default MPI_reduce for remainder.");
    reduce__default((char *)buf + (pipe_length * increment),
                    (char *)rbuf + (pipe_length * increment), remainder, datatype, op, root,
                    comm);
  }

  smpi_free_tmp_buffer(tmp_buf);
  return MPI_SUCCESS;
}
}
}
