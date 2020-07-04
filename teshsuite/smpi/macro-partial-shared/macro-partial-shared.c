/* Copyright (c) 2017-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <mpi.h>
#include <stdint.h>
#include <inttypes.h>

// Set the elements between buf[start] and buf[stop-1] to (i+value)%256
static void set(uint8_t *buf, size_t start, size_t stop, uint8_t value) {
  for(size_t i = start; i < stop; i++) {
    buf[i] = (i+value)%256;
  }
}

// Return the number of times that an element is equal to (i+value)%256 between buf[start] and buf[stop-1].
static size_t count_all(const uint8_t* buf, size_t start, size_t stop, uint8_t value)
{
  size_t occ = 0;
  for(size_t i = start ; i < stop ; i++) {
    if(buf[i] == (i+value)%256) {
      occ ++;
    }
  }
  return occ;
}

// Return true iff the values from buf[start] to buf[stop-1] are all equal to (i+value)%256.
static int check_all(const uint8_t* buf, size_t start, size_t stop, uint8_t value)
{
  size_t occ = count_all(buf, start, stop, value);
  return occ == stop-start;
}

// Return true iff "enough" elements are equal to (i+value)%256 between buf[start] and buf[stop-1].
static int check_enough(const uint8_t* buf, size_t start, size_t stop, uint8_t value)
{
  int page_size = 0x1000;
  size_t size = stop-start;
  if(size <= 2*page_size) // we are not sure to have a whole page that is shared
    return 1;
  size_t occ = count_all(buf, start, stop, value);
  return occ >= size - 2*page_size;
}

int main(int argc, char *argv[])
{
  MPI_Init(&argc, &argv);
  int rank;
  int size;
  size_t mem_size = 0x1000000;
  size_t shared_blocks[] = {
    0,         0x123456,
    0x130000, 0x130001,
    0x345678, 0x345789,
    0x444444, 0x555555,
    0x555556, 0x560000,
    0x800000, 0x1000000
  };
  int nb_blocks = (sizeof(shared_blocks)/sizeof(size_t))/2;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  //Let's Allocate a shared memory buffer
  uint8_t *buf;
  buf = SMPI_PARTIAL_SHARED_MALLOC(mem_size, shared_blocks, nb_blocks);
  set(buf, 0, mem_size, 0);
  MPI_Barrier(MPI_COMM_WORLD);

  // Process 0 write in shared blocks
  if(rank == 0) {
    for(int i = 0; i < nb_blocks; i++) {
      size_t start = shared_blocks[2*i];
      size_t stop = shared_blocks[2*i+1];
      set(buf, start, stop, 42);
    }
  }
  MPI_Barrier(MPI_COMM_WORLD);
  // All processes check that their shared blocks have been written (at least partially)
  for(int i = 0; i < nb_blocks; i++) {
    size_t start = shared_blocks[2*i];
    size_t stop = shared_blocks[2*i+1];
    int is_shared = check_enough(buf, start, stop, 42);
    printf("[%d] The result of the shared check for block (0x%zx, 0x%zx) is: %d\n", rank, start, stop, is_shared);
  }


  // Check the private blocks
  MPI_Barrier(MPI_COMM_WORLD);
  for(int i = 0; i < nb_blocks-1; i++) {
    size_t start = shared_blocks[2*i+1];
    size_t stop = shared_blocks[2*i+2];
    int is_private = check_all(buf, start, stop, 0);
    printf("[%d] The result of the private check for block (0x%zx, 0x%zx) is: %d\n", rank, start, stop, is_private);
  }

  SMPI_SHARED_FREE(buf);

  MPI_Finalize();
  return 0;
}
