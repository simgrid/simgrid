/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Smoke test of the SMPI argument checks: every call below is erroneous, and must be reported and return an error.
 * On one rank, the local checks are tested. On two ranks, the checks comparing the arguments of several ranks are. */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

static void expect_error(int ret, const char* what)
{
  if (ret == MPI_SUCCESS)
    printf("Error not detected: %s\n", what);
}

static void local_errors(void)
{
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
  expect_error(MPI_Allreduce(sbuf, sbuf, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD), "aliased buffers");

  MPI_Isend(sbuf, 4, MPI_INT, 0, 0, MPI_COMM_WORLD, &req);
  expect_error(MPI_Recv(rbuf, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE), "truncation");
  MPI_Wait(&req, MPI_STATUS_IGNORE);

  MPI_Isend(sbuf, 2, MPI_INT, 0, 1, MPI_COMM_WORLD, &req);
  expect_error(MPI_Recv(&d, 1, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE), "type mismatch");
  MPI_Wait(&req, MPI_STATUS_IGNORE);

  MPI_Isend(sbuf, 6, MPI_BYTE, 0, 4, MPI_COMM_WORLD, &req);
  expect_error(MPI_Recv(rbuf, 2, MPI_INT, 0, 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE), "partial element");
  MPI_Wait(&req, MPI_STATUS_IGNORE);

  /* Legal but not portable: only reported as a warning */
  MPI_Isend(sbuf, 1, MPI_INT, 0, 40000, MPI_COMM_WORLD, &req);
  MPI_Recv(rbuf, 1, MPI_INT, 0, 40000, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  MPI_Wait(&req, MPI_STATUS_IGNORE);

  expect_error(MPI_Bsend(sbuf, 1, MPI_INT, 0, 2, MPI_COMM_WORLD), "no buffer attached");
  int two[1]  = {2};
  int zero[1] = {0};
  MPI_Comm cart;
  expect_error(MPI_Cart_create(MPI_COMM_WORLD, 1, two, zero, 0, &cart), "topology too large");
  /* Reported as legal but risky, and left unmatched until MPI_Finalize, which reports it again */
  MPI_Irecv(&d, 1, MPI_DOUBLE, 0, 3, MPI_COMM_WORLD, &req);
  MPI_Request_free(&req);

  free(rbuf);
}

/* For the size mismatch, rank 1 enters the collective first (its small send completes alone), so that the root detects
 * the error and returns instead of waiting for data. */
static void cross_rank_errors(int rank)
{
  int sbuf[2]   = {0, 1};
  int rbuf[3]   = {0};
  int counts[2] = {1, 1};
  int displs[2] = {0, 0};
  int token     = 0;

  expect_error(MPI_Allgatherv(sbuf, 1, MPI_INT, rbuf, counts, displs, MPI_INT, MPI_COMM_WORLD), "overlapping blocks");
  displs[1] = 1;
  if (rank == 1) {
    MPI_Gatherv(sbuf, 2, MPI_INT, NULL, NULL, NULL, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Send(&token, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
  } else {
    MPI_Recv(&token, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    expect_error(MPI_Gatherv(sbuf, 1, MPI_INT, rbuf, counts, displs, MPI_INT, 0, MPI_COMM_WORLD), "pairwise size");
  }
}

int main(int argc, char* argv[])
{
  int rank;
  int size;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  if (size == 1)
    local_errors();
  else
    cross_rank_errors(rank);
  MPI_Finalize();
  return 0;
}
