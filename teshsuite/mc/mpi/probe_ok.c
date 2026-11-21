/* Copyright (c) 2024-2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Rank 0 probes the message of rank 1, checks the status, and receives it */

#include <assert.h>
#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv)
{
  int rank;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (rank == 0) {
    MPI_Status status;
    int count;
    int value = 0;
    MPI_Probe(1, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status, MPI_INT, &count);
    assert(status.MPI_SOURCE == 1 && status.MPI_TAG == 42 && count == 1);
    MPI_Recv(&value, 1, MPI_INT, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    printf("rank 0 received %d\n", value);
  } else if (rank == 1) {
    int value = 1;
    MPI_Send(&value, 1, MPI_INT, 0, 42, MPI_COMM_WORLD);
  }

  MPI_Finalize();
  return 0;
}
