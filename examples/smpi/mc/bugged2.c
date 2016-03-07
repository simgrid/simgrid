/* A simple bugged MPI_ISend and MPI_IRecv test */

/* Copyright (c) 2009, 2011, 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <mpi.h>
#include <simgrid/modelchecker.h>

int main(int argc, char **argv)
{
  int x,y, err, size, rank;
  MPI_Status status;

  /* Initialize MPI */
  err = MPI_Init(&argc, &argv);
  if (err != MPI_SUCCESS) {
    printf("MPI initialization failed!\n");
    exit(1);
  }

  MPI_Comm_size(MPI_COMM_WORLD, &size);   /* Get nr of tasks */
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);   /* Get id of this process */
  if (size % 3 != 0) {
    printf("run this program with exactly 3*N processes \n");
    MPI_Finalize();
    exit(0);
  }

  if (rank % 3 == 0) {
    MPI_Recv(&x, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    printf("(%d) x <- %d\n", rank, x);
    MPI_Recv(&y, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    printf("(%d) y <- %d\n", rank, y);
  }else{
    MPI_Send(&rank, 1, MPI_INT, (rank / 3) * 3, 42, MPI_COMM_WORLD);
    printf("Sent %d to rank %d\n", rank, (rank / 3) * 3);
  }

  MPI_Finalize();

  return 0;
}
