/* Copyright (c) 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example should be instructive to learn about SMPI_SHARED_CALL */

#include <stdio.h>
#include <mpi.h>
#include <stdint.h>
#include <inttypes.h>

// Return the number of occurences of the given value between buf[start] and buf[stop-1].
int count_all(uint8_t *buf, int start, int stop, uint8_t value) {
  int occ = 0;
  for(int i = start ; i < stop ; i++) {
    if(buf[i] == value) {
      occ ++;
    }
  }
  return occ;
}

// Return true iff the values from buf[start] to buf[stop-1] are all equal to value.
int check_all(uint8_t *buf, int start, int stop, uint8_t value) {
  int occ = count_all(buf, start, stop, value);
  return occ == stop-start;
}

// Return true iff "enough" occurences of the given value are between buf[start] and buf[stop-1].
int check_enough(uint8_t *buf, int start, int stop, uint8_t value) {
  int page_size = 0x1000;
  int size = stop-start;
  if(size <= 2*page_size) // we are not sure to have a whole page that is shared
    return 1;
  int occ = count_all(buf, start, stop, value);
  return occ >= size - 2*page_size;
}

int main(int argc, char *argv[])
{
  MPI_Init(&argc, &argv);
  int rank;
  int size;
  int mem_size = 0x10000000;
  int shared_blocks[] = {
    0,         0x1234567,
    0x1300000, 0x1300010,
    0x3456789, 0x3457890,
    0x4444444, 0x5555555,
    0x5555565, 0x5600000,
    0x8000000, 0x10000000
  };
  int nb_blocks = (sizeof(shared_blocks)/sizeof(int))/2;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  //Let's Allocate a shared memory buffer
  uint8_t *buf;
  buf = SMPI_PARTIAL_SHARED_MALLOC(mem_size, shared_blocks, nb_blocks);
  memset(buf, 0, mem_size);
  MPI_Barrier(MPI_COMM_WORLD);

  // Process 0 write in shared blocks
  if(rank == 0) {
    for(int i = 0; i < nb_blocks; i++) {
      int start = shared_blocks[2*i];
      int stop = shared_blocks[2*i+1];
      memset(buf+start, 42, stop-start);
    }
  }
  MPI_Barrier(MPI_COMM_WORLD);
  // All processes check that their shared blocks have been written (at least partially)
  for(int i = 0; i < nb_blocks; i++) {
    int start = shared_blocks[2*i];
    int stop = shared_blocks[2*i+1];
    int is_shared = check_enough(buf, start, stop, 42);
    printf("[%d] The result of the shared check for block (0x%x, 0x%x) is: %d\n", rank, start, stop, is_shared);
  }


  // Check the private blocks
  MPI_Barrier(MPI_COMM_WORLD);
  for(int i = 0; i < nb_blocks-1; i++) {
    int start = shared_blocks[2*i+1];
    int stop = shared_blocks[2*i+2];
    int is_private = check_all(buf, start, stop, 0);
    printf("[%d] The result of the private check for block (0x%x, 0x%x) is: %d\n", rank, start, stop, is_private);
  }

  SMPI_SHARED_FREE(buf);

  MPI_Finalize();
  return 0;
}
