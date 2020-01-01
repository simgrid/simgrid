/* ../../../smpi_script/bin/smpirun -hostfile hostfile_send_deterministic -platform ../../platforms/cluster_backbone.xml -np 3 --cfg=smpi/send-is-detached-thresh:0 gdb\ --args\ ./send_deterministic */

/* Copyright (c) 2009-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <mpi.h>
#include <simgrid/modelchecker.h>

int main(int argc, char **argv)
{
  int recv_buff;
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
  if (size < 2) {
    printf("run this program with at least 2 processes \n");
    MPI_Finalize();
    exit(0);
  }

  if (rank == 0) {
    for (int i = 0; i < size - 1; i++) {
      MPI_Recv(&recv_buff, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    }
  }else{
    MPI_Send(&rank, 1, MPI_INT, 0, 42, MPI_COMM_WORLD);
  }

  MPI_Finalize();

  return 0;
}
