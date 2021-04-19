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
  int rank;
  int size;

  MPI_Init(&argc, &argv);
  int maxlen = argc >= 2 ? atoi(argv[1]) : 1;

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm dup;
  MPI_Comm_dup(MPI_COMM_WORLD, &dup);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_set_errhandler(dup, MPI_ERRORS_RETURN);

  int* sb = (int*)calloc(size * maxlen, sizeof(int));
  int* rb = (int*)calloc(size * maxlen, sizeof(int));

  for (int i = 0; i < size * maxlen; ++i) {
    sb[i] = rank*size + i;
    rb[i] = 0;
  }

  int status = MPI_Allreduce(sb, rb, size * maxlen, MPI_INT, MPI_SUM, dup);

  if (rank == 0 && status != MPI_SUCCESS) {
    printf("all_to_all returned %d\n", status);
    fflush(stdout);
  }
  //Do not free dup and rb
  free(sb);
  MPI_Finalize();
  return (EXIT_SUCCESS);
}
