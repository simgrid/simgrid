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

// Dining philosophers solution with semaphores

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>

struct forks {
    int philosopher;
    pthread_mutex_t *left_fork;
    pthread_mutex_t *right_fork;
    sem_t* sem_dining;
    int DEBUG;
} *forks;

static void * philosopher_doit(void *forks_arg) {
    struct forks *forks = forks_arg;
    sem_wait(forks->sem_dining);
    pthread_mutex_lock(forks->left_fork);
    pthread_mutex_lock(forks->right_fork);

    if(forks->DEBUG) printf("Philosopher %d just ate.\n", forks->philosopher);
    
    pthread_mutex_unlock(forks->left_fork);
    pthread_mutex_unlock(forks->right_fork);
    sem_post(forks->sem_dining);
    return NULL;
}

int main(int argc, char* argv[]) {
    if(argc != 3){
        printf("Usage: %s NUM_PHILOSOPHERS DEBUG\n", argv[0]);
        return 1;
    }

    int NUM_THREADS = atoi(argv[1]);
    int DEBUG = atoi(argv[2]);

    pthread_t thread[NUM_THREADS];
    pthread_mutex_t mutex_resource[NUM_THREADS];
    sem_t sem_dining;
    sem_init(&sem_dining, 0, NUM_THREADS);

    forks = malloc(NUM_THREADS * sizeof(struct forks));

    int i;
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_mutex_init(&mutex_resource[i], NULL);
        forks[i] = (struct forks){i,
                                  &mutex_resource[i],
                                  &mutex_resource[(i+1) % NUM_THREADS],
                                  &sem_dining,
                                  DEBUG};
    }

    for (i = 0; i < NUM_THREADS; i++) {
        pthread_create(&thread[i], NULL, &philosopher_doit, &forks[i]);
    }

    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(thread[i], NULL);
    }

    free(forks);
    return 0;
}
