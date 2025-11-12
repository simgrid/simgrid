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

pthread_mutex_t mutex;
pthread_cond_t cond;
pthread_t thread;
sem_t start;

static void* thread_doit(void* unused)
{
  pthread_mutex_lock(&mutex);
  sem_post(&start);
  pthread_cond_wait(&cond, &mutex);
  pthread_mutex_unlock(&mutex);
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
  sem_init(&start, 0, 0);
  if (pthread_cond_init(&cond, NULL) != 0) {
    printf("Failed to initialize cond\n");
    return EXIT_FAILURE;
  }

  if (pthread_create(&thread, NULL, &thread_doit, NULL) != 0) {
    printf("Failed to create thread\n");
    return EXIT_FAILURE;
  }

  // Wait for the thread to get to the condition variable
  sem_wait(&start);

  pthread_mutex_lock(&mutex);
  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&mutex);

  if (pthread_join(thread, NULL) != 0) {
    printf("Failed to join thread\n");
    return EXIT_FAILURE;
  }

  pthread_mutex_destroy(&mutex);
  sem_destroy(&start);
  pthread_cond_destroy(&cond);

  return EXIT_SUCCESS;
}
