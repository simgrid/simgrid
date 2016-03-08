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

  sb = (int *) xbt_malloc(size * sizeof(int));
  rb = (int *) xbt_malloc(size * sizeof(int));

  for (i = 0; i < size; ++i) {
    sb[i] = rank*size + i;
    rb[i] = 0;
  }
  printf("[%d] sndbuf=[", rank);
  for (i = 0; i < size; i++)
    printf("%d ", sb[i]);
  printf("]\n");

  int root=0;
  status = MPI_Reduce(sb, rb, size, MPI_INT, MPI_SUM, root, MPI_COMM_WORLD);
  MPI_Barrier(MPI_COMM_WORLD);

  if (rank == root) {
    printf("[%d] rcvbuf=[", rank);
    for (i = 0; i < size; i++)
      printf("%d ", rb[i]);
    printf("]\n");
    if (status != MPI_SUCCESS) {
      printf("all_to_all returned %d\n", status);
      fflush(stdout);
    }
  }

  printf("[%d] second sndbuf=[", rank);
  for (i = 0; i < 1; i++)
    printf("%d ", sb[i]);
  printf("]\n");

  root=size-1;
  status = MPI_Reduce(sb, rb, 1, MPI_INT, MPI_PROD, root, MPI_COMM_WORLD);
  MPI_Barrier(MPI_COMM_WORLD);

  if (rank == root) {
    printf("[%d] rcvbuf=[", rank);
    for (i = 0; i < 1; i++)
      printf("%d ", rb[i]);
    printf("]\n");
    if (status != MPI_SUCCESS) {
      printf("all_to_all returned %d\n", status);
      fflush(stdout);
    }
  }
  free(sb);
  free(rb);
  MPI_Finalize();
  return (EXIT_SUCCESS);
}
