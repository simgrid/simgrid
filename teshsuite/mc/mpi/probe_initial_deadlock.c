/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Each rank probes the message of the other before sending its own: the initial state is already a deadlock */

#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv)
{
  int rank;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  int peer = 1 - rank;
  MPI_Probe(peer, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  MPI_Send(&rank, 1, MPI_INT, peer, 0, MPI_COMM_WORLD);
  MPI_Recv(&peer, 1, MPI_INT, peer, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  printf("rank %d received the message of rank %d\n", rank, peer);

  MPI_Finalize();
  return 0;
}
