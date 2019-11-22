/* Copyright (c) 2009-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example should be instructive to learn about SMPI_SHARED_CALL */

#include <stdio.h>
#include <mpi.h>
#include <stdint.h>
#include <inttypes.h>

static void* hash(const char *str, uint64_t* ans)
{
  const char *tohash = str;
  *ans=5381;
  printf("hashing !\n");
  int c = *tohash;
  while (c != 0) {
    *ans = ((*ans << 5) + *ans) + c; /* hash * 33 + c */
    tohash++;
    c = *tohash;
  }
  return NULL;
}

int main(int argc, char *argv[])
{
  MPI_Init(&argc, &argv);
  int rank;
  int size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  //Let's Allocate a memory buffer
  uint64_t* buf = calloc(1, sizeof(uint64_t));
  //one writes data in it
  if(rank==0){
    *buf=size;
  }

  MPI_Barrier(MPI_COMM_WORLD);
  //everyone reads from it.
  printf("[%d] The value in the shared buffer is: %" PRIu64"\n", rank, *buf);

  MPI_Barrier(MPI_COMM_WORLD);
  //Try SMPI_SHARED_CALL function, which should call hash only once and for all.
  static const char str[] = "onceandforall";
  if(rank==size-1){
    SMPI_SHARED_CALL(hash,str,str,buf);
  }

  MPI_Barrier(MPI_COMM_WORLD);

  printf("[%d] After change, the value in the shared buffer is: %" PRIu64"\n", rank, *buf);

  free(buf);

  MPI_Finalize();
  return 0;
}
