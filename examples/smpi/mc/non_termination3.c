/* Copyright (c) 2015-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <mpi.h>
#include <simgrid/modelchecker.h>

int x = 0;
int y = 0;

int main(int argc, char **argv) {
  int recv_x;
  int recv_y;
  int size;
  int rank;
  MPI_Status status;

  MPI_Init(&argc, &argv);

  MPI_Comm_size(MPI_COMM_WORLD, &size);   /* Get nr of tasks */
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);   /* Get id of this process */

  MC_ignore(&status.count, sizeof status.count);

  if (rank == 0) {
    while (x<5) {
      MPI_Recv(&recv_x, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      MPI_Recv(&recv_y, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    }
  } else {
    while (x<5) {
      int old_x = x;
      x = old_x - y;
      MPI_Send(&x, 1, MPI_INT, 0, 42, MPI_COMM_WORLD);
      y = old_x + y;
      MPI_Send(&y, 1, MPI_INT, 0, 42, MPI_COMM_WORLD);
    }
  }

  MPI_Finalize();

  return 0;
}
