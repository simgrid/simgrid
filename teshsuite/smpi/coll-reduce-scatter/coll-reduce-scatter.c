/* Copyright (c) 2013-2021. The SimGrid Team.
 * All rights reserved.                           */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/*
 * Test of reduce scatter.
 * Each processor contributes its rank + the index to the reduction,  then receives the ith sum
 * Can be called with any number of processors.
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

int main( int argc, char **argv )
{
  int err = 0;
  int toterr;
  int size;
  int rank;
  int i;
  MPI_Comm comm;

  MPI_Init( &argc, &argv );
  comm = MPI_COMM_WORLD;

  MPI_Comm_size( comm, &size );
  MPI_Comm_rank( comm, &rank );
  MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

  int* sendbuf = (int *) malloc( size * sizeof(int) );
  for (i=0; i<size; i++)
    sendbuf[i] = rank + i;
  int* recvcounts = (int*) malloc (size * sizeof(int));
  int* recvbuf  = (int*) malloc (size * sizeof(int));
  for (i=0; i<size; i++)
    recvcounts[i] = 1;
  int retval;

  retval = MPI_Reduce_scatter(NULL, recvbuf, recvcounts, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  if(retval!=MPI_ERR_BUFFER)
    printf("MPI_Reduce_scatter did not return MPI_ERR_BUFFER for empty sendbuf\n");
  retval = MPI_Reduce_scatter(sendbuf, NULL, recvcounts, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  if(retval!=MPI_ERR_BUFFER)
    printf("MPI_Reduce_scatter did not return MPI_ERR_BUFFER for empty recvbuf\n");
  retval = MPI_Reduce_scatter(sendbuf, recvbuf, NULL, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  if(retval!=MPI_ERR_COUNT)
    printf("MPI_Reduce_scatter did not return MPI_ERR_COUNT for NULL recvcounts\n");
  retval = MPI_Reduce_scatter(sendbuf, recvbuf, recvcounts, MPI_DATATYPE_NULL, MPI_SUM, MPI_COMM_WORLD);
  if(retval!=MPI_ERR_TYPE)
    printf("MPI_Reduce_scatter did not return MPI_ERR_TYPE for MPI_DATATYPE_NULL type\n");
  retval = MPI_Reduce_scatter(sendbuf, recvbuf, recvcounts, MPI_DOUBLE, MPI_OP_NULL, MPI_COMM_WORLD);
  if(retval!=MPI_ERR_OP)
    printf("MPI_Reduce_scatter did not return MPI_ERR_OP for MPI_OP_NULL op\n");
  retval = MPI_Reduce_scatter(sendbuf, recvbuf, recvcounts, MPI_DOUBLE, MPI_SUM, MPI_COMM_NULL);
  if(retval!=MPI_ERR_COMM)
    printf("MPI_Reduce_scatter did not return MPI_ERR_COMM for MPI_COMM_NULL comm\n");

  MPI_Reduce_scatter( sendbuf, recvbuf, recvcounts, MPI_INT, MPI_SUM, comm );
  int sumval = size * rank + ((size - 1) * size)/2;
  /* recvbuf should be size * (rank + i) */
  if (recvbuf[0] != sumval) {
    err++;
    fprintf( stdout, "Did not get expected value for reduce scatter\n" );
    fprintf( stdout, "[%d] Got %d expected %d\n", rank, recvbuf[0], sumval );
  }

  MPI_Allreduce( &err, &toterr, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD );
  if (rank == 0 && toterr == 0) {
    printf( " No Errors\n" );
  }
  free(sendbuf);
  free(recvcounts);
  free(recvbuf);

  MPI_Finalize();

  return toterr;
}
