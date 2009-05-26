/* $Id$ */

/* philosopher - classical dinning philosopher as a demo xbt syncro stuff   */

/* Copyright (c) 2007 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras.h"
#include "xbt/synchro.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(philo, "Logs of this example");


/** Philosopher logic **/
int lunch_amount = 10;
int philosopher_amount;

xbt_mutex_t mutex;
xbt_cond_t *forks;

#define THINKING 0
#define EATING 1
int *state;

int *id;                        /* to pass a pointer to the threads without race condition */

static void pickup(int id, int lunch)
{
  INFO2("Thread %d gets hungry (lunch #%d)", id, lunch);
  xbt_mutex_acquire(mutex);
  while (state[(id + (philosopher_amount - 1)) % philosopher_amount] == EATING
         || state[(id + 1) % philosopher_amount] == EATING) {
    xbt_cond_wait(forks[id], mutex);
  }

  state[id] = EATING;
  xbt_assert1(state[(id + (philosopher_amount - 1)) % philosopher_amount] ==
              THINKING
              && state[(id + 1) % philosopher_amount] == THINKING,
              "Philosopher %d eats at the same time that one of its neighbors!!!",
              id);

  xbt_mutex_release(mutex);
  INFO1("Thread %d eats", id);
}

static void putdown(int id)
{
  INFO1("Thread %d is full", id);
  xbt_mutex_acquire(mutex);
  state[id] = THINKING;
  xbt_cond_signal(forks
                  [(id + (philosopher_amount - 1)) % philosopher_amount]);
  xbt_cond_signal(forks[(id + 1) % philosopher_amount]);

  xbt_mutex_release(mutex);
  INFO1("Thread %d thinks", id);
}

/*
 * Some additionnal code to let the father wait the childs
 */
xbt_mutex_t mut_end;
xbt_cond_t cond_end;
int running_threads;

xbt_mutex_t dead_end;

/* Code ran by each thread */
static void philo_thread(void *arg)
{
  int id = *(int *) arg;
  int i;

  for (i = 0; i < lunch_amount; i++) {
    pickup(id, i);
    gras_os_sleep(id / 100.0);  /* each philosopher sleeps and eat a time related to its ID */
    putdown(id);
    gras_os_sleep(id / 100.0);
  }

  xbt_mutex_acquire(mut_end);
  running_threads--;
  xbt_cond_signal(cond_end);
  xbt_mutex_release(mut_end);

  /* Enter an endless loop to test the killing facilities */
  INFO1
    ("Thread %d tries to enter the dead-end; hopefully, the master will cancel it",
     id);
  xbt_mutex_acquire(dead_end);
  INFO1("Oops, thread %d reached the dead-end. Cancelation failed", id);
}

int philosopher(int argc, char *argv[]);
int philosopher(int argc, char *argv[])
{
  int i;
  xbt_thread_t *philosophers;

  gras_init(&argc, argv);
  xbt_assert0(argc >= 2,
              "This program expects one argument (the amount of philosophers)");

  /* initializations of the philosopher mecanisms */
  philosopher_amount = atoi(argv[1]);
  state = xbt_new0(int, philosopher_amount);
  id = xbt_new0(int, philosopher_amount);
  forks = xbt_new(xbt_cond_t, philosopher_amount);
  philosophers = xbt_new(xbt_thread_t, philosopher_amount);

  mutex = xbt_mutex_init();
  for (i = 0; i < philosopher_amount; i++) {
    state[i] = THINKING;
    id[i] = i;
    forks[i] = xbt_cond_init();
  }

  /* setup the ending mecanism */
  running_threads = philosopher_amount;
  cond_end = xbt_cond_init();
  mut_end = xbt_mutex_init();
  dead_end = xbt_mutex_init();
  xbt_mutex_acquire(dead_end);

  INFO2("Spawn the %d threads (%d lunches scheduled)", philosopher_amount,
        lunch_amount);
  /* spawn threads */
  for (i = 0; i < philosopher_amount; i++) {
    char *name = bprintf("thread %d", i);
    philosophers[i] = xbt_thread_create(name, philo_thread, &id[i]);
    free(name);
  }

  /* wait for them */
  xbt_mutex_acquire(mut_end);
  while (running_threads)
    xbt_cond_wait(cond_end, mut_end);
  xbt_mutex_release(mut_end);

  INFO0("Cancel all childs");
  /* nuke them threads */
  for (i = 0; i < philosopher_amount; i++) {
    xbt_thread_cancel(philosophers[i]);
  }

  xbt_mutex_release(dead_end);
  gras_exit();
  return 0;
}
