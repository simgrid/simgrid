/* Copyright (c) 2002-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Simple test code that may deadlock:

   Thread 1 locks mutex1 then mutex2 while thread 2 locks in reverse order.
   Deadlock occurs when each thread get one mutex and then ask for the other one.
 */

#include <pthread.h>
#include <stdio.h>

pthread_mutex_t mutex1;
pthread_mutex_t mutex2;

static void* thread_fun1(void* val)
{
  pthread_mutex_lock(&mutex1);
  pthread_mutex_lock(&mutex2);
  pthread_mutex_unlock(&mutex1);
  pthread_mutex_unlock(&mutex2);

  fprintf(stderr, "The thread %d is terminating.\n", *(int*)val);
  return NULL;
}
static void* thread_fun2(void* val)
{
  pthread_mutex_lock(&mutex2);
  pthread_mutex_lock(&mutex1);
  pthread_mutex_unlock(&mutex1);
  pthread_mutex_unlock(&mutex2);

  fprintf(stderr, "The thread %d is terminating.\n", *(int*)val);
  return NULL;
}

int main(int argc, char* argv[])
{
  pthread_mutex_init(&mutex1, NULL);
  pthread_mutex_init(&mutex2, NULL);

  int id[2] = {0, 1};
  pthread_t thread1;
  pthread_t thread2;
  pthread_create(&thread1, NULL, thread_fun1, &id[0]);
  pthread_create(&thread2, NULL, thread_fun2, &id[1]);
  fprintf(stderr, "All threads are started.\n");
  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);

  pthread_mutex_destroy(&mutex1);
  pthread_mutex_destroy(&mutex2);

  fprintf(stderr, "User's main is terminating.\n");
  return 0;
}
