/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mpi.h"
#include <stdio.h>

static int test(int myid, int numprocs)
{
// The tags should match on the sender and receiver side.
// The distinction between sendtag and recvtag is mainly
// useful to make some other Recv or Send calls match the sendrecv. 
#define TAG_RCV 999
#define TAG_SND 999


#define BUFLEN 10
  int left, right;
  int buffer[BUFLEN], buffer2[BUFLEN];
  int i;
  MPI_Status status;

  for (i = 0; i < BUFLEN; i++) {
    buffer[i] = myid;
  }

  right = (myid + 1) % numprocs;
  left = myid - 1;
  if (left < 0)
    left = numprocs - 1;

  /* performs a right-to-left shift of vectors */
  MPI_Sendrecv(buffer, 10, MPI_INT, left, TAG_SND, buffer2, 10, MPI_INT,
               right, TAG_RCV, MPI_COMM_WORLD, &status);

  for (i = 0; i < BUFLEN; i++) {
    if (buffer2[i] != right) {
      fprintf(stderr, "[%d] error: should have values %d, has %d\n", myid,
              right, buffer2[i]);
      return (0);
    }
  }
  return (1);
}

int main(int argc, char *argv[])
{

  int myid, numprocs;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);


  if (0 == myid)
    printf("\n    *** MPI_Sendrecv test ***\n\n");

  if (test(myid, numprocs)) {
    fprintf(stderr, "[%d] ok.\n", myid);
  } else {
    fprintf(stderr, "[%d] failed.\n", myid);
  }


  MPI_Finalize();
  return 0;
}
