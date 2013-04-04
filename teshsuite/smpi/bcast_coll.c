/* Copyright (c) 2009. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <mpi.h>

int main(int argc, char **argv)
{
  int size, rank;
  int value = 3;
  int status;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (0 == rank) {
    value = 17;
  }
  status = MPI_Bcast(&value, 1, MPI_INT, 0, MPI_COMM_WORLD);
  printf("node %d has value %d after broadcast\n", rank, value);

  MPI_Barrier(MPI_COMM_WORLD);

  if (rank == 0) {
    if (status != MPI_SUCCESS) {
      printf("bcast returned %d\n", status);
      fflush(stdout);
    }
  }

  MPI_Finalize();
  return 0;
}
