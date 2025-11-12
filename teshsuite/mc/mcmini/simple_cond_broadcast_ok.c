/* Copyright (C) 2022, Maxwell Pirtle, Luka Jovanovic, Gene Cooperman
 * (pirtle.m@northeastern.edu, jovanovic.l@northeastern.edu, gene@ccs.neu.edu)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

pthread_mutex_t mutex;
sem_t sem_start, sem_end;

static void* thread_doit(void* unused)
{
  sem_post(&sem_start); // Increment sem_start, signaling this thread is ready
  pthread_mutex_lock(&mutex);
  sem_wait(&sem_end); // Wait until main thread signals to proceed
  pthread_mutex_unlock(&mutex);
  return NULL;
}

int main(int argc, char* argv[])
{
  if (argc < 2) {
    printf("Expected usage: %s THREAD_NUM\n", argv[0]);
    return -1;
  }

  int THREAD_NUM = atoi(argv[1]);

  pthread_t* threads = malloc(sizeof(pthread_t) * THREAD_NUM);

  pthread_mutex_init(&mutex, NULL);
  sem_init(&sem_start, 0, 0); // Initialize semaphore sem_start to 0
  sem_init(&sem_end, 0, 0);   // Initialize semaphore sem_end to 0

  for (int i = 0; i < THREAD_NUM; i++) {
    pthread_create(&threads[i], NULL, &thread_doit, NULL);
  }

  // Wait for all threads to be ready
  for (int i = 0; i < THREAD_NUM; i++) {
    sem_wait(&sem_start);
  }

  // Signal all threads to proceed
  for (int i = 0; i < THREAD_NUM; i++) {
    sem_post(&sem_end);
  }

  // Wait for all threads to complete
  for (int i = 0; i < THREAD_NUM; i++) {
    pthread_join(threads[i], NULL);
  }

  free(threads);
  pthread_mutex_destroy(&mutex);
  sem_destroy(&sem_start);
  sem_destroy(&sem_end);

  return 0;
}
