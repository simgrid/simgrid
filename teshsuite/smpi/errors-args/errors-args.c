/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Smoke test of the SMPI argument checks: every call below is erroneous, and must be reported and return an error.
 * It runs on a single rank, so that a collective failing on one rank cannot desynchronize the others. */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

static void expect_error(int ret, const char* what)
{
  if (ret == MPI_SUCCESS)
    printf("Error not detected: %s\n", what);
}

int main(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);
  int sbuf[4] = {0, 1, 2, 3};
  int* rbuf   = malloc(4 * sizeof(int)); /* malloc'ed so that SMPI knows its size */
  int counts[1];
  int displs[1] = {3};
  int one[1]    = {1};
  MPI_Request req;
  double d;

  counts[0] = 2;
  expect_error(MPI_Gatherv(sbuf, 2, MPI_INT, rbuf, counts, displs, MPI_INT, 0, MPI_COMM_WORLD), "block out of buffer");
  displs[0] = 0;
  expect_error(MPI_Gatherv(sbuf, 2, MPI_INT, rbuf, one, displs, MPI_INT, 0, MPI_COMM_WORLD), "self block size");
  counts[0] = -1;
  expect_error(MPI_Alltoallv(sbuf, counts, displs, MPI_INT, rbuf, one, displs, MPI_INT, MPI_COMM_WORLD),
               "negative count");

  MPI_Isend(sbuf, 4, MPI_INT, 0, 0, MPI_COMM_WORLD, &req);
  expect_error(MPI_Recv(rbuf, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE), "truncation");
  MPI_Wait(&req, MPI_STATUS_IGNORE);

  MPI_Isend(sbuf, 2, MPI_INT, 0, 1, MPI_COMM_WORLD, &req);
  expect_error(MPI_Recv(&d, 1, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE), "type mismatch");
  MPI_Wait(&req, MPI_STATUS_IGNORE);

  free(rbuf);
  MPI_Finalize();
  return 0;
}
