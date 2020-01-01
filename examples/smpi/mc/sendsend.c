/* A simple bugged MPI_Send and MPI_Recv test: it deadlocks when MPI_Send are blocking */

/* Copyright (c) 2009-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv)
{
  int size;
  int rank;
  MPI_Status status;

  /* Initialize MPI */
  int err = MPI_Init(&argc, &argv);
  if (err != MPI_SUCCESS) {
    printf("MPI initialization failed!\n");
    exit(1);
  }

  MPI_Comm_size(MPI_COMM_WORLD, &size); /* Get nr of tasks */
  MPI_Comm_rank(MPI_COMM_WORLD, &rank); /* Get id of this process */
  if (size < 2) {
    printf("run this program with exactly 2 processes (-np 2)\n");
    MPI_Finalize();
    exit(0);
  }

  int recv_buff;
  if (rank == 0) {
    MPI_Send(&rank, 1, MPI_INT, 1, 42, MPI_COMM_WORLD);
    printf("Sent %d to rank 1\n", rank);
    MPI_Recv(&recv_buff, 1, MPI_INT, 1, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    printf("rank 0 recv the data\n");
  } else {
    MPI_Send(&rank, 1, MPI_INT, 0, 42, MPI_COMM_WORLD);
    printf("Sent %d to rank 0\n", rank);
    MPI_Recv(&recv_buff, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    printf("rank 1 recv the data\n");
  }

  MPI_Finalize();

  return 0;
}
