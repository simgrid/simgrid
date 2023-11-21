/* Copyright (c) 2002-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Simple producer/consumer example with pthreads and semaphores */

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int AmountProduced = 3; /* Amount of items produced by a producer */
int AmountConsumed = 3; /* Amount of items consumed by a consumer */
int ProducerCount  = 2; /* Amount of producer threads*/
int ConsumerCount  = 2; /* Amount of consumer threads*/
int BufferSize     = 4; /* Size of the buffer */

sem_t empty;
sem_t full;
int in  = 0;
int out = 0;
int* buffer;
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
    if (do_output) {
      int item = buffer[out];
      fprintf(stderr, "Consumer %d: Remove Item %d from %d\n", *((int*)id), item, out);
    }
    out = (out + 1) % BufferSize;
    pthread_mutex_unlock(&mutex);
    sem_post(&empty);
  }
  return NULL;
}

int main(int argc, char** argv)
{
  int opt;
  while ((opt = getopt(argc, argv, "c:C:p:P:q")) != -1) {
    switch (opt) {
      case 'q':
        do_output = 0;
        break;
      case 'c':
        AmountConsumed = atoi(optarg);
        break;
      case 'C':
        ConsumerCount = atoi(optarg);
        break;
      case 'p':
        AmountProduced = atoi(optarg);
        break;
      case 'P':
        ProducerCount = atoi(optarg);
        break;
      default: /* '?' */
        printf("unknown option: %c\n", optopt);
        break;
    }
  }
  pthread_t* pro = malloc(ProducerCount * sizeof(pthread_t));
  pthread_t* con = malloc(ConsumerCount * sizeof(pthread_t));
  buffer         = malloc(sizeof(int) * BufferSize);
  pthread_mutex_init(&mutex, NULL);
  sem_init(&empty, 0, BufferSize);
  sem_init(&full, 0, 0);

  int* ids = malloc(sizeof(int) * (ProducerCount + ConsumerCount));
  for (int i = 0; i < ProducerCount + ConsumerCount; i++)
    ids[i] = i + 1; // The identity of each thread (for debug messages)

  for (int i = 0; i < ProducerCount; i++)
    pthread_create(&pro[i], NULL, producer, &ids[i]);
  for (int i = 0; i < ConsumerCount; i++)
    pthread_create(&con[i], NULL, consumer, &ids[i]);

  for (int i = 0; i < ProducerCount; i++)
    pthread_join(pro[i], NULL);
  for (int i = 0; i < ConsumerCount; i++)
    pthread_join(con[i], NULL);

  pthread_mutex_destroy(&mutex);
  sem_destroy(&empty);
  sem_destroy(&full);
  free(pro);
  free(con);
  free(buffer);
  free(ids);

  return 0;
}
