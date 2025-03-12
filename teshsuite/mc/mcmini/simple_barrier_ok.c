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

pthread_barrier_t barrier;

int main(int argc, char* argv[])
{
    pthread_barrier_init(&barrier, NULL, 1);
    pthread_barrier_wait(&barrier);
    return 0;
}

