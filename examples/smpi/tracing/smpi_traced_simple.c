/* Copyright (c) 2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include <mpi.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
  MPI_Init(&argc, &argv);
  MPI_Barrier (MPI_COMM_WORLD);
  MPI_Finalize();
  return 0;
}
