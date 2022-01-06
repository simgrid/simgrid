/* Copyright (c) 2012-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "mpi.h"
#define SIZE 4

int main(int argc, char **argv) {
  int rank;
  double a[SIZE][SIZE] = {{0}};

  MPI_Datatype columntype;

  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  MPI_Type_vector(SIZE, 1, SIZE, MPI_DOUBLE, &columntype);
  MPI_Type_commit(&columntype);

    if (rank == 0) {
      for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
          a[i][j] = i*SIZE+j;
    }

    /* only one column is sent this is an example for non-contiguous data*/
    MPI_Bcast(a, 1, columntype, 0, MPI_COMM_WORLD);

    for (int i = 0; i < SIZE; i++) {
      for (int j = 0; j < SIZE; j++) {
        printf("rank= %d, a[%d][%d]=%f\n", rank, i, j, a[i][j]);
      }
      printf("\n");
    }

  MPI_Type_free(&columntype);
  MPI_Finalize();
  return 0;
}
