/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <pthread.h>
#include <unistd.h>

pthread_mutex_t mutex;
pthread_cond_t cond;
pthread_t thread;

static void* thread_doit(void* unused)
{
  pthread_mutex_lock(&mutex);
  pthread_cond_wait(&cond, &mutex);
  pthread_mutex_unlock(&mutex);
  return NULL;
}

int main(int argc, char* argv[])
{

  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&cond, NULL);

  pthread_create(&thread, NULL, &thread_doit, NULL);

  pthread_mutex_lock(&mutex);
  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&mutex);

  pthread_join(thread, NULL);

  return 0;
}
