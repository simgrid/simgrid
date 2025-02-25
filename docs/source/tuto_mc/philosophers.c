/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

// Naive dining philosophers solution, which leads to deadlock.

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int DEBUG = 0;

struct forks {
  int philosopher;
  pthread_mutex_t* left_fork;
  pthread_mutex_t* right_fork;
}* forks;

static void* philosopher_code(void* forks_arg)
{
  struct forks* forks = forks_arg;
  pthread_mutex_lock(forks->left_fork);
  pthread_mutex_lock(forks->right_fork);

  if (DEBUG)
    printf("Philosopher %d just ate.\n", forks->philosopher);

  pthread_mutex_unlock(forks->left_fork);
  pthread_mutex_unlock(forks->right_fork);
  return NULL;
}

int main(int argc, char* argv[])
{
  if (argc != 3) {
    printf("Usage: %s NUM_PHILOSOPHERS DEBUG\n", argv[0]);
    return 1;
  }

  int NUM_THREADS = atoi(argv[1]);
  DEBUG           = atoi(argv[2]);

  pthread_t thread[NUM_THREADS];
  pthread_mutex_t mutex_resource[NUM_THREADS];

  forks = malloc(NUM_THREADS * sizeof(struct forks));

  int i;
  for (i = 0; i < NUM_THREADS; i++) {
    pthread_mutex_init(&mutex_resource[i], NULL);
    forks[i] = (struct forks){i, &mutex_resource[i], &mutex_resource[(i + 1) % NUM_THREADS]};
  }

  for (i = 0; i < NUM_THREADS; i++) {
    pthread_create(&thread[i], NULL, &philosopher_code, &forks[i]);
  }

  for (i = 0; i < NUM_THREADS; i++) {
    pthread_join(thread[i], NULL);
  }

  free(forks);
  return 0;
}
