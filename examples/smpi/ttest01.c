/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mpi.h"
#include <stdio.h>
#include "instr/instr.h"

#define DATATOSENT 100000000

int main(int argc, char *argv[])
{
  MPI_Status status;
  int rank, numprocs, tag = 0;
  int *r = malloc(sizeof(int) * DATATOSENT);

  MPI_Init(&argc,&argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  TRACE_smpi_set_category ("A"); 
  if (rank == 0){
    MPI_Send(r, DATATOSENT, MPI_INT, 1, tag, MPI_COMM_WORLD);
  }else if (rank == 1){
    MPI_Recv(r, DATATOSENT, MPI_INT, 0, tag, MPI_COMM_WORLD, &status);
  }else{ 
    //do nothing
  }
  TRACE_smpi_set_category ("B"); 
  if (rank == 0){
    MPI_Recv(r, DATATOSENT, MPI_INT, 1, tag, MPI_COMM_WORLD, &status);
  }else if (rank == 1){
    MPI_Send(r, DATATOSENT, MPI_INT, 0, tag, MPI_COMM_WORLD);
  }else{ 
    //do nothing
  }
  TRACE_smpi_set_category ("C"); 
  MPI_Barrier (MPI_COMM_WORLD);
  MPI_Finalize();
  return 0;
}
