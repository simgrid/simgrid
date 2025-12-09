// Copyright (C) 2010-2017  Cesar Rodriguez <cesar.rodriguez@lipn.fr>
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.

#include <assert.h>
#include <pthread.h>
#include <stdio.h>

/*
 This benchmark simulates a system that
 continuously and non-deterministically
 pokes a thread pool of workers to identify
 workers that are free to be assigned tasks.

 The main function spawns a set of work
 threads that can be busy or not.
 This is simulated by a thread wa that acts
 as a representative of a work thread and
 signals via a channel *a* that it is free
 to work.

 Then, it will for K iterations poke the
 thread pool non-determistically to gather
 the number of free threads.

 Thus, the maximum number of free threads
 is automatically bounded by K.

 For a fixed K (e.g. K 1) and an
 increasing N, we obtain an exponential
 number of SSBs in nidhugg.

 For a fixed N > 1 (e.g. N 2) and
 an increasing K, we obtain almost
 the same number of maximal configurations
 as SSBs in nidhugg.
*/

#ifndef PARAM1
#define PARAM1 4
#endif

#ifndef PARAM2
#define PARAM2 4
#endif

#define N PARAM1 // number of threads
#define K PARAM2 // nunmber of iterations

pthread_mutex_t ma[N];
int a[N];

pthread_mutex_t mi;
int i = 0;
int d = 0;

// chooses a number i
static void* choice()
{
  // printf ("choice: start\n");
  for (int x = 1; x < N; x++) {
    // printf ("choice: it %d\n",x);
    pthread_mutex_lock(&mi);
    i = x;
    pthread_mutex_unlock(&mi);
  }
  // printf ("choice: end\n");
  return 0;
}

// writer thread to a
static void* wa(void* arg)
{
  unsigned id = (unsigned long)arg;
  // printf ("wa%u: start\n", id);
  pthread_mutex_lock(&ma[id]);
  // Does some work and signals that it is done
  a[id] = 1;
  pthread_mutex_unlock(&ma[id]);
  // printf ("wa%u: end\n", id);

  return 0;
}

// reader thread to a
static void* ra()
{
  // get the tid to poke
  int idx = 0;
  // printf ("ra: start\n");
  pthread_mutex_lock(&mi);
  idx = i;
  pthread_mutex_unlock(&mi);
  // printf ("ra: got i from choice\n");

  // poke it!
  pthread_mutex_lock(&ma[idx]);
  // if the thread is done, increment d
  // note that although we will create
  // multiple ra's, they will never be
  // concurrent. thus there is no data race on d.
  if (a[idx] == 1) {
    d++;
    a[idx] = 0;
  }
  pthread_mutex_unlock(&ma[idx]);

  return 0;
}

int main()
{
  pthread_t idk;
  pthread_t idr;
  pthread_t idw[N];

  pthread_mutex_init(&mi, NULL);

  // printf ("== start ==\n");

  // spawn the write threads
  for (int x = 0; x < N; x++) {
    // initialize the array index
    a[x] = 0;
    // initialize the mutex
    pthread_mutex_init(&ma[x], NULL);
    // spawn the writer a[x]
    pthread_create(&idw[x], NULL, wa, (void*)(long)x);
  }

  // lets find out in K iterations
  // how many elements have been initialized
  // by poking one thread non-deterministically
  // per iteration.
  for (int x = 0; x < K; x++) {
    // printf ("main: iteration %d\n", x);
    //  spawn the counter
    pthread_create(&idk, NULL, choice, NULL);
    // spawn the poke thread
    pthread_create(&idr, NULL, ra, NULL);
    // join the threads
    pthread_join(idk, NULL);
    pthread_join(idr, NULL);
  }

  assert(d >= 0);
  assert(d <= N);

  // join the writer threads
  for (int x = 0; x < N; x++)
    pthread_join(idw[x], NULL);

  // assert that d equals to the number of threads that finished before the
  // poking thread checked
  for (int x = 0; x < N; x++)
    if (a[x])
      d++;
  assert(d == N);

  return 0;
}
