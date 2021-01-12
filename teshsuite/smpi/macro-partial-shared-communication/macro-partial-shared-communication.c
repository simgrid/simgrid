/* Copyright (c) 2017-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <mpi.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>

// Set the elements between buf[start] and buf[stop-1] to (i+value)%256
static void set(uint8_t *buf, size_t start, size_t stop, uint8_t value) {
  for(size_t i = start; i < stop; i++) {
    buf[i] = (uint8_t)((i + value) % 256);
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
  assert(size%2 == 0);
  uint8_t *buf;
  buf = SMPI_PARTIAL_SHARED_MALLOC(mem_size, shared_blocks, nb_blocks);
  memset(buf, rank, mem_size);
  MPI_Barrier(MPI_COMM_WORLD);

  // Even processes write their rank in private blocks
  if(rank%2 == 0) {
    for(int i = 0; i < nb_blocks-1; i++) {
      size_t start = shared_blocks[2*i+1];
      size_t stop = shared_blocks[2*i+2];
      set(buf, start, stop, (uint8_t)rank);
    }
  }
  // Then, even processes send their buffer to their successor
  if(rank%2 == 0) {
    MPI_Send(buf, (int)mem_size, MPI_UINT8_T, rank + 1, 0, MPI_COMM_WORLD);
  }
  else {
    MPI_Recv(buf, (int)mem_size, MPI_UINT8_T, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  }


  // Odd processes verify that they successfully received the message
  if(rank%2 == 1) {
    for(int i = 0; i < nb_blocks-1; i++) {
      size_t start = shared_blocks[2*i+1];
      size_t stop = shared_blocks[2*i+2];
      int comm     = check_all(buf, start, stop, (uint8_t)(rank - 1));
      printf("[%d] The result of the (normal) communication check for block (0x%zx, 0x%zx) is: %d\n", rank, start, stop, comm);
    }
    memset(buf, rank, mem_size);
  }

  MPI_Barrier(MPI_COMM_WORLD);

  // Then, even processes send a sub-part of their buffer their successor
  // Note that the last block should not be copied entirely
  if(rank%2 == 0) {
    MPI_Send(buf + 0x10000, (int)(mem_size - 0xa00000), MPI_UINT8_T, rank + 1, 0, MPI_COMM_WORLD);
  }
  else {
    MPI_Recv(buf + 0x10000, (int)(mem_size - 0xa00000), MPI_UINT8_T, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  }


  // Odd processes verify that they successfully received the message
  if(rank%2 == 1) {
    for(int i = 0; i < nb_blocks-1; i++) {
      size_t start = shared_blocks[2*i+1];
      size_t stop = shared_blocks[2*i+2];
      int comm     = check_all(buf, start, stop, (uint8_t)(rank - 1));
      printf("[%d] The result of the (shifted) communication check for block (0x%zx, 0x%zx) is: %d\n", rank, start, stop, comm);
    }
  }

  SMPI_SHARED_FREE(buf);

  MPI_Finalize();
  return 0;
}
