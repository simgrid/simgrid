/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Rank 0 gives the address of a heap block to rank 1, which writes it while rank 0 frees it (a data race), or before a
 * barrier that precedes the free (correct), depending on the argument (race or barrier). This relies on SMPI running
 * all ranks in the same address space. */

#include <mpi.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv)
{
  int rank;
  int* block = NULL;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (rank == 0)
    block = malloc(sizeof(int));
  uintptr_t addr = (uintptr_t)block;
  MPI_Bcast(&addr, sizeof(addr), MPI_BYTE, 0, MPI_COMM_WORLD);
  if (rank == 1)
    *(int*)addr = 1;
  if (strcmp(argv[1], "barrier") == 0)
    MPI_Barrier(MPI_COMM_WORLD);
  if (rank == 0)
    free(block);
  MPI_Barrier(MPI_COMM_WORLD);

  MPI_Finalize();
  return 0;
}
