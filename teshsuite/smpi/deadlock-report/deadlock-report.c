/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Deadlock between two ranks, to check how SMPI reports it: rank 0 waits for a message that rank 1 never sends, while
 * rank 1 waits for any message. */

#include <mpi.h>

int main(int argc, char* argv[])
{
  int rank;
  int value = 0;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (rank == 0)
    MPI_Recv(&value, 1, MPI_INT, 1, 7, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  else
    MPI_Recv(&value, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  MPI_Finalize();
  return 0;
}
