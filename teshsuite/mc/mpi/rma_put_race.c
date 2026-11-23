/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Rank 0 puts or accumulates into the window of rank 1, which accesses that window in the same epoch (a data race,
 * unless both accesses are atomic) or after the fence that closes it (correct). The argument selects the case:
 *  - write:    put; rank 1 writes its window
 *  - put:      put; rank 1 puts from its window, whose origin buffer SMPI reads on behalf of rank 1
 *  - after:    put; rank 1 writes its window after the fence
 *  - acc_read: accumulate; rank 1 reads its window
 *  - acc_acc:  accumulate; rank 1 accumulates into its own window too, which is correct */

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
  if (rank == 0 && strncmp(argv[1], "acc", 3) == 0)
    MPI_Accumulate(&one, 1, MPI_INT, 1, 0, 1, MPI_INT, MPI_SUM, win);
  else if (rank == 0)
    MPI_Put(&one, 1, MPI_INT, 1, 0, 1, MPI_INT, win);
  else if (strcmp(argv[1], "write") == 0)
    buf[0] = 2;
  else if (strcmp(argv[1], "put") == 0)
    MPI_Put(&buf[0], 1, MPI_INT, 0, 1, 1, MPI_INT, win);
  else if (strcmp(argv[1], "acc_read") == 0)
    one = buf[0];
  else if (strcmp(argv[1], "acc_acc") == 0)
    MPI_Accumulate(&one, 1, MPI_INT, 1, 0, 1, MPI_INT, MPI_SUM, win);
  MPI_Win_fence(0, win);
  if (rank == 1 && strcmp(argv[1], "after") == 0)
    buf[0] = 2;

  MPI_Win_free(&win);
  MPI_Finalize();
  return 0;
}
