/* Copyright (c) 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <mpi.h>

int main(int argc, char **argv)
{
   int size, rank;
   int success = 1;
   int retval;
   int sendcount = 1;            // one double to each process
   int recvcount = 1;
   int i;
   double *sndbuf = NULL;
   double rcvd;
   int root = 0;                 // arbitrary choice

   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &size);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);

   // on root, initialize sendbuf
   if (root == rank) {
     sndbuf = malloc(size * sizeof(double));
     for (i = 0; i < size; i++) {
       sndbuf[i] = (double) i;
     }
   }

   retval = MPI_Scatter(sndbuf, sendcount, MPI_DOUBLE, &rcvd, recvcount, MPI_DOUBLE, root, MPI_COMM_WORLD);
   if (root == rank) {
     free(sndbuf);
   }
   if (retval != MPI_SUCCESS) {
     fprintf(stderr, "(%s:%d) MPI_Scatter() returned retval=%d\n", __FILE__, __LINE__, retval);
     return 0;
   }
   // verification
   if ((double) rank != rcvd) {
     fprintf(stderr, "[%d] has %f instead of %d\n", rank, rcvd, rank);
     success = 0;
   }

  /* test 1 */
  if (0 == rank)
    printf("** Small Test Result: ...\n");
  if (!success)
    printf("\t[%d] failed.\n", rank);
  else
    printf("\t[%d] ok.\n", rank);

  MPI_Finalize();
  return 0;
}
