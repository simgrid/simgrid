/* Copyright (c) 2012-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <mpi.h>

/* test*/
int main(int argc, char *argv[])
{
  MPI_Init(&argc, &argv);
  MPI_Barrier (MPI_COMM_WORLD);
  sleep(1000);
  MPI_Finalize();
  return 0;
}
