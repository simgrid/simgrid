/* Copyright (c) 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "colls_private.h"
#ifndef NUM_CORE
#define NUM_CORE 8
#endif

int bcast_SMP_binary_segment_byte = 8192;

int smpi_coll_tuned_bcast_SMP_binary(void *buf, int count,
                                     MPI_Datatype datatype, int root,
                                     MPI_Comm comm)
{
  int tag = COLL_TAG_BCAST;
  MPI_Status status;
  MPI_Request request;
  MPI_Request *request_array;
  MPI_Status *status_array;
  int rank, size;
  int i;
  MPI_Aint extent;
  extent = smpi_datatype_get_extent(datatype);

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  int host_num_core = simcall_host_get_core(SIMIX_host_self());
  // do we use the default one or the number of cores in the platform ?
  // if the number of cores is one, the platform may be simulated with 1 node = 1 core
  if (host_num_core == 1) host_num_core = NUM_CORE;

  int segment = bcast_SMP_binary_segment_byte / extent;
  int pipe_length = count / segment;
  int remainder = count % segment;

  int to_intra_left = (rank / host_num_core) * host_num_core + (rank % host_num_core) * 2 + 1;
  int to_intra_right = (rank / host_num_core) * host_num_core + (rank % host_num_core) * 2 + 2;
  int to_inter_left = ((rank / host_num_core) * 2 + 1) * host_num_core;
  int to_inter_right = ((rank / host_num_core) * 2 + 2) * host_num_core;
  int from_inter = (((rank / host_num_core) - 1) / 2) * host_num_core;
  int from_intra = (rank / host_num_core) * host_num_core + ((rank % host_num_core) - 1) / 2;
  int increment = segment * extent;

  int base = (rank / host_num_core) * host_num_core;
  int num_core = host_num_core;
  if (((rank / host_num_core) * host_num_core) == ((size / host_num_core) * host_num_core))
    num_core = size - (rank / host_num_core) * host_num_core;

  // if root is not zero send to rank zero first
  if (root != 0) {
    if (rank == root)
      smpi_mpi_send(buf, count, datatype, 0, tag, comm);
    else if (rank == 0)
      smpi_mpi_recv(buf, count, datatype, root, tag, comm, &status);
  }
  // when a message is smaller than a block size => no pipeline 
  if (count <= segment) {
    // case ROOT-of-each-SMP
    if (rank % host_num_core == 0) {
      // case ROOT
      if (rank == 0) {
        //printf("node %d left %d right %d\n",rank,to_inter_left,to_inter_right);
        if (to_inter_left < size)
          smpi_mpi_send(buf, count, datatype, to_inter_left, tag, comm);
        if (to_inter_right < size)
          smpi_mpi_send(buf, count, datatype, to_inter_right, tag, comm);
        if ((to_intra_left - base) < num_core)
          smpi_mpi_send(buf, count, datatype, to_intra_left, tag, comm);
        if ((to_intra_right - base) < num_core)
          smpi_mpi_send(buf, count, datatype, to_intra_right, tag, comm);
      }
      // case LEAVES ROOT-of-eash-SMP
      else if (to_inter_left >= size) {
        //printf("node %d from %d\n",rank,from_inter);
        request = smpi_mpi_irecv(buf, count, datatype, from_inter, tag, comm);
        smpi_mpi_wait(&request, &status);
        if ((to_intra_left - base) < num_core)
          smpi_mpi_send(buf, count, datatype, to_intra_left, tag, comm);
        if ((to_intra_right - base) < num_core)
          smpi_mpi_send(buf, count, datatype, to_intra_right, tag, comm);
      }
      // case INTERMEDIAT ROOT-of-each-SMP
      else {
        //printf("node %d left %d right %d from %d\n",rank,to_inter_left,to_inter_right,from_inter);
        request = smpi_mpi_irecv(buf, count, datatype, from_inter, tag, comm);
        smpi_mpi_wait(&request, &status);
        smpi_mpi_send(buf, count, datatype, to_inter_left, tag, comm);
        if (to_inter_right < size)
          smpi_mpi_send(buf, count, datatype, to_inter_right, tag, comm);
        if ((to_intra_left - base) < num_core)
          smpi_mpi_send(buf, count, datatype, to_intra_left, tag, comm);
        if ((to_intra_right - base) < num_core)
          smpi_mpi_send(buf, count, datatype, to_intra_right, tag, comm);
      }
    }
    // case non ROOT-of-each-SMP
    else {
      // case leaves
      if ((to_intra_left - base) >= num_core) {
        request = smpi_mpi_irecv(buf, count, datatype, from_intra, tag, comm);
        smpi_mpi_wait(&request, &status);
      }
      // case intermediate
      else {
        request = smpi_mpi_irecv(buf, count, datatype, from_intra, tag, comm);
        smpi_mpi_wait(&request, &status);
        smpi_mpi_send(buf, count, datatype, to_intra_left, tag, comm);
        if ((to_intra_right - base) < num_core)
          smpi_mpi_send(buf, count, datatype, to_intra_right, tag, comm);
      }
    }

    return MPI_SUCCESS;
  }

  // pipeline bcast
  else {
    request_array =
        (MPI_Request *) xbt_malloc((size + pipe_length) * sizeof(MPI_Request));
    status_array =
        (MPI_Status *) xbt_malloc((size + pipe_length) * sizeof(MPI_Status));

    // case ROOT-of-each-SMP
    if (rank % host_num_core == 0) {
      // case ROOT
      if (rank == 0) {
        for (i = 0; i < pipe_length; i++) {
          //printf("node %d left %d right %d\n",rank,to_inter_left,to_inter_right);
          if (to_inter_left < size)
            smpi_mpi_send((char *) buf + (i * increment), segment, datatype,
                     to_inter_left, (tag + i), comm);
          if (to_inter_right < size)
            smpi_mpi_send((char *) buf + (i * increment), segment, datatype,
                     to_inter_right, (tag + i), comm);
          if ((to_intra_left - base) < num_core)
            smpi_mpi_send((char *) buf + (i * increment), segment, datatype,
                     to_intra_left, (tag + i), comm);
          if ((to_intra_right - base) < num_core)
            smpi_mpi_send((char *) buf + (i * increment), segment, datatype,
                     to_intra_right, (tag + i), comm);
        }
      }
      // case LEAVES ROOT-of-eash-SMP
      else if (to_inter_left >= size) {
        //printf("node %d from %d\n",rank,from_inter);
        for (i = 0; i < pipe_length; i++) {
          request_array[i] = smpi_mpi_irecv((char *) buf + (i * increment), segment, datatype,
                    from_inter, (tag + i), comm);
        }
        for (i = 0; i < pipe_length; i++) {
          smpi_mpi_wait(&request_array[i], &status);
          if ((to_intra_left - base) < num_core)
            smpi_mpi_send((char *) buf + (i * increment), segment, datatype,
                     to_intra_left, (tag + i), comm);
          if ((to_intra_right - base) < num_core)
            smpi_mpi_send((char *) buf + (i * increment), segment, datatype,
                     to_intra_right, (tag + i), comm);
        }
      }
      // case INTERMEDIAT ROOT-of-each-SMP
      else {
        //printf("node %d left %d right %d from %d\n",rank,to_inter_left,to_inter_right,from_inter);
        for (i = 0; i < pipe_length; i++) {
          request_array[i] = smpi_mpi_irecv((char *) buf + (i * increment), segment, datatype,
                    from_inter, (tag + i), comm);
        }
        for (i = 0; i < pipe_length; i++) {
          smpi_mpi_wait(&request_array[i], &status);
          smpi_mpi_send((char *) buf + (i * increment), segment, datatype,
                   to_inter_left, (tag + i), comm);
          if (to_inter_right < size)
            smpi_mpi_send((char *) buf + (i * increment), segment, datatype,
                     to_inter_right, (tag + i), comm);
          if ((to_intra_left - base) < num_core)
            smpi_mpi_send((char *) buf + (i * increment), segment, datatype,
                     to_intra_left, (tag + i), comm);
          if ((to_intra_right - base) < num_core)
            smpi_mpi_send((char *) buf + (i * increment), segment, datatype,
                     to_intra_right, (tag + i), comm);
        }
      }
    }
    // case non-ROOT-of-each-SMP
    else {
      // case leaves
      if ((to_intra_left - base) >= num_core) {
        for (i = 0; i < pipe_length; i++) {
          request_array[i] = smpi_mpi_irecv((char *) buf + (i * increment), segment, datatype,
                    from_intra, (tag + i), comm);
        }
        smpi_mpi_waitall((pipe_length), request_array, status_array);
      }
      // case intermediate
      else {
        for (i = 0; i < pipe_length; i++) {
          request_array[i] = smpi_mpi_irecv((char *) buf + (i * increment), segment, datatype,
                    from_intra, (tag + i), comm);
        }
        for (i = 0; i < pipe_length; i++) {
          smpi_mpi_wait(&request_array[i], &status);
          smpi_mpi_send((char *) buf + (i * increment), segment, datatype,
                   to_intra_left, (tag + i), comm);
          if ((to_intra_right - base) < num_core)
            smpi_mpi_send((char *) buf + (i * increment), segment, datatype,
                     to_intra_right, (tag + i), comm);
        }
      }
    }

    free(request_array);
    free(status_array);
  }

  // when count is not divisible by block size, use default BCAST for the remainder
  if ((remainder != 0) && (count > segment)) {
    XBT_WARN("MPI_bcast_SMP_binary use default MPI_bcast.");	  
    smpi_mpi_bcast((char *) buf + (pipe_length * increment), remainder, datatype,
              root, comm);
  }

  return 1;
}
