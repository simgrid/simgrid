/* Copyright (c) 2018-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define N (1024 * 1024 * 1)

int main(int argc, char* argv[])
{
  int size, rank;
  struct timeval start, end;
  char hostname[256];
  int hostname_len;

  MPI_Init(&argc, &argv);

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Get_processor_name(hostname, &hostname_len);

  // Allocate a 1 MiB buffer
  char* buffer = malloc(sizeof(char) * N);

  // Communicate along the ring
  if (rank == 0) {
    gettimeofday(&start, NULL);
    printf("Rank %d (running on '%s'): sending the message rank %d\n", rank, hostname, 1);
    MPI_Send(buffer, N, MPI_BYTE, 1, 1, MPI_COMM_WORLD);
    MPI_Recv(buffer, N, MPI_BYTE, size - 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    printf("Rank %d (running on '%s'): received the message from rank %d\n", rank, hostname, size - 1);
    gettimeofday(&end, NULL);
    printf("%f\n", (end.tv_sec * 1000000.0 + end.tv_usec - start.tv_sec * 1000000.0 - start.tv_usec) / 1000000.0);

  } else {
    MPI_Recv(buffer, N, MPI_BYTE, rank - 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    printf("Rank %d (running on '%s'): receive the message and sending it to rank %d\n", rank, hostname,
           (rank + 1) % size);
    MPI_Send(buffer, N, MPI_BYTE, (rank + 1) % size, 1, MPI_COMM_WORLD);
  }

  MPI_Finalize();
  return 0;
}
