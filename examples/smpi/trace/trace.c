/* Copyright (c) 2009-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "mpi.h"
#include "simgrid/instr.h"
#include <unistd.h>

#define DATATOSENT 100000

int main(int argc, char *argv[])
{
  int rank;
  int numprocs;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  sleep(rank);
  /** Tests:
  * A: 0(isend -> wait) with 1(recv)
  * B: 0(send) with 1(irecv -> wait)
  * C: 0(N * isend -> N * wait) with 1(N * recv)
  * D: 0(N * isend -> N * waitany) with 1(N * recv)
  * E: 0(N*send) with 1(N*irecv, N*wait)
  * F: 0(N*send) with 1(N*irecv, N*waitany)
  * G: 0(N* isend -> waitall) with 1(N*recv)
  * H: 0(N*send) with 1(N*irecv, waitall)
  * I: 0(2*N*send, 2*N*Irecv, Waitall) with
  *    1(N*irecv, waitall, N*isend, N*waitany) with
  *    2(N*irecv, N*waitany, N*isend, waitall)
  * J: 0(N*isend, N*test, N*wait) with (N*irecv, N*test, N*wait)
  s*/
  int N = 13;

  int tag = 12345;
/////////////////////////////////////////
////////////////// RANK 0
///////////////////////////////////
  if (rank == 0) {
    MPI_Request request;
    MPI_Status status;
    MPI_Request req[2 * N];
    MPI_Status sta[2 * N];
    int *r = (int *) malloc(sizeof(int) * DATATOSENT);

    /** Test A */
    TRACE_smpi_set_category("A");
    MPI_Isend(r, DATATOSENT, MPI_INT, 1, tag, MPI_COMM_WORLD, &request);
    MPI_Wait(&request, &status);
    MPI_Barrier(MPI_COMM_WORLD);

    /** Test B */
    TRACE_smpi_set_category("B");
    MPI_Send(r, DATATOSENT, MPI_INT, 1, tag, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);

    /** Test C */
    TRACE_smpi_set_category("C");
    for (int i = 0; i < N; i++) {
      MPI_Isend(r, DATATOSENT, MPI_INT, 1, tag, MPI_COMM_WORLD, &req[i]);
    }
    for (int i = 0; i < N; i++) {
      MPI_Wait(&req[i], &sta[i]);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    TRACE_smpi_set_category("D");
    for (int i = 0; i < N; i++) {
      MPI_Isend(r, DATATOSENT, MPI_INT, 1, tag, MPI_COMM_WORLD, &req[i]);
    }
    for (int i = 0; i < N; i++) {
      int completed;
      MPI_Waitany(N, req, &completed, sta);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    TRACE_smpi_set_category("E");
    for (int i = 0; i < N; i++) {
      MPI_Send(r, DATATOSENT, MPI_INT, 1, tag, MPI_COMM_WORLD);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    TRACE_smpi_set_category("F");
    for (int i = 0; i < N; i++) {
      MPI_Send(r, DATATOSENT, MPI_INT, 1, tag, MPI_COMM_WORLD);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    TRACE_smpi_set_category("G");
    for (int i = 0; i < N; i++) {
      MPI_Isend(r, DATATOSENT, MPI_INT, 1, tag, MPI_COMM_WORLD, &req[i]);
    }
    MPI_Waitall(N, req, sta);
    MPI_Barrier(MPI_COMM_WORLD);

    TRACE_smpi_set_category("H");
    for (int i = 0; i < N; i++) {
      MPI_Send(r, DATATOSENT, MPI_INT, 1, tag, MPI_COMM_WORLD);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    TRACE_smpi_set_category("I");
    for (int i = 0; i < 2 * N; i++) {
      if (i < N) {
        MPI_Send(r, DATATOSENT, MPI_INT, 2, tag, MPI_COMM_WORLD);
      } else {
        MPI_Send(r, DATATOSENT, MPI_INT, 1, tag, MPI_COMM_WORLD);
      }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    for (int i = 0; i < 2 * N; i++) {
      if (i < N) {
        MPI_Irecv(r, DATATOSENT, MPI_INT, 1, tag, MPI_COMM_WORLD, &req[i]);
      } else {
        MPI_Irecv(r, DATATOSENT, MPI_INT, 2, tag, MPI_COMM_WORLD, &req[i]);
      }
    }
    MPI_Waitall(2 * N, req, sta);
    MPI_Barrier(MPI_COMM_WORLD);

    TRACE_smpi_set_category("J");
    for (int i = 0; i < N; i++) {
      MPI_Isend(r, DATATOSENT, MPI_INT, 1, tag, MPI_COMM_WORLD, &req[i]);
    }
    for (int i = 0; i < N; i++) {
      int flag;
      MPI_Test(&req[i], &flag, &sta[i]);
    }
    for (int i = 0; i < N; i++) {
      MPI_Wait(&req[i], &sta[i]);
    }
    free(r);
/////////////////////////////////////////
////////////////// RANK 1
///////////////////////////////////
  } else if (rank == 1) {
    MPI_Request request;
    MPI_Status status;
    MPI_Request req[N];
    MPI_Status sta[N];
    int *r = (int *) malloc(sizeof(int) * DATATOSENT);

    TRACE_smpi_set_category("A");
    MPI_Recv(r, DATATOSENT, MPI_INT, 0, tag, MPI_COMM_WORLD, &status);
    MPI_Barrier(MPI_COMM_WORLD);

    TRACE_smpi_set_category("B");
    MPI_Irecv(r, DATATOSENT, MPI_INT, 0, tag, MPI_COMM_WORLD, &request);
    MPI_Wait(&request, &status);
    MPI_Barrier(MPI_COMM_WORLD);

    TRACE_smpi_set_category("C");
    for (int i = 0; i < N; i++) {
      MPI_Recv(r, DATATOSENT, MPI_INT, 0, tag, MPI_COMM_WORLD, &sta[i]);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    TRACE_smpi_set_category("D");
    for (int i = 0; i < N; i++) {
      MPI_Recv(r, DATATOSENT, MPI_INT, 0, tag, MPI_COMM_WORLD, &sta[i]);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    TRACE_smpi_set_category("E");
    for (int i = 0; i < N; i++) {
      MPI_Irecv(r, DATATOSENT, MPI_INT, 0, tag, MPI_COMM_WORLD, &req[i]);
    }
    for (int i = 0; i < N; i++) {
      MPI_Wait(&req[i], &sta[i]);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    TRACE_smpi_set_category("F");
    for (int i = 0; i < N; i++) {
      MPI_Irecv(r, DATATOSENT, MPI_INT, 0, tag, MPI_COMM_WORLD, &req[i]);
    }
    for (int i = 0; i < N; i++) {
      int completed;
      MPI_Waitany(N, req, &completed, sta);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    TRACE_smpi_set_category("G");
    for (int i = 0; i < N; i++) {
      MPI_Recv(r, DATATOSENT, MPI_INT, 0, tag, MPI_COMM_WORLD, &sta[i]);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    TRACE_smpi_set_category("H");
    for (int i = 0; i < N; i++) {
      MPI_Irecv(r, DATATOSENT, MPI_INT, 0, tag, MPI_COMM_WORLD, &req[i]);
    }
    MPI_Waitall(N, req, sta);
    MPI_Barrier(MPI_COMM_WORLD);

    TRACE_smpi_set_category("I");
    for (int i = 0; i < N; i++) {
      MPI_Irecv(r, DATATOSENT, MPI_INT, 0, tag, MPI_COMM_WORLD, &req[i]);
    }
    MPI_Waitall(N, req, sta);

    MPI_Barrier(MPI_COMM_WORLD);
    for (int i = 0; i < N; i++) {
      MPI_Isend(r, DATATOSENT, MPI_INT, 0, tag, MPI_COMM_WORLD, &req[i]);
    }
    MPI_Waitall(N, req, sta);
    MPI_Barrier(MPI_COMM_WORLD);

    TRACE_smpi_set_category("J");
    for (int i = 0; i < N; i++) {
      MPI_Irecv(r, DATATOSENT, MPI_INT, 0, tag, MPI_COMM_WORLD, &req[i]);
    }
    for (int i = 0; i < N; i++) {
      int flag;
      MPI_Test(&req[i], &flag, &sta[i]);
    }
    for (int i = 0; i < N; i++) {
      MPI_Wait(&req[i], &sta[i]);
    }
    free(r);
/////////////////////////////////////////
////////////////// RANK 2
///////////////////////////////////
  } else if (rank == 2) {
    MPI_Request req[N];
    MPI_Status sta[N];
    int *r = (int *) malloc(sizeof(int) * DATATOSENT);

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    TRACE_smpi_set_category("I");
    for (int i = 0; i < N; i++) {
      MPI_Irecv(r, DATATOSENT, MPI_INT, 0, tag, MPI_COMM_WORLD, &req[i]);
    }
    for (int i = 0; i < N; i++) {
      int completed;
      MPI_Waitany(N, req, &completed, sta);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    for (int i = 0; i < N; i++) {
      MPI_Send(r, DATATOSENT, MPI_INT, 0, tag, MPI_COMM_WORLD);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    free(r);
  }
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();
  return 0;
}
