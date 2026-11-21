/* Copyright (c) 2024-2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Rank 0 probes the message of rank 1 with tag 1. The other messages to rank 0 do not match (wrong tag or wrong
 * source), so the race between the matching send and the probe is never reversible. */

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
    int value = 0;
    MPI_Probe(1, 1, MPI_COMM_WORLD, &status);
    assert(status.MPI_SOURCE == 1 && status.MPI_TAG == 1);
    MPI_Recv(&value, 1, MPI_INT, 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&value, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&value, 1, MPI_INT, 2, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    printf("rank 0 received all messages\n");
  } else if (rank == 1) {
    MPI_Send(&rank, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    MPI_Send(&rank, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
  } else {
    MPI_Send(&rank, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
  }

  MPI_Finalize();
  return 0;
}
