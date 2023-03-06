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
  void* mutex;
} sthread_mutex_t;
int sthread_mutex_init(sthread_mutex_t* mutex, const /*pthread_mutexattr_t*/ void* attr);
int sthread_mutex_lock(sthread_mutex_t* mutex);
int sthread_mutex_trylock(sthread_mutex_t* mutex);
int sthread_mutex_unlock(sthread_mutex_t* mutex);
int sthread_mutex_destroy(sthread_mutex_t* mutex);

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
void sthread_sleep(double seconds);

int sthread_access_begin(void* objaddr, const char* objname, const char* file, int line);
void sthread_access_end(void* objaddr, const char* objname, const char* file, int line);

#if defined(__cplusplus)
}
#endif

#endif
