/* Copyright (c) 2026. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* The default MPI_Scan of SMPI waits for its receptions with MPI_Waitany. The model-checker must explore all the
 * variants of these WaitAny without losing the states leading to them. */

#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv)
{
  int rank;
  int in = 1;
  int out;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Scan(&in, &out, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
  if (out != rank + 1)
    printf("Rank %d got %d instead of %d\n", rank, out, rank + 1);
  MPI_Finalize();
  return 0;
}
