/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Rank 0 puts into the window of rank 1, which accesses that window in the same epoch (a data race) or after the fence
 * that closes it (correct). The argument selects how rank 1 accesses its window:
 *  - write: a plain write
 *  - put:   the origin buffer of a put of its own, read by SMPI on behalf of rank 1
 *  - after: a plain write after the fence */

#include <mpi.h>
#include <string.h>

int main(int argc, char** argv)
{
  int buf[2] = {0, 0};
  int one    = 1;
  int rank;
  MPI_Win win;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Win_create(buf, sizeof(buf), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);

  MPI_Win_fence(0, win);
  if (rank == 0)
    MPI_Put(&one, 1, MPI_INT, 1, 0, 1, MPI_INT, win);
  else if (strcmp(argv[1], "write") == 0)
    buf[0] = 2;
  else if (strcmp(argv[1], "put") == 0)
    MPI_Put(&buf[0], 1, MPI_INT, 0, 1, 1, MPI_INT, win);
  MPI_Win_fence(0, win);
  if (rank == 1 && strcmp(argv[1], "after") == 0)
    buf[0] = 2;

  MPI_Win_free(&win);
  MPI_Finalize();
  return 0;
}
