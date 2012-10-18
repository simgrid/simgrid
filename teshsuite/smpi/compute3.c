/* Copyright (c) 2009-2012. The SimGrid Team. All rights reserved.          */

/* This example should be instructive to learn about SMPI_SAMPLE_LOCAL and 
   SMPI_SAMPLE_GLOBAL macros for execution sampling */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <mpi.h>

int main(int argc, char *argv[])
{
  int i,j;
  double d;
  MPI_Init(&argc, &argv);
  d = 2.0;
  for (i=0;i<5;i++) {	
     SMPI_SAMPLE_GLOBAL(3,-1)  { // I want no more than 3 benchs (thres<0)
	fprintf(stderr,"[rank:%d] Run the first computation. It's globally benched, and I want no more than 3 benchmarks (thres<0)\n", smpi_process_index());
    
	for (j=0;j<100*1000*1000;j++) { // 100 kflop
	   if (d < 100000) {
	      d = d * d;
	   } else {
	      d = 2;
	   }
	}     
     }
  }

  for (i=0;i<5;i++) {	
     SMPI_SAMPLE_LOCAL(0, 0.1) { // I want the standard error to go below 0.1 second. Two tests at least will be run (count is not >0)
	fprintf(stderr,"[rank:%d] Run the first (locally benched) computation. It's locally benched, and I want the standard error to go below 0.1 second (count is not >0)\n", smpi_process_index());
	for (j=0;j<100*1000*1000;j++) { // 100 kflop
	   if (d < 100000) {
	      d = d * d;
	   } else {
	      d = 2;
	   }
	}
     }
  }
   
  fprintf(stderr,"[%d] The result of the computation is: %f\n", smpi_process_index(), d);

  MPI_Finalize();
  return 0;
}
