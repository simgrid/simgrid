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

// default value
#ifndef PARAM1
#define PARAM1 3
#endif

#define K PARAM1

/*
The left part of the unfolding:

             r0:mi  ...... r1:mi
           /  |
w0:ma[0] .. r0:ma[0]
   |   /   /      \         .
r0:ma[0] w0:ma[0] r1:mi
   |               |
r1:mi             r1:ma[1]
   |             .  |
r1:ma[1].       . w1:ma[1]
   |     .     .
w1:ma[1]  .   .
           . .
         w1:ma[1]
   \     /     \   /
   r1:ma[1]    r1:ma[1]

*/
pthread_mutex_t ma[K];
pthread_mutex_t mi;

// The parametric threads
// wa locks on a different mutex
// they are all concurrent with each other
static void* wa(void* arg)
{
  unsigned id = (unsigned long)arg;
  pthread_mutex_lock(&ma[id]);
  pthread_mutex_unlock(&ma[id]);
  return 0;
}

// ra locks on a common lock
// and then conflicts with one of the wa's
// To prevent a blow-up in the number
// comment out the unlock mi.
// Not unlocking, will produce an exponential
// number of the SSBs:
/*
               a:lock x  .... b:lock x
                  |              |
                  |              |
                  |              |
 c:lock y ...  a:lock y       b:lock z .... d:lock z

*/

// then it locks on one of wa
static void* ra(void* arg)
{
  unsigned id = (unsigned long)arg;
  pthread_mutex_lock(&mi);
  pthread_mutex_lock(&ma[id]);
  pthread_mutex_unlock(&ma[id]);
  pthread_mutex_unlock(&mi);
  return 0;
}

int main()
{
  pthread_t idr[K];
  pthread_t idw[K];
  pthread_mutex_init(&mi, NULL);

  for (int i = 0; i < K; i++) {
    pthread_mutex_init(&ma[i], NULL);
    pthread_create(&idw[i], NULL, wa, (void*)(long)i);
    pthread_create(&idr[i], NULL, ra, (void*)(long)i);
  }

  pthread_exit(0);
  for (int i = 0; i < K; i++) {
    pthread_join(idw[i], NULL);
    pthread_join(idr[i], NULL);
  }
}
