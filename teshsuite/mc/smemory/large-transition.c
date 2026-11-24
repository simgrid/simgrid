/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* A thread writes many separate bytes before racing with the main thread, so that the transition ending its segment
 * carries a large memory trace. Its replay must not block while passing this trace to the race detector. */

#include <pthread.h>
#include <stdlib.h>

static char* big;
static int shared;

static void* worker(void* arg)
{
  unsigned index = 1;
  for (int i = 0; i < 300000; i++) { /* scattered bytes, so that the accesses are not merged */
    index      = (index * 1103515245 + 12345) % (1 << 20);
    big[index] = 1;
  }
  shared = 1;
  return arg;
}

int main(void)
{
  pthread_t thread;
  big = malloc(1 << 20);
  pthread_create(&thread, NULL, worker, NULL);
  shared = 2;
  pthread_join(thread, NULL);
  free(big);
  return 0;
}
