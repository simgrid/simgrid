/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Racy program: a thread copies a structure while another thread writes one of its fields.
 *
 * The copy is a single access to the whole structure (a memcpy), which starts before the field. The race must be
 * detected on the bytes of the field. */

#include <pthread.h>

struct point {
  int coords[4];
};
struct point shared;
struct point copy;

static void* copier(void* arg)
{
  copy = shared;
  return NULL;
}

static void* writer(void* arg)
{
  shared.coords[2] = 1;
  return NULL;
}

int main()
{
  pthread_t t1;
  pthread_t t2;
  pthread_create(&t1, NULL, copier, NULL);
  pthread_create(&t2, NULL, writer, NULL);
  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  return 0;
}
