/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

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

  sb = (int *) malloc(size * sizeof(int));
  if (!sb) {
    perror("can't allocate send buffer");
    fflush(stderr);
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }
  rb = (int *) malloc(size * sizeof(int));
  if (!rb) {
    perror("can't allocate recv buffer");
    fflush(stderr);
    free(sb);
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }
  for (i = 0; i < size; ++i) {
    sb[i] = rank + 1;
    rb[i] = 0;
  }
  status = MPI_Alltoall(sb, 1, MPI_INT, rb, 1, MPI_INT, MPI_COMM_WORLD);

  printf("[%d] rcvbuf=[", rank);
  for (i = 0; i < size; i++)
    printf("%d ", rb[i]);
  printf("]\n");


  if (rank == 0) {
    if (status != 0) {
      printf("all_to_all returned %d\n", status);
      fflush(stdout);
    }
  }
  free(sb);
  free(rb);
  MPI_Finalize();
  return (EXIT_SUCCESS);
}
