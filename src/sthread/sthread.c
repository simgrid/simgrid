/* Copyright (c) 2002-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* SimGrid's pthread interposer. Redefinition of the pthread symbols (see the comment in sthread.h) */

#define _GNU_SOURCE
#include "src/sthread/sthread.h"
#include "src/internal_config.h"
#include <dlfcn.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>

#if HAVE_VALGRIND_H
#include <stdlib.h>
#include <valgrind/valgrind.h>
#endif

/* We don't want to intercept pthread within SimGrid. Instead we should provide the real implem to SimGrid */
static int (*raw_pthread_create)(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*);
static int (*raw_pthread_join)(pthread_t, void**);
static int (*raw_pthread_mutex_init)(pthread_mutex_t*, const pthread_mutexattr_t*);
static int (*raw_pthread_mutex_lock)(pthread_mutex_t*);
static int (*raw_pthread_mutex_trylock)(pthread_mutex_t*);
static int (*raw_pthread_mutex_unlock)(pthread_mutex_t*);
static int (*raw_pthread_mutex_destroy)(pthread_mutex_t*);

static int (*raw_pthread_mutexattr_init)(pthread_mutexattr_t*);
static int (*raw_pthread_mutexattr_settype)(pthread_mutexattr_t*, int);
static int (*raw_pthread_mutexattr_gettype)(const pthread_mutexattr_t*, int*);
static int (*raw_pthread_mutexattr_getrobust)(const pthread_mutexattr_t*, int*);
static int (*raw_pthread_mutexattr_setrobust)(pthread_mutexattr_t*, int);

static int (*raw_pthread_barrier_init)(pthread_barrier_t*, const pthread_barrierattr_t*, unsigned int count);
static int (*raw_pthread_barrier_wait)(pthread_barrier_t*);
static int (*raw_pthread_barrier_destroy)(pthread_barrier_t*);

static int (*raw_pthread_cond_init)(pthread_cond_t*, const pthread_condattr_t*);
static int (*raw_pthread_cond_signal)(pthread_cond_t*);
static int (*raw_pthread_cond_broadcast)(pthread_cond_t*);
static int (*raw_pthread_cond_wait)(pthread_cond_t*, pthread_mutex_t*);
static int (*raw_pthread_cond_timedwait)(pthread_cond_t*, pthread_mutex_t*, const struct timespec* abstime);
static int (*raw_pthread_cond_destroy)(pthread_cond_t*);

static unsigned int (*raw_sleep)(unsigned int);
static int (*raw_usleep)(useconds_t);
static int (*raw_gettimeofday)(struct timeval*, void*);

static sem_t* (*raw_sem_open)(const char*, int);
static int (*raw_sem_init)(sem_t*, int, unsigned int);
static int (*raw_sem_wait)(sem_t*);
static int (*raw_sem_post)(sem_t*);
static int (*raw_sem_destroy)(sem_t*);
static int (*raw_sem_trywait)(sem_t*);
static int (*raw_sem_timedwait)(sem_t*, const struct timespec*);

static void intercepter_init()
{
  raw_pthread_create = dlsym(RTLD_NEXT, "pthread_create");
  raw_pthread_join   = dlsym(RTLD_NEXT, "pthread_join");
  raw_pthread_mutex_init    = dlsym(RTLD_NEXT, "pthread_mutex_init");
  raw_pthread_mutex_lock    = dlsym(RTLD_NEXT, "pthread_mutex_lock");
  raw_pthread_mutex_trylock = dlsym(RTLD_NEXT, "pthread_mutex_trylock");
  raw_pthread_mutex_unlock  = dlsym(RTLD_NEXT, "pthread_mutex_unlock");
  raw_pthread_mutex_destroy = dlsym(RTLD_NEXT, "pthread_mutex_destroy");

  raw_pthread_mutexattr_init      = dlsym(RTLD_NEXT, "pthread_mutexattr_init");
  raw_pthread_mutexattr_settype   = dlsym(RTLD_NEXT, "pthread_mutexattr_settype");
  raw_pthread_mutexattr_gettype   = dlsym(RTLD_NEXT, "pthread_mutexattr_gettype");
  raw_pthread_mutexattr_getrobust = dlsym(RTLD_NEXT, "pthread_mutexattr_getrobust");
  raw_pthread_mutexattr_setrobust = dlsym(RTLD_NEXT, "pthread_mutexattr_setrobust");

  raw_pthread_barrier_init = dlsym(RTLD_NEXT, "raw_pthread_barrier_init");
  raw_pthread_barrier_wait = dlsym(RTLD_NEXT, "raw_pthread_barrier_wait");
  raw_pthread_barrier_destroy = dlsym(RTLD_NEXT, "raw_pthread_barrier_destroy");

  raw_pthread_cond_init      = dlsym(RTLD_NEXT, "raw_pthread_cond_init");
  raw_pthread_cond_signal    = dlsym(RTLD_NEXT, "raw_pthread_cond_signal");
  raw_pthread_cond_broadcast = dlsym(RTLD_NEXT, "raw_pthread_cond_broadcast");
  raw_pthread_cond_wait      = dlsym(RTLD_NEXT, "raw_pthread_cond_wait");
  raw_pthread_cond_timedwait = dlsym(RTLD_NEXT, "raw_pthread_cond_timedwait");
  raw_pthread_cond_destroy   = dlsym(RTLD_NEXT, "raw_pthread_cond_destroy");

  raw_sleep        = dlsym(RTLD_NEXT, "sleep");
  raw_usleep       = dlsym(RTLD_NEXT, "usleep");
  raw_gettimeofday = dlsym(RTLD_NEXT, "gettimeofday");

  raw_sem_open = dlsym(RTLD_NEXT, "sem_open");
  raw_sem_init = dlsym(RTLD_NEXT, "sem_init");
  raw_sem_wait = dlsym(RTLD_NEXT, "sem_wait");
  raw_sem_post = dlsym(RTLD_NEXT, "sem_post");
  raw_sem_destroy   = dlsym(RTLD_NEXT, "sem_destroy");
  raw_sem_trywait   = dlsym(RTLD_NEXT, "sem_trywait");
  raw_sem_timedwait = dlsym(RTLD_NEXT, "sem_timedwait");
}

