/* A simple bugged MPI_ISend and MPI_IRecv test */

/* Copyright (c) 2009, 2011, 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <mpi.h>
#include <simgrid/modelchecker.h>

int main(int argc, char **argv)
{
  int recv_buff, err, size, rank, i;
  MPI_Status status;

  /* Initialize MPI */
  err = MPI_Init(&argc, &argv);
  if (err != MPI_SUCCESS) {
    printf("MPI initialization failed!\n");
    exit(1);
  }

  MPI_Comm_size(MPI_COMM_WORLD, &size);   /* Get nr of tasks */
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);   /* Get id of this process */
  if (size < 2) {
    printf("run this program with exactly 2 processes (-np 2)\n");
    MPI_Finalize();
    exit(0);
  }

  if (rank == 0) {
    printf("MPI_ISend / MPI_IRecv Test \n");

    for(i=0; i < size - 1; i++){
      MPI_Recv(&recv_buff, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      printf("Message received from %d\n", recv_buff);
    }

    printf("recv_buff = %d\n", recv_buff);
  }else{
    MPI_Send(&rank, 1, MPI_INT, 0, 42, MPI_COMM_WORLD);
    printf("Sent %d to rank 0\n", rank);
  }

  MPI_Finalize();

  return 0;
}
