/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Race-free program: two threads write at the same offset of two different memory pages.
 *
 * The race detector stores the accesses page by page. The two writes must not be confused. */

#include <pthread.h>

_Alignas(4096) char page1[4096];
_Alignas(4096) char page2[4096];

static void* write_page1(void* arg)
{
  page1[8] = 1;
  return NULL;
}

static void* write_page2(void* arg)
{
  page2[8] = 1;
  return NULL;
}

int main()
{
  pthread_t t1;
  pthread_t t2;
  pthread_create(&t1, NULL, write_page1, NULL);
  pthread_create(&t2, NULL, write_page2, NULL);
  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  return 0;
}
