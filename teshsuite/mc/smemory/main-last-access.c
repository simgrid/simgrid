/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Racy program: the main thread reads x after its last synchronization, while a thread that it never joins writes x.
 *
 * The read is done after the last simcall of the main thread: it must be checked anyway. */

#include <pthread.h>

int x = 0;

static void* writer(void* arg)
{
  x = 1;
  return NULL;
}

static void* do_nothing(void* arg)
{
  return NULL;
}

int main()
{
  pthread_t t1;
  pthread_t t2;
  pthread_create(&t1, NULL, writer, NULL);
  pthread_create(&t2, NULL, do_nothing, NULL);
  pthread_join(t2, NULL);
  return x;
}
