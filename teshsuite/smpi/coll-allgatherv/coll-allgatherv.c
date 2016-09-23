/* Copyright (c) 2009-2010, 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "mpi.h"

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#endif

int main(int argc, char *argv[])
{
  int i,rank, size;
  int *sb, *rb;
  int *recv_counts, *recv_disps;
  int recv_sb_size;
  int status;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  recv_counts = (int *) xbt_malloc(size * sizeof(int));
  recv_disps = (int *) xbt_malloc(size * sizeof(int));

  recv_sb_size = 0;
  for (i = 0; i < size; i++) {
    recv_counts[i] = i + 1;
    recv_disps[i] = recv_sb_size;
    recv_sb_size += i + 1;
  }

  sb = (int *) xbt_malloc(recv_counts[rank] * sizeof(int));
  rb = (int *) xbt_malloc(recv_sb_size * sizeof(int));

  for (i = 0; i < recv_counts[rank]; ++i)
    sb[i] = recv_disps[rank] + i;
  for (i = 0; i < recv_sb_size; ++i)  
    rb[i] = -1;

  printf("[%d] sndbuf=[", rank);
  for (i = 0; i < recv_counts[rank]; i++)
    printf("%d ", sb[i]);
  printf("]\n");

  status = MPI_Allgatherv(sb, recv_counts[rank], MPI_INT, rb, recv_counts, recv_disps, MPI_INT, MPI_COMM_WORLD);

  printf("[%d] rcvbuf=[", rank);
  for (i = 0; i < recv_sb_size; i++)
    printf("%d ", rb[i]);
  printf("]\n");

  if (rank == 0) {
    if (status != MPI_SUCCESS) {
      printf("allgatherv returned %d\n", status);
      fflush(stdout);
    }
  }
  free(sb);
  free(rb);
  free(recv_counts);
  free(recv_disps);
  MPI_Finalize();
  return (EXIT_SUCCESS);
}
