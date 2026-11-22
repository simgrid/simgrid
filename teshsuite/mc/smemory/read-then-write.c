/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Racy program: a thread reads x while another thread writes it, without any synchronization.
 *
 * In the first explored execution, the read happens before the write. The race must be detected anyway, which
 * requires to remember the reads and not only the writes. */

#include <pthread.h>

int x = 0;

static void* reader(void* arg)
{
  return (void*)(long)x;
}

static void* writer(void* arg)
{
  x = 1;
  return NULL;
}

int main()
{
  pthread_t t1;
  pthread_t t2;
  pthread_create(&t1, NULL, reader, NULL);
  pthread_create(&t2, NULL, writer, NULL);
  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  return 0;
}
