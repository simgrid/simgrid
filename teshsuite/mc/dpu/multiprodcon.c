#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef PARAM2
#define PARAM2 5
#endif

#define TASKS 2  // maximum number of tasks processed by a worker
int PRODS = 2;   // number of producers
#define ITERS 2  // number of iterations that producers make before producing a number
int WORKERS = 5; // number of worker threads

pthread_mutex_t* mut;
int* workreq;
int* workdone;

pthread_mutex_t prodmut;
unsigned prodbuff;

static void* worker(void* arg)
{
  unsigned id = (unsigned long)arg;
  int i, havework;

  havework = 0;
  for (i = 0; i < TASKS; i++) {
    pthread_mutex_lock(mut + id);
    // printf ("w%d: lock, workreq %d\n", id, workreq[id]);

    // if I have a task, perform it, decrementing the counter
    if (workreq[id]) {
      workreq[id]--;
      havework = 1;
    }

    pthread_mutex_unlock(mut + id);

    // work is done here
    if (havework) {
      workdone[id]++;
      havework = 0;
    }
  }
  return 0;
}

static void* producer(void* arg)
{
  unsigned dst, i, tmp;
  long id = (unsigned long)arg;

  // printf ("prod%d: starting\n", id);
  assert(id < PRODS);

  // The producer threads collaboratively construct a number in base PRODS that
  // have ITERS*PRODS digits. Each producer adds ITERS digits to the number.
  // The number is stored in prodbuff, it is initially 0, and each producer
  // adds a digit to the end. In the end, tmp stores a slightly different
  // number for each producer
  for (i = 0; i < ITERS; i++) {
    pthread_mutex_lock(&prodmut);
    prodbuff = prodbuff * PRODS + id; // add a digit
    tmp      = prodbuff;
    // printf ("prod%d: now %u\n", id, prodbuff);
    pthread_mutex_unlock(&prodmut);
  }

  // each number constructed by a producer must be within range
  assert(tmp <= pow(PRODS, ITERS * PRODS));

  // depending on the constructed number, which simulates some sort of thread
  // affinity, we send work to one worker thread dst
  dst = tmp % WORKERS;

  // sending work
  pthread_mutex_lock(mut + dst);
  workreq[dst]++;
  // printf ("prod%d: dst %u, prodbuff %u, workreq %d\n", id, dst, tmp, workreq[dst]);
  pthread_mutex_unlock(mut + dst);

  // printf ("prod%d: done\n", id);
  return 0;
}

int main()
{
  mut      = malloc(sizeof(pthread_mutex_t) * WORKERS);
  workreq  = malloc(sizeof(int) * WORKERS);
  workdone = malloc(sizeof(int) * WORKERS);

  pthread_t workers[WORKERS];
  pthread_t producers[PRODS];

  // printf ("== start ==\n");

  // we should have at least 2 workers
  assert(WORKERS >= 2);

  // launch the worker threads; thread i will work when workreq[i] >= 1, and
  // will increment workdon[i] when it finishes a task
  for (int i = 0; i < WORKERS; i++) {
    pthread_mutex_init(mut + i, 0);
    workreq[i]  = 0;
    workdone[i] = 0;
    pthread_create(&workers[i], 0, worker, (void*)(long)i);
  }

  // launch the producer threads, which will build a shared buffer and
  pthread_mutex_init(&prodmut, 0);
  prodbuff = 0;
  for (int i = 0; i < PRODS; i++)
    pthread_create(&producers[i], 0, producer, (void*)(long)i);

  // wait for all threads to finish
  for (int i = 0; i < WORKERS; i++)
    pthread_join(workers[i], NULL);

  for (int i = 0; i < PRODS; i++)
    pthread_join(producers[i], NULL);

  // printf ("== done ==\n");
  pthread_exit(0);
}