static int sthread_inside_simgrid = 1;
void sthread_enable(void)
{ // Start intercepting all pthread calls
  sthread_inside_simgrid = 0;
}
void sthread_disable(void)
{ // Stop intercepting all pthread calls
  sthread_inside_simgrid = 1;
}

#define _STHREAD_CONCAT(a, b) a##b
#define intercepted_pthcall(name, raw_params, call_params, sim_params)                                                 \
  int _STHREAD_CONCAT(pthread_, name) raw_params                                                                       \
  {                                                                                                                    \
    if (_STHREAD_CONCAT(raw_pthread_, name) == NULL)                                                                   \
      intercepter_init();                                                                                              \
    if (sthread_inside_simgrid)                                                                                        \
      return _STHREAD_CONCAT(raw_pthread_, name) call_params;                                                          \
                                                                                                                       \
    sthread_disable();                                                                                                 \
    int res = _STHREAD_CONCAT(sthread_, name) sim_params;                                                              \
    sthread_enable();                                                                                                  \
    return res;                                                                                                        \
  }

intercepted_pthcall(mutexattr_init, (pthread_mutexattr_t * attr), (attr), ((sthread_mutexattr_t*)attr));
intercepted_pthcall(mutexattr_settype, (pthread_mutexattr_t * attr, int type), (attr, type),
                    ((sthread_mutexattr_t*)attr, type));
intercepted_pthcall(mutexattr_gettype, (const pthread_mutexattr_t* attr, int* type), (attr, type),
                    ((sthread_mutexattr_t*)attr, type));
intercepted_pthcall(mutexattr_setrobust, (pthread_mutexattr_t * attr, int robustness), (attr, robustness),
                    ((sthread_mutexattr_t*)attr, robustness));
intercepted_pthcall(mutexattr_getrobust, (const pthread_mutexattr_t* attr, int* robustness), (attr, robustness),
                    ((sthread_mutexattr_t*)attr, robustness));

intercepted_pthcall(create, (pthread_t * thread, const pthread_attr_t* attr, void* (*start_routine)(void*), void* arg),
                    (thread, attr, start_routine, arg), ((sthread_t*)thread, attr, start_routine, arg));
intercepted_pthcall(join, (pthread_t thread, void** retval), (thread, retval), ((sthread_t)thread, retval));

intercepted_pthcall(mutex_init, (pthread_mutex_t * mutex, const pthread_mutexattr_t* attr), (mutex, attr),
                    ((sthread_mutex_t*)mutex, (sthread_mutexattr_t*)attr));
intercepted_pthcall(mutex_lock, (pthread_mutex_t * mutex), (mutex), ((sthread_mutex_t*)mutex));
intercepted_pthcall(mutex_trylock, (pthread_mutex_t * mutex), (mutex), ((sthread_mutex_t*)mutex));
intercepted_pthcall(mutex_unlock, (pthread_mutex_t * mutex), (mutex), ((sthread_mutex_t*)mutex));
intercepted_pthcall(mutex_destroy, (pthread_mutex_t * mutex), (mutex), ((sthread_mutex_t*)mutex));

intercepted_pthcall(barrier_init, (pthread_barrier_t * barrier, const pthread_barrierattr_t* attr, unsigned count),
                    (barrier, attr, count), ((sthread_barrier_t*)barrier, (const sthread_barrierattr_t*)attr, count));
