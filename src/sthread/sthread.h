/* Copyright (c) 2002-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* SimGrid's pthread interposer. Intercepts most of the pthread and semaphore calls to model-check them.
 *
 * Intercepting on pthread is somewhat complicated by the fact that pthread is used everywhere in the system headers.
 * To reduce definition conflicts, our redefinitions of the pthread symbols (in sthread.c) load as little headers as
 * possible. Thus, the actual implementations are separated in another file (sthread_impl.cpp) and are used as black
 * boxes by our redefinitions.
 *
 * The sthread_* symbols are those actual implementations, used in the pthread_* redefinitions. */

#ifndef SIMGRID_STHREAD_H
#define SIMGRID_STHREAD_H

#include "xbt/base.h"
#include <sys/time.h>

#if defined(__cplusplus)
extern "C" {
#endif

// Launch the simulation. The old main function (passed as a parameter) is launched as an actor
int sthread_main(int argc, char** argv, char** envp, int (*raw_main)(int, char**, char**));
XBT_PUBLIC void sthread_enable(void);  // Start intercepting all pthread calls
XBT_PUBLIC void sthread_disable(void); // Stop intercepting all pthread calls

typedef unsigned long int sthread_t;
int sthread_create(sthread_t* thread, const /*pthread_attr_t*/ void* attr, void* (*start_routine)(void*), void* arg);
int sthread_join(sthread_t thread, void** retval);

typedef struct {
  unsigned recursive : 1;
  unsigned errorcheck : 1;
  unsigned robust : 1;
} sthread_mutexattr_t;

int sthread_mutexattr_init(sthread_mutexattr_t* attr);
int sthread_mutexattr_settype(sthread_mutexattr_t* attr, int type);
int sthread_mutexattr_gettype(const sthread_mutexattr_t* attr, int* type);
int sthread_mutexattr_getrobust(const sthread_mutexattr_t* attr, int* robustness);
int sthread_mutexattr_setrobust(sthread_mutexattr_t* attr, int robustness);

typedef struct {
  void* mutex;
} sthread_mutex_t;
int sthread_mutex_init(sthread_mutex_t* mutex, const sthread_mutexattr_t* attr);
int sthread_mutex_lock(sthread_mutex_t* mutex);
int sthread_mutex_trylock(sthread_mutex_t* mutex);
int sthread_mutex_unlock(sthread_mutex_t* mutex);
int sthread_mutex_destroy(sthread_mutex_t* mutex);

typedef struct {
  unsigned unused : 1;
} sthread_barrierattr_t;

typedef struct {
  void* barrier;
} sthread_barrier_t;
int sthread_barrier_init(sthread_barrier_t* barrier, const sthread_barrierattr_t* attr, unsigned count);
int sthread_barrier_wait(sthread_barrier_t* barrier);
int sthread_barrier_destroy(sthread_barrier_t* barrier);

typedef struct {
  unsigned unused : 1;
} sthread_condattr_t;

typedef struct {
  void* cond;
  void* mutex;
} sthread_cond_t;
int sthread_cond_init(sthread_cond_t* cond, sthread_condattr_t* attr);
int sthread_cond_signal(sthread_cond_t* cond);
int sthread_cond_broadcast(sthread_cond_t* cond);
int sthread_cond_wait(sthread_cond_t* cond, sthread_mutex_t* mutex);
int sthread_cond_timedwait(sthread_cond_t* cond, sthread_mutex_t* mutex, const struct timespec* abs_timeout);
int sthread_cond_destroy(sthread_cond_t* cond);

typedef struct {
  void* sem;
} sthread_sem_t;
int sthread_sem_init(sthread_sem_t* sem, int pshared, unsigned int value);
int sthread_sem_destroy(sthread_sem_t* sem);
int sthread_sem_post(sthread_sem_t* sem);
int sthread_sem_wait(sthread_sem_t* sem);
int sthread_sem_trywait(sthread_sem_t* sem);
int sthread_sem_timedwait(sthread_sem_t* sem, const struct timespec* abs_timeout);

int sthread_gettimeofday(struct timeval* tv);
unsigned int sthread_sleep(double seconds);
int sthread_usleep(double seconds);

int sthread_access_begin(void* objaddr, const char* objname, const char* file, int line, const char* function);
void sthread_access_end(void* objaddr, const char* objname, const char* file, int line, const char* function);

#if defined(__cplusplus)
}
#endif

#endif
