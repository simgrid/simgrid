/* Copyright (c) 2024-2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* The reception posted by rank 0 takes the only message, so its probe never matches: every execution deadlocks */

#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv)
{
  int rank;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (rank == 0) {
    MPI_Request request;
    int value = 0;
    MPI_Irecv(&value, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &request);
    MPI_Probe(1, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Wait(&request, MPI_STATUS_IGNORE);
    printf("rank 0 received %d\n", value);
  } else if (rank == 1) {
    int value = 1;
    MPI_Send(&value, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
  }

  MPI_Finalize();
  return 0;
}
