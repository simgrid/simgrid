/* Copyright (c) 2009-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Copyright (c) 2019. Jonathan Borne.                                      */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define DIM 2
#define Q 2
/* Where DIM is the dimension of the Grid (2D) */
/* and Q is the number of processes per dimension */
#define N 3
/* Local matrices size N*N */

int main(int argc, char** argv)
{
  /* Nb of nodes in the grid:
     initialized by MPI_Comm_size according to commandline -np value */
  int nbNodes;

  /* Communicators */
  MPI_Comm gridComm;
  MPI_Comm lineComm;
  /* Current process ranks */
  int rank;
  int gridSize;
  int myGridRank;
  int myLineRank;
  int myColRank;
  /* coords: used to get myLineRank and myColRank
     initialized by MPI_Cart_coords
   */
  int coords[DIM];
  /* dims: Integer array of size ndims specifying the number
     of processes in each dimension.
     if init value is 0 it is reset by MPI_Dims_create.
  */
  int dims[DIM];
  for (int i = 0; i < DIM; i++) {
    dims[i] = Q;
  }
  /* periods:
     Logical array of size ndims specifying whether the grid is
     periodic (true) or not (false) in each dimension. */
  int periods[DIM];
  for (int i = 0; i < DIM; i++) {
    periods[i] = 1;
  }
  /* reorder: do not allows rank reordering when creating the grid comm */
  int reorder = 0;
  /* remainDims[]: used to set which dimension is kept in subcommunicators */
  int remainDim[DIM];

  /* Local Matrix */
  int* A = malloc(N * N * sizeof(int));

  /* Init */
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nbNodes);

  printf("rank %d: Alive \n", rank);

  MPI_Barrier(MPI_COMM_WORLD);

  /* Set dims[] values to descibe a grid of nbNodes and DIM dimensions*/
  MPI_Cart_create(MPI_COMM_WORLD, DIM, dims, periods, reorder, &gridComm);

  if (gridComm == MPI_COMM_NULL)
    printf("error grid NULLCOMM \n");

  MPI_Comm_rank(gridComm, &myGridRank);
  MPI_Comm_size(gridComm, &gridSize);
  MPI_Cart_coords(gridComm, myGridRank, DIM, coords);
  myLineRank = coords[0];
  myColRank  = coords[1];

  MPI_Barrier(MPI_COMM_WORLD);

  /* Create a line communicator for current process */
  remainDim[0] = 0;
  remainDim[1] = 1;
  MPI_Cart_sub(gridComm, remainDim, &lineComm);

  /* Check if lineComm was initialized */
  if (lineComm == MPI_COMM_NULL)
    printf("(%d,%d): ERR (lineComm == NULLCOMM)\n", myLineRank, myColRank);

  /* A Initialization */
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      *(A + (i * N) + j) = i == j ? rank : 0;
    }
  }

  MPI_Barrier(MPI_COMM_WORLD);

  /* Broadcast */
  int root = 0;
  MPI_Bcast(A, N * N, MPI_INT, root, lineComm);

  /* Print A */
  printf("process:(%d,%d) \n", myLineRank, myColRank);

  printf("-------------------\n");
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      printf("%d ", *(A + (i * N) + j));
    }
    printf("\n");
  }
  printf("-------------------\n");

  MPI_Comm_free(&lineComm);
  MPI_Comm_free(&gridComm);
  free(A);
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();
  return 0;
}
