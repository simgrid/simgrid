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
#include <stdlib.h>
#include <stdio.h>

int THREAD_NUM; 
int DEBUG = 0;

pthread_barrier_t barrier;
pthread_t *thread;

static void * thread_doit(void *unused)
{
    if(DEBUG) printf("Thread %lu: Waiting at barrier\n", (unsigned long)pthread_self());
    pthread_barrier_wait(&barrier);
    if(DEBUG) printf("Thread %lu: Passed the barrier\n", (unsigned long)pthread_self());
    return NULL;
}

int main(int argc, char* argv[]) {
    if(argc != 3){
        printf("Usage: %s THREAD_NUM DEBUG_FLAG\n", argv[0]);
        return 1;
    }

    THREAD_NUM = atoi(argv[1]);
    DEBUG = atoi(argv[2]);

    thread = (pthread_t*) malloc(THREAD_NUM * sizeof(pthread_t));

    pthread_barrier_init(&barrier, NULL, THREAD_NUM);
    for(int i = 0; i < THREAD_NUM; i++) {
        pthread_create(&thread[i], NULL, &thread_doit, NULL);
    }

    pthread_barrier_wait(&barrier);

    for(int i = 0; i < THREAD_NUM; i++) {
        pthread_join(thread[i], NULL);
    }

    free(thread);
    return 0;
}
