/* Copyright (c) 2024-2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Ranks 1 and 2 both send to rank 0, which probes with MPI_ANY_SOURCE: both outcomes must be explored. The race
 * between the first send and the probe is not reversible, while the race with the second send is. */

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
    MPI_Probe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
    printf("rank 0 probed the message of rank %d\n", status.MPI_SOURCE);
    MPI_Recv(&value, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    assert(value == status.MPI_SOURCE);
    MPI_Recv(&value, 1, MPI_INT, 3 - status.MPI_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  } else {
    MPI_Send(&rank, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
  }

  MPI_Finalize();
  return 0;
}
