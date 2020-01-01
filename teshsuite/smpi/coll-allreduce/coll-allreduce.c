/* Copyright (c) 2009-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "mpi.h"

int main(int argc, char *argv[])
{
  int rank;
  int size;
  int i;
  int status;
  int mult=1;

  MPI_Init(&argc, &argv);
  int maxlen = argc >= 2 ? atoi(argv[1]) : 1;

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

  if (maxlen > 1)
    mult = maxlen > size ? size : maxlen;
  int* sb = xbt_new0(int, size * maxlen);
  int* rb = xbt_new0(int, size * maxlen);

  for (i = 0; i < size *maxlen; ++i) {
    sb[i] = rank*size + i;
    rb[i] = 0;
  }

  status = MPI_Allreduce(NULL, rb, size, MPI_UNSIGNED_LONG_LONG, MPI_SUM, MPI_COMM_WORLD);
  if(status!=MPI_ERR_BUFFER)
    printf("MPI_Allreduce did not return MPI_ERR_BUFFER for empty sendbuf\n");
  status = MPI_Allreduce(sb, NULL, size, MPI_UNSIGNED_LONG_LONG, MPI_SUM, MPI_COMM_WORLD);
  if(status!=MPI_ERR_BUFFER)
    printf("MPI_Allreduce did not return MPI_ERR_BUFFER for empty recvbuf\n");
  status = MPI_Allreduce(sb, rb, -1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, MPI_COMM_WORLD);
  if(status!=MPI_ERR_COUNT)
    printf("MPI_Allreduce did not return MPI_ERR_COUNT for -1 count\n");
  status = MPI_Allreduce(sb, rb, size, MPI_DATATYPE_NULL, MPI_SUM, MPI_COMM_WORLD);
  if(status!=MPI_ERR_TYPE)
    printf("MPI_Allreduce did not return MPI_ERR_TYPE for MPI_DATATYPE_NULL type\n");
  status = MPI_Allreduce(sb, rb, size, MPI_UNSIGNED_LONG_LONG, MPI_OP_NULL, MPI_COMM_WORLD);
  if(status!=MPI_ERR_OP)
    printf("MPI_Allreduce did not return MPI_ERR_OP for MPI_OP_NULL op\n");
  status = MPI_Allreduce(sb, rb, size, MPI_UNSIGNED_LONG_LONG, MPI_SUM, MPI_COMM_NULL);
  if(status!=MPI_ERR_COMM)
    printf("MPI_Allreduce did not return MPI_ERR_COMM for MPI_COMM_NULL comm\n");

  printf("[%d] sndbuf=[", rank);
  for (i = 0; i < size *mult; i++)
    printf("%d ", sb[i]);
  printf("]\n");
  status = MPI_Allreduce(sb, rb, size *maxlen, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

  printf("[%d] rcvbuf=[", rank);
  for (i =  0; i < size *mult; i++)//do not print everything
    printf("%d ", rb[i]);
  printf("]\n");

  if (rank == 0 && status != MPI_SUCCESS) {
    printf("all_to_all returned %d\n", status);
    fflush(stdout);
  }
  xbt_free(sb);
  xbt_free(rb);
  MPI_Finalize();
  return (EXIT_SUCCESS);
}
