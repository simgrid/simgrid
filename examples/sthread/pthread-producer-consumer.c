/* Copyright (c) 2002-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Simple producer/consumer example with pthreads and semaphores */

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AmountProduced 3 /* Amount of items produced by a producer */
#define AmountConsumed 3 /* Amount of items consumed by a consumer */
#define ProducerCount 2  /* Amount of producer threads*/
#define ConsumerCount 2  /* Amount of consumer threads*/
#define BufferSize 4     /* Size of the buffer */

sem_t empty;
sem_t full;
int in  = 0;
int out = 0;
int buffer[BufferSize];
pthread_mutex_t mutex;
int do_output = 1;

static void* producer(void* id)
{
  for (int i = 0; i < AmountProduced; i++) {
    sem_wait(&empty);
    pthread_mutex_lock(&mutex);
    buffer[in] = i;
    if (do_output)
      fprintf(stderr, "Producer %d: Insert Item %d at %d\n", *((int*)id), buffer[in], in);
    in = (in + 1) % BufferSize;
    pthread_mutex_unlock(&mutex);
    sem_post(&full);
  }
  return NULL;
}
static void* consumer(void* id)
{
  for (int i = 0; i < AmountConsumed; i++) {
    sem_wait(&full);
    pthread_mutex_lock(&mutex);
    int item = buffer[out];
    if (do_output)
      fprintf(stderr, "Consumer %d: Remove Item %d from %d\n", *((int*)id), item, out);
    out = (out + 1) % BufferSize;
    pthread_mutex_unlock(&mutex);
    sem_post(&empty);
  }
  return NULL;
}

int main(int argc, char** argv)
{
  if (argc == 2 && strcmp(argv[1], "-q") == 0)
    do_output = 0;
  pthread_t pro[2], con[2];
  pthread_mutex_init(&mutex, NULL);
  sem_init(&empty, 0, BufferSize);
  sem_init(&full, 0, 0);

  int ids[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}; // The identity of each thread (for debug messages)

  for (int i = 0; i < ProducerCount; i++)
    pthread_create(&pro[i], NULL, (void*)producer, (void*)&ids[i]);
  for (int i = 0; i < ConsumerCount; i++)
    pthread_create(&con[i], NULL, (void*)consumer, (void*)&ids[i]);

  for (int i = 0; i < ProducerCount; i++)
    pthread_join(pro[i], NULL);
  for (int i = 0; i < ConsumerCount; i++)
    pthread_join(con[i], NULL);

  pthread_mutex_destroy(&mutex);
  sem_destroy(&empty);
  sem_destroy(&full);

  return 0;
}