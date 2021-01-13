/* Copyright (c) 2009-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "mpi.h"

int main(int argc, char *argv[])
{
  int i;
  int rank;
  int size;
  int status;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

  int* recv_counts = (int *) xbt_malloc(size * sizeof(int));
  int* recv_disps = (int *) xbt_malloc(size * sizeof(int));

  int recv_sb_size = 0;
  for (i = 0; i < size; i++) {
    recv_counts[i] = i + 1;
    recv_disps[i] = recv_sb_size;
    recv_sb_size += i + 1;
  }

  int* sb = (int *) xbt_malloc(recv_counts[rank] * sizeof(int));
  int* rb = (int *) xbt_malloc(recv_sb_size * sizeof(int));

  status = MPI_Allgatherv(NULL, recv_counts[rank], MPI_INT, rb, recv_counts, recv_disps, MPI_INT, MPI_COMM_WORLD);
  if(status!=MPI_ERR_BUFFER)
    printf("MPI_Allgatherv did not return MPI_ERR_BUFFER for empty sendbuf\n");
  status = MPI_Allgatherv(sb, -1, MPI_INT, rb, recv_counts, recv_disps, MPI_INT, MPI_COMM_WORLD);
  if(status!=MPI_ERR_COUNT)
    printf("MPI_Allgatherv did not return MPI_ERR_COUNT for -1 sendcount\n");
  status = MPI_Allgatherv(sb, recv_counts[rank], MPI_DATATYPE_NULL, rb, recv_counts, recv_disps, MPI_INT, MPI_COMM_WORLD);
  if(status!=MPI_ERR_TYPE)
    printf("MPI_Allgatherv did not return MPI_ERR_TYPE for MPI_DATATYPE_NULL sendtype\n");
  status = MPI_Allgatherv(sb, recv_counts[rank], MPI_INT, NULL, recv_counts, recv_disps, MPI_INT, MPI_COMM_WORLD);
  if(status!=MPI_ERR_BUFFER)
    printf("MPI_Allgatherv did not return MPI_ERR_BUFFER for empty recvbuf\n");
  status = MPI_Allgatherv(sb, recv_counts[rank], MPI_INT, rb, NULL, recv_disps, MPI_INT, MPI_COMM_WORLD);
  if(status!=MPI_ERR_COUNT)
    printf("MPI_Allgatherv did not return MPI_ERR_COUNT for NULL recvcounts\n");
  status = MPI_Allgatherv(sb, recv_counts[rank], MPI_INT, rb, recv_counts, NULL, MPI_INT, MPI_COMM_WORLD);
  if(status!=MPI_ERR_ARG)
    printf("MPI_Allgatherv did not return MPI_ERR_ARG for NULL recvdisps\n");
  status = MPI_Allgatherv(sb, recv_counts[rank], MPI_INT, rb, recv_counts, recv_disps, MPI_DATATYPE_NULL, MPI_COMM_WORLD);
  if(status!=MPI_ERR_TYPE)
    printf("MPI_Allgatherv did not return MPI_ERR_TYPE for MPI_DATATYPE_NULL recvtype\n");
  status = MPI_Allgatherv(sb, recv_counts[rank], MPI_INT, rb, recv_counts, recv_disps, MPI_INT, MPI_COMM_NULL);
  if(status!=MPI_ERR_COMM)
    printf("MPI_Allgatherv did not return MPI_ERR_COMM for MPI_COMM_NULL comm\n");

  printf("[%d] sndbuf=[", rank);
  for (i = 0; i < recv_counts[rank]; i++){
    sb[i] = recv_disps[rank] + i;
    printf("%d ", sb[i]);
  }
  printf("]\n");

  for (i = 0; i < recv_sb_size; i++)
    rb[i] = -1;

  status = MPI_Allgatherv(sb, recv_counts[rank], MPI_INT, rb, recv_counts, recv_disps, MPI_INT, MPI_COMM_WORLD);

  printf("[%d] rcvbuf=[", rank);
  for (i = 0; i < recv_sb_size; i++)
    printf("%d ", rb[i]);
  printf("]\n");

  if (rank == 0 && status != MPI_SUCCESS) {
    printf("allgatherv returned %d\n", status);
    fflush(stdout);
  }
  xbt_free(sb);
  xbt_free(rb);
  xbt_free(recv_counts);
  xbt_free(recv_disps);
  MPI_Finalize();
  return (EXIT_SUCCESS);
}
