/* Copyright (c) 2009-2010, 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "mpi.h"

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#endif

int main(int argc, char *argv[])
{
  int rank, size;
  int i;
  int *sb;
  int *rb;
  int status;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  int count = 2;
  sb = (int *) xbt_malloc(count * sizeof(int));
  rb = (int *) xbt_malloc(count * size * sizeof(int));
  
  for (i = 0; i < count; ++i)
    sb[i] = rank * count + i;
  for (i = 0; i < count * size; ++i)  
    rb[i] = 0;

  printf("[%d] sndbuf=[", rank);
  for (i = 0; i < count; i++)
    printf("%d ", sb[i]);
  printf("]\n");

  status = MPI_Allgather(sb, count, MPI_INT, rb, count, MPI_INT, MPI_COMM_WORLD);

  printf("[%d] rcvbuf=[", rank);
  for (i = 0; i < count * size; i++)
    printf("%d ", rb[i]);
  printf("]\n");

  if (rank == 0) {
    if (status != MPI_SUCCESS) {
      printf("allgather returned %d\n", status);
      fflush(stdout);
    }
  }
  free(sb);
  free(rb);
  MPI_Finalize();
  return (EXIT_SUCCESS);
}
