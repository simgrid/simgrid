/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/******************** Non-deterministic message ordering  *********************/
/* Server assumes a fixed order in the reception of messages from its clients */
/* which is incorrect because the message ordering is non-deterministic       */
/******************************************************************************/

#include <mpi.h>

int main(int argc, char **argv)
{
  int size;
  int rank;
  MPI_Status status;

  /* Initialize MPI */
  int err = MPI_Init(&argc, &argv);
  if (err != MPI_SUCCESS) {
    printf("MPI initialization failed!\n");
    exit(1);
  }

  MPI_Comm_size(MPI_COMM_WORLD, &size);   /* Get nr of tasks */
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);   /* Get id of this process */
  if (size != 4) {
    printf("run this program with exactly 4 processes (-np 4)\n");
    MPI_Finalize();
    exit(0);
  }

  if (rank == 0) {
    int recv_buffer;
    for (int i = 0; i < size - 1; i++) {
      MPI_Recv(&recv_buffer, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      printf("Message received from %d\n", recv_buffer);
    }

    if (recv_buffer != 3) {
      printf("The last received message is not 3 but %d!\n", recv_buffer);
      fflush(stdout);
      abort();
    }
  }else{
    MPI_Send(&rank, 1, MPI_INT, 0, 42, MPI_COMM_WORLD);
    printf("Sent %d to rank 0\n", rank);
  }

  MPI_Finalize();

  return 0;
}
