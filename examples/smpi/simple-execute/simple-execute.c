/* A simple example ping-pong program to test MPI_Send and MPI_Recv */

/* Copyright (c) 2009-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <mpi.h>

/* This test performs a simple pingpong between 2 processes
   Each process calls smpi_execute or smpi_execute_flops */

int main(int argc, char *argv[])
{
  const int tag1 = 42;
  const int tag2 = 43; /* Message tag */
  int size;
  int rank;
  int msg = 99;
  MPI_Status status;
  int err = MPI_Init(&argc, &argv); /* Initialize MPI */

  if (err != MPI_SUCCESS) {
    printf("MPI initialization failed!\n");
    exit(1);
  }
  MPI_Comm_size(MPI_COMM_WORLD, &size);   /* Get nr of tasks */
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);   /* Get id of this process */
  if (size != 2) {
    printf("run this program with exactly 2 processes (-np 2)\n");
    MPI_Finalize();
    exit(0);
  }
  if (rank == 0) {
    printf("\n    *** Ping-pong test (MPI_Send/MPI_Recv) ***\n\n");

    /* start ping-pong tests between several pairs */
    int dst = 1;
    printf("[%d] About to send 1st message '%d' to process [%d]\n", rank, msg, dst);
    MPI_Send(&msg, 1, MPI_INT, dst, tag1, MPI_COMM_WORLD);

    /* Inject five seconds of fake computation time */
    /* We are in a public file, not internal to simgrid, so _benched flavour is preferred, as it protects against accidental skip */
    /* smpi_execute_benched here is mostly equivalent to sleep, which is intercepted by smpi and turned into smpi_sleep */
    /* Difference with sleep is only for energy consumption */
    smpi_execute_benched(5.0);

    MPI_Recv(&msg, 1, MPI_INT, dst, tag2, MPI_COMM_WORLD, &status);     /* Receive a message */
    printf("[%d] Received reply message '%d' from process [%d]\n", rank, msg, dst);
  } else {
    int src = 0;
    MPI_Recv(&msg, 1, MPI_INT, src, tag1, MPI_COMM_WORLD, &status);     /* Receive a message */
    printf("[%d] Received 1st message '%d' from process [%d]\n", rank, msg, src);
    msg++;

    /* Inject 762.96 Mflops of computation time - Host Jupiter is 76.296Mf per second, so this should amount to 10s */
    /* We are in a public file, not internal to simgrid, so _benched flavour is preferred, as it protects against accidental skip */
    smpi_execute_flops_benched(762960000);

    printf("[%d] After a nap, increment message's value to  '%d'\n", rank, msg);
    printf("[%d] About to send back message '%d' to process [%d]\n", rank, msg, src);
    MPI_Send(&msg, 1, MPI_INT, src, tag2, MPI_COMM_WORLD);
  }
  MPI_Finalize();
  return 0;
}
