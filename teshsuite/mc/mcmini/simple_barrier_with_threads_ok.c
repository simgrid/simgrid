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

#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

int DEBUG;

pthread_barrier_t barrier;

static void * thread_doit(void *t)
{
    int *tid = (int*)t;
    if(DEBUG) {
        printf("Thread %d: Waiting at barrier\n", *tid);
    }
    pthread_barrier_wait(&barrier);
    if(DEBUG) {
        printf("Thread %d: Crossed barrier\n", *tid);
    }
    return NULL;
}

int main(int argc, char* argv[])
{
    if(argc < 3) {
        printf("Expected usage: %s THREAD_NUM DEBUG_FLAG\n", argv[0]);
        printf("DEBUG_FLAG: 0 - Don't display debug information, 1 - Display debug information\n");
        return -1;
    }

    int THREAD_NUM = atoi(argv[1]);
    DEBUG = atoi(argv[2]);

    pthread_t *threads = malloc(sizeof(pthread_t) * THREAD_NUM);

    pthread_barrier_init(&barrier, NULL, THREAD_NUM);

    int *tids = malloc(sizeof(int) * THREAD_NUM);
    for(int i = 0; i < THREAD_NUM; i++) {
        tids[i] = i;
        pthread_create(&threads[i], NULL, &thread_doit, &tids[i]);
    }

    for(int i = 0; i < THREAD_NUM; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    free(tids);
    pthread_barrier_destroy(&barrier);
    
    return 0;
}
