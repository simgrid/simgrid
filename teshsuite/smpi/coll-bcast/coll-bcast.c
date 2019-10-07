/* Copyright (c) 2009-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <mpi.h>

int main(int argc, char **argv)
{
  int i;
  int size;
  int rank;
  int count = 2048;
  int status;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

  int *values = (int *) xbt_malloc(count * sizeof(int));

  for (i = 0; i < count; i++)
    values[i] = (0 == rank) ? 17 : 3;

  status = MPI_Bcast(NULL, count, MPI_INT, 0, MPI_COMM_WORLD);
  if(status!=MPI_ERR_BUFFER)
    printf("MPI_Bcast did not return MPI_ERR_BUFFER for empty sendbuf\n");
  status = MPI_Bcast(values, -1, MPI_INT, 0, MPI_COMM_WORLD);
  if(status!=MPI_ERR_COUNT)
    printf("MPI_Bcast did not return MPI_ERR_COUNT for -1 sendcount\n");
  status = MPI_Bcast(values, count, MPI_DATATYPE_NULL, 0, MPI_COMM_WORLD);
  if(status!=MPI_ERR_TYPE)
    printf("MPI_Bcast did not return MPI_ERR_TYPE for MPI_DATATYPE_NULL sendtype\n");
  status = MPI_Bcast(values, count, MPI_INT, -1, MPI_COMM_WORLD);
  if(status!=MPI_ERR_ROOT)
    printf("MPI_Bcast did not return MPI_ERR_ROOT for -1 root\n");
  status = MPI_Bcast(values, count, MPI_INT, size, MPI_COMM_WORLD);
  if(status!=MPI_ERR_ROOT)
    printf("MPI_Bcast did not return MPI_ERR_ROOT for root > size\n");
  status = MPI_Bcast(values, count, MPI_INT, 0, MPI_COMM_NULL);
  if(status!=MPI_ERR_COMM)
    printf("MPI_Bcast did not return MPI_ERR_COMM for MPI_COMM_NULL comm\n");

  MPI_Bcast(values, count, MPI_INT, 0, MPI_COMM_WORLD);

  int good = 0;
  for (i = 0; i < count; i++)
    if (values[i]==17) good++;
  printf("[%d] number of values equals to 17: %d\n", rank, good);

  MPI_Barrier(MPI_COMM_WORLD);
  xbt_free(values);

  count = 4096;
  values = (int *) xbt_malloc(count * sizeof(int));

  for (i = 0; i < count; i++)
    values[i] = (size -1 == rank) ? 17 : 3;

  status = MPI_Bcast(values, count, MPI_INT, size-1, MPI_COMM_WORLD);

  good = 0;
  for (i = 0; i < count; i++)
    if (values[i]==17) good++;
  printf("[%d] number of values equals to 17: %d\n", rank, good);

  if (rank == 0 && status != MPI_SUCCESS) {
    printf("bcast returned %d\n", status);
    fflush(stdout);
  }
  xbt_free(values);
  MPI_Finalize();
  return 0;
}
