/* Copyright (c) 2002-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Code with both recursive and non-recursive mutexes */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// Structure to hold the mutex's name and pointer to the actual mutex
typedef struct {
  const char* name;
  pthread_mutex_t* mutex;
} ThreadData;

static void* thread_function(void* arg)
{
  ThreadData* data       = (ThreadData*)arg;
  pthread_mutex_t* mutex = data->mutex;
  const char* name       = data->name;

  pthread_mutex_lock(mutex);
  fprintf(stderr, "Got the lock on the %s mutex.\n", name);

  // Attempt to relock the mutex - This behavior depends on the mutex type
  if (pthread_mutex_trylock(mutex) == 0) {
    fprintf(stderr, "Got the lock again on the %s mutex.\n", name);
    pthread_mutex_unlock(mutex);
  } else {
    fprintf(stderr, "Failed to relock the %s mutex.\n", name);
  }

  pthread_mutex_unlock(mutex);

  // pthread_exit(NULL); TODO: segfaulting
  return NULL;
}

int main()
{
  pthread_t thread1;
  pthread_t thread2;
  pthread_mutex_t mutex_dflt = PTHREAD_MUTEX_INITIALIZER; // Non-recursive mutex

  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_t mutex_rec;
  pthread_mutex_init(&mutex_rec, &attr);

  ThreadData data1 = {"default", &mutex_dflt};
  ThreadData data2 = {"recursive", &mutex_rec};

  pthread_create(&thread1, NULL, thread_function, &data1);
  pthread_create(&thread2, NULL, thread_function, &data2);

  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);

  pthread_mutex_destroy(&mutex_dflt);
  pthread_mutex_destroy(&mutex_rec);

  return 0;
}
