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
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>

sem_t sem;

int main(int argc, char* argv[]) {
    if(argc < 2) {
        printf("Expected usage: %s START_NUM\n", argv[0]);
        return -1;
    }

    int start_num = atoi(argv[1]);

    sem_init(&sem, 0, start_num);

    for(int i = 0; i < start_num; i++) {
        sem_wait(&sem);
    }

    sem_post(&sem);
    sem_post(&sem);
    sem_wait(&sem);
    sem_wait(&sem);

    sem_destroy(&sem);

    return 0;
}