intercepted_pthcall(barrier_wait, (pthread_barrier_t* barrier),( barrier),((sthread_barrier_t*) barrier));
intercepted_pthcall(barrier_destroy, (pthread_barrier_t* barrier),( barrier),((sthread_barrier_t*) barrier));

intercepted_pthcall(cond_init, (pthread_cond_t * cond, const pthread_condattr_t* attr), (cond, attr),
                    ((sthread_cond_t*)cond, (sthread_condattr_t*)attr));
intercepted_pthcall(cond_signal, (pthread_cond_t * cond), (cond), ((sthread_cond_t*)cond));
intercepted_pthcall(cond_broadcast, (pthread_cond_t * cond), (cond), ((sthread_cond_t*)cond));
intercepted_pthcall(cond_wait, (pthread_cond_t * cond, pthread_mutex_t* mutex), (cond, mutex),
                    ((sthread_cond_t*)cond, (sthread_mutex_t*)mutex));
intercepted_pthcall(cond_timedwait, (pthread_cond_t * cond, pthread_mutex_t* mutex, const struct timespec* abstime),
                    (cond, mutex, abstime), ((sthread_cond_t*)cond, (sthread_mutex_t*)mutex, abstime));
intercepted_pthcall(cond_destroy, (pthread_cond_t * cond), (cond), ((sthread_cond_t*)cond));

#define intercepted_call(rettype, name, raw_params, call_params, sim_params)                                           \
  rettype name raw_params                                                                                              \
  {                                                                                                                    \
    if (_STHREAD_CONCAT(raw_, name) == NULL)                                                                           \
      intercepter_init();                                                                                              \
    if (sthread_inside_simgrid)                                                                                        \
      return _STHREAD_CONCAT(raw_, name) call_params;                                                                  \
                                                                                                                       \
    sthread_disable();                                                                                                 \
    rettype res = _STHREAD_CONCAT(sthread_, name) sim_params;                                                          \
    sthread_enable();                                                                                                  \
    return res;                                                                                                        \
  }

intercepted_call(int, sem_init, (sem_t * sem, int pshared, unsigned int value), (sem, pshared, value),
                 ((sthread_sem_t*)sem, pshared, value));
intercepted_call(int, sem_destroy, (sem_t * sem), (sem), ((sthread_sem_t*)sem));
intercepted_call(int, sem_post, (sem_t * sem), (sem), ((sthread_sem_t*)sem));
intercepted_call(int, sem_wait, (sem_t * sem), (sem), ((sthread_sem_t*)sem));
intercepted_call(int, sem_trywait, (sem_t * sem), (sem), ((sthread_sem_t*)sem));
intercepted_call(int, sem_timedwait, (sem_t * sem, const struct timespec* abs_timeout), (sem, abs_timeout),
                 ((sthread_sem_t*)sem, abs_timeout));

/* Glibc < 2.31 uses type "struct timezone *" for the second parameter of gettimeofday.
   Other implementations use "void *" instead. */
#ifdef __GLIBC__
#if !__GLIBC_PREREQ(2, 31)
#define TIMEZONE_TYPE struct timezone
#endif
#endif
#ifndef TIMEZONE_TYPE
#define TIMEZONE_TYPE void
#endif
intercepted_call(int, gettimeofday, (struct timeval * tv, XBT_ATTRIB_UNUSED TIMEZONE_TYPE* tz), (tv, tz), (tv));
intercepted_call(unsigned int, sleep, (unsigned int seconds), (seconds), (seconds));
intercepted_call(int, usleep, (useconds_t usec), (usec), (((double)usec) / 1000000.));

/* Trampoline for the real main() */
static int (*raw_main)(int, char**, char**);

/* Our fake main() that gets called by __libc_start_main() */
static int main_hook(int argc, char** argv, char** envp)
{
  return sthread_main(argc, argv, envp, raw_main);
}

/* Wrapper for __libc_start_main() that replaces the real main function with our hooked version. */
int __libc_start_main(int (*main)(int, char**, char**), int argc, char** argv, int (*init)(int, char**, char**),
                      void (*fini)(void), void (*rtld_fini)(void), void* stack_end);

int __libc_start_main(int (*main)(int, char**, char**), int argc, char** argv, int (*init)(int, char**, char**),
                      void (*fini)(void), void (*rtld_fini)(void), void* stack_end)
{
  /* Save the real main function address */
  raw_main = main;

  /* Find the real __libc_start_main()... */
  typeof(&__libc_start_main) orig = dlsym(RTLD_NEXT, "__libc_start_main");
  /* ... and call it with our custom main function */
#if HAVE_VALGRIND_H
  /* ... unless valgrind is used, and this instance is not the target program (but the valgrind launcher) */
  if (getenv("VALGRIND_LIB") && !RUNNING_ON_VALGRIND)
    return orig(raw_main, argc, argv, init, fini, rtld_fini, stack_end);
#endif
  return orig(main_hook, argc, argv, init, fini, rtld_fini, stack_end);
}
