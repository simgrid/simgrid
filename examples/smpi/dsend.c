/* Copyright (c) 2011. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This program simply does a very small exchange to test whether using SIMIX dsend to model the eager mode works */

#include <stdio.h>
#include <mpi.h>

int main(int argc, char *argv[]) {
  int rank;
  int data=11;
   

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   
  if (rank==1) {
	  data=22;
	  MPI_Send(&data,1,MPI_INT,(rank+1)%2,666,MPI_COMM_WORLD);
//	 smpi_sleep(1000);
  } else {
	  MPI_Recv(&data,1,MPI_INT,-1,666,MPI_COMM_WORLD,NULL);
	  if (data !=22) {
		  printf("rank %d: Damn, data does not match (got %d)\n",rank, data);
	  }
  }

  printf("rank %d: data exchanged\n", rank);
  MPI_Finalize();
  return 0;
}
