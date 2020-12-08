/* Copyright (c) 2009-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u/Actor.hpp>
#include "smpi/smpi.h"
#include "smpi/sampi.h"

int main(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);
  // useless alocations for testing and coverage
  void* pointer = malloc(100 * sizeof(int));
  void* ptmp;
  if ((ptmp = realloc(pointer, 50 * sizeof(int))) != nullptr)
    pointer = ptmp;
  if ((ptmp = realloc(pointer, 200 * sizeof(int))) != nullptr)
    pointer = ptmp;
  free(pointer);
  pointer = calloc(100, sizeof(int));
  int rank;
  int err = MPI_Comm_rank(MPI_COMM_WORLD, &rank);   /* Get id of this process */
  if (err != MPI_SUCCESS) {
    fprintf(stderr, "MPI_Comm_rank failed: %d", err);
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    exit(EXIT_FAILURE);
  }
  AMPI_Iteration_in(MPI_COMM_WORLD);
  simgrid::s4u::this_actor::sleep_for(rank);
  AMPI_Iteration_out(MPI_COMM_WORLD);

  AMPI_Iteration_in(MPI_COMM_WORLD);
  simgrid::s4u::this_actor::sleep_for(rank);
  AMPI_Iteration_out(MPI_COMM_WORLD);
  if (rank == 0) {
    free(pointer);
    pointer = nullptr;
  }
  AMPI_Migrate(MPI_COMM_WORLD);
  if (rank != 0)
    free(pointer);

  MPI_Finalize();
  return 0;
}

