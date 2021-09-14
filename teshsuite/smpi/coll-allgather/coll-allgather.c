/* Copyright (c) 2009-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "mpi.h"

int main(int argc, char *argv[])
{
  int rank;
  int size;
  int status;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

  int count = 2;
  int* sb = (int *) xbt_malloc(count * sizeof(int));
  int* rb = (int *) xbt_malloc(count * size * sizeof(int));

  for (int i = 0; i < count; ++i)
    sb[i] = rank * count + i;
  for (int i = 0; i < count * size; ++i)
    rb[i] = 0;

  status = MPI_Allgather(NULL, count, MPI_INT, rb, count, MPI_INT, MPI_COMM_WORLD);
  if(status!=MPI_ERR_BUFFER)
    printf("MPI_Allgather did not return MPI_ERR_BUFFER for empty sendbuf\n");
  status = MPI_Allgather(sb, -1, MPI_INT, rb, count, MPI_INT, MPI_COMM_WORLD);
  if(status!=MPI_ERR_COUNT)
    printf("MPI_Allgather did not return MPI_ERR_COUNT for -1 sendcount\n");
  status = MPI_Allgather(sb, count, MPI_DATATYPE_NULL, rb, count, MPI_INT, MPI_COMM_WORLD);
  if(status!=MPI_ERR_TYPE)
    printf("MPI_Allgather did not return MPI_ERR_TYPE for MPI_DATATYPE_NULL sendtype\n");
  status = MPI_Allgather(sb, count, MPI_INT, NULL, count, MPI_INT, MPI_COMM_WORLD);
  if(status!=MPI_ERR_BUFFER)
    printf("MPI_Allgather did not return MPI_ERR_BUFFER for empty recvbuf\n");
  status = MPI_Allgather(sb, count, MPI_INT, rb, -1, MPI_INT, MPI_COMM_WORLD);
  if(status!=MPI_ERR_COUNT)
    printf("MPI_Allgather did not return MPI_ERR_COUNT for -1 recvcount\n");
  status = MPI_Allgather(sb, count, MPI_INT, rb, count, MPI_DATATYPE_NULL, MPI_COMM_WORLD);
  if(status!=MPI_ERR_TYPE)
    printf("MPI_Allgather did not return MPI_ERR_TYPE for MPI_DATATYPE_NULL recvtype\n");
  status = MPI_Allgather(sb, count, MPI_INT, rb, count, MPI_INT, MPI_COMM_NULL);
  if(status!=MPI_ERR_COMM)
    printf("MPI_Allgather did not return MPI_ERR_COMM for MPI_COMM_NULL comm\n");

  printf("[%d] sndbuf=[", rank);
  for (int i = 0; i < count; i++)
    printf("%d ", sb[i]);
  printf("]\n");

  status = MPI_Allgather(sb, count, MPI_INT, rb, count, MPI_INT, MPI_COMM_WORLD);

  printf("[%d] rcvbuf=[", rank);
  for (int i = 0; i < count * size; i++)
    printf("%d ", rb[i]);
  printf("]\n");

  if (rank == 0 && status != MPI_SUCCESS) {
    printf("allgather returned %d\n", status);
    fflush(stdout);
  }
  xbt_free(sb);
  xbt_free(rb);
  MPI_Finalize();
  return EXIT_SUCCESS;
}
