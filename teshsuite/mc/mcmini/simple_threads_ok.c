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
#include <unistd.h>
#include <stdio.h>

static void * thread_doit(void *unused) {
    int len = (int) ((drand48() * 5) + 1);
    sleep(len);
    return NULL;
}

int main(int argc, char* argv[]) {
    if(argc < 2) {
        printf("Expected usage: %s THREAD_NUM\n", argv[0]);
        return -1;
    }

    int thread_num = atoi(argv[1]);

    pthread_t *threads = malloc(sizeof(pthread_t) * thread_num);

    for(int i = 0; i < thread_num; i++) {
        pthread_create(&threads[i], NULL, &thread_doit, NULL);
    }

    for(int i = 0; i < thread_num; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);

    return 0;
}
