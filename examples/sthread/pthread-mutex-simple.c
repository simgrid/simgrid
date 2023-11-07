/* Copyright (c) 2002-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Simple test code with no bug  */

#include <pthread.h>
#include <stdio.h>

pthread_mutex_t mutex;

static void* thread_fun(void* val)
{
  pthread_mutex_lock(&mutex);
  pthread_mutex_unlock(&mutex);

  fprintf(stderr, "The thread %d is terminating.\n", *(int*)val);
  return NULL;
}

int main(int argc, char* argv[])
{
  pthread_mutex_init(&mutex, NULL);

  int id[2] = {0, 1};
  pthread_t thread1;
  pthread_t thread2;
  pthread_create(&thread1, NULL, thread_fun, &id[0]);
  pthread_create(&thread2, NULL, thread_fun, &id[1]);
  fprintf(stderr, "All threads are started.\n");
  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);

  pthread_mutex_destroy(&mutex);

  fprintf(stderr, "User's main is terminating.\n");
  return 0;
}
