/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Race-free program: the main thread writes x before creating the thread that reads it.
 *
 * The write is done before the first simcall of the main thread, and must be attributed to this first simcall. When the
 * exploration replays the beginning of the execution to explore another interleaving, this write must not be sent
 * again with a later transition of the main thread, or it would seem to race with the read. */

#include <pthread.h>

int x = 0;

static void* reader(void* arg)
{
  return (void*)(long)x;
}

static void* do_nothing(void* arg)
{
  return NULL;
}

int main()
{
  x = 42;
  pthread_t t1;
  pthread_t t2;
  pthread_create(&t1, NULL, reader, NULL);
  pthread_create(&t2, NULL, do_nothing, NULL);
  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  return 0;
}
