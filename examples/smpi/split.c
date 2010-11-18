/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <mpi.h>

int main(int argc, char *argv[])
{
  int worldrank;
  int localrank;
  MPI_Comm localcomm;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &worldrank);
  MPI_Comm_split(MPI_COMM_WORLD, worldrank % 2, worldrank, &localcomm);
  MPI_Comm_rank(localcomm, &localrank);
  printf("node with world rank %d has local rank %d\n", worldrank, localrank);
  MPI_Finalize();
  return 0;
}
