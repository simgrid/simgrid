/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Two threads take a mutex in turn. Between two of its critical sections, the main thread writes a byte that depends
 * on the pid, so that the replays of the exploration, done in other processes, do not access the same memory as the
 * first execution. With the argument "same", the byte does not depend on the pid. */

#include <pthread.h>
#include <string.h>
#include <unistd.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static char bytes[64];
static int counter;

static void* worker(void* arg)
{
  pthread_mutex_lock(&mutex);
  counter++;
  pthread_mutex_unlock(&mutex);
  return arg;
}

int main(int argc, char* argv[])
{
  int deterministic = argc > 1 && strcmp(argv[1], "same") == 0;
  pthread_t thread;
  pthread_create(&thread, NULL, worker, NULL);
  pthread_mutex_lock(&mutex);
  counter++;
  pthread_mutex_unlock(&mutex);
  bytes[deterministic ? 0 : getpid() % 64] = 1;
  for (int i = 0; i < 2; i++) {
    pthread_mutex_lock(&mutex);
    counter++;
    pthread_mutex_unlock(&mutex);
  }
  pthread_join(thread, NULL);
  return 0;
}
