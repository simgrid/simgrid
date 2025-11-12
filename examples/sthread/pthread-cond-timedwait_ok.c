/* Copyright (c) 2002-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Simple test code with no bug  */

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

pthread_mutex_t mutex;
pthread_cond_t cond;
pthread_t thread;

static void* thread_doit(void* unused)
{
  // Try twice to get signaled
  for (int i = 0; i < 2; i++) {
    struct timespec time_to_wait = {time(NULL), 0};
    time_to_wait.tv_sec += 1;

    printf("Thread: Wait for 1s\n");
    pthread_mutex_lock(&mutex);
    int res = pthread_cond_timedwait(&cond, &mutex, &time_to_wait);
    pthread_mutex_unlock(&mutex);
    if (res == 0) {
      printf("Thread: got signaled!\n");
    } else {
      printf("Thread: waiting the signal timeouted!\n");
    }
  }
  return NULL;
}

int main(int argc, char* argv[])
{
  if (argc > 1) {
    fprintf(stderr, "Usage: %s\n", argv[0]);
    return EXIT_FAILURE;
  }

  if (pthread_mutex_init(&mutex, NULL) != 0) {
    printf("Failed to initialize mutex\n");
    return EXIT_FAILURE;
  }
  if (pthread_cond_init(&cond, NULL) != 0) {
    printf("Failed to initialize cond\n");
    return EXIT_FAILURE;
  }

  if (pthread_create(&thread, NULL, &thread_doit, NULL) != 0) {
    printf("Failed to create thread\n");
    return EXIT_FAILURE;
  }

  printf("main: sleep 5ms\n");
  usleep(5 * 1000);

  pthread_mutex_lock(&mutex);
  printf("main: signal the condition\n");
  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&mutex);

  if (pthread_join(thread, NULL) != 0) {
    printf("Failed to join thread\n");
    return EXIT_FAILURE;
  }

  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&cond);

  return EXIT_SUCCESS;
}
