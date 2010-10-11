/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <semaphore.h>

int main()
{
  sem_t *s;
  const struct timespec *t;
  sem_timedwait(s, t);
}
