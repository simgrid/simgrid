/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Racy program: two threads write a local variable of the main thread, whose address they get as parameter.
 *
 * The local variables whose address escapes must be observed, even if they live on a stack. */

#include <pthread.h>

static void* writer(void* arg)
{
  *(int*)arg = 1;
  return NULL;
}

int main()
{
  int local = 0;
  pthread_t t1;
  pthread_t t2;
  pthread_create(&t1, NULL, writer, &local);
  pthread_create(&t2, NULL, writer, &local);
  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  return local;
}
