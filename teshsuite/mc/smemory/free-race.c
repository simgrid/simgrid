/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Racy program: a thread frees a block while another thread reads it, without any synchronization.
 *
 * Freeing a block writes all of it, so this race must be detected as a race between a read and a write. */

#include <pthread.h>
#include <stdlib.h>

int* shared;

static void* reader(void* arg)
{
  return (void*)(long)*shared;
}

static void* freer(void* arg)
{
  free(shared);
  return NULL;
}

int main()
{
  shared  = malloc(sizeof(int));
  *shared = 1;
  pthread_t t1;
  pthread_t t2;
  pthread_create(&t1, NULL, reader, NULL);
  pthread_create(&t2, NULL, freer, NULL);
  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  return 0;
}
