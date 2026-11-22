/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Racy program: two threads write the same byte of a large block, which is mapped after the first memory accesses.
 *
 * The memory mapped late in the execution must be observed as the rest. */

#include <pthread.h>
#include <stdlib.h>

int first_access = 0;
char* block;

static void* writer(void* arg)
{
  block[0] = 1;
  return NULL;
}

int main()
{
  first_access = 1;
  block        = malloc(1 << 20); // Large enough to be mapped apart from the rest of the heap
  pthread_t t1;
  pthread_t t2;
  pthread_create(&t1, NULL, writer, NULL);
  pthread_create(&t2, NULL, writer, NULL);
  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  return 0;
}
