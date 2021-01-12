/* Copyright (c) 2009-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <mpi.h>

int main(int argc, char **argv)
{
  int size;
  int rank;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

  int status = MPI_Barrier(MPI_COMM_NULL);
  if(status!=MPI_ERR_COMM)
    printf("MPI_Barrier did not return MPI_ERR_COMM for MPI_COMM_NULL comm\n");

  MPI_Barrier(MPI_COMM_WORLD);

  if (0 == rank) {
    printf("... Barrier ....\n");
  }

  MPI_Finalize();
  return 0;
}
