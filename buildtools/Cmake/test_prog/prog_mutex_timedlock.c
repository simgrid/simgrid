/* Copyright (c) 2010-2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <pthread.h>

int main(void)
{
  pthread_mutex_t s;
  const struct timespec t;
  sem_timedlock(&s, &t);
  return 0;
}
