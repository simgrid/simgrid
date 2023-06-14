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
static int (*raw_mutex_init)(pthread_mutex_t*, const pthread_mutexattr_t*);
static int (*raw_mutex_lock)(pthread_mutex_t*);
static int (*raw_mutex_trylock)(pthread_mutex_t*);
static int (*raw_mutex_unlock)(pthread_mutex_t*);
static int (*raw_mutex_destroy)(pthread_mutex_t*);

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
  raw_mutex_init     = dlsym(RTLD_NEXT, "pthread_mutex_init");
  raw_mutex_lock     = dlsym(RTLD_NEXT, "pthread_mutex_lock");
  raw_mutex_trylock  = dlsym(RTLD_NEXT, "pthread_mutex_trylock");
  raw_mutex_unlock   = dlsym(RTLD_NEXT, "pthread_mutex_unlock");
  raw_mutex_destroy  = dlsym(RTLD_NEXT, "pthread_mutex_destroy");

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
int pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void*), void* arg)
{
  if (raw_pthread_create == NULL)
    intercepter_init();

  if (sthread_inside_simgrid)
    return raw_pthread_create(thread, attr, start_routine, arg);

  sthread_disable();
  int res = sthread_create((sthread_t*)thread, attr, start_routine, arg);
  sthread_enable();
  return res;
}
int pthread_join(pthread_t thread, void** retval)
{
  if (raw_pthread_join == NULL)
    intercepter_init();
  if (sthread_inside_simgrid)
    return raw_pthread_join(thread, retval);

  sthread_disable();
  int res = sthread_join((sthread_t)thread, retval);
  sthread_enable();
  return res;
}

int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr)
{
  if (raw_mutex_init == NULL)
    intercepter_init();

  if (sthread_inside_simgrid)
    return raw_mutex_init(mutex, attr);

  sthread_disable();
  int res = sthread_mutex_init((sthread_mutex_t*)mutex, attr);
  sthread_enable();
  return res;
}

int pthread_mutex_lock(pthread_mutex_t* mutex)
{
  if (raw_mutex_lock == NULL)
    intercepter_init();

  if (sthread_inside_simgrid)
    return raw_mutex_lock(mutex);

  sthread_disable();
  int res = sthread_mutex_lock((sthread_mutex_t*)mutex);
  sthread_enable();
  return res;
}

int pthread_mutex_trylock(pthread_mutex_t* mutex)
{
  if (raw_mutex_trylock == NULL)
    intercepter_init();

  if (sthread_inside_simgrid)
    return raw_mutex_trylock(mutex);

  sthread_disable();
  int res = sthread_mutex_trylock((sthread_mutex_t*)mutex);
  sthread_enable();
  return res;
}

int pthread_mutex_unlock(pthread_mutex_t* mutex)
{
  if (raw_mutex_unlock == NULL)
    intercepter_init();

  if (sthread_inside_simgrid)
    return raw_mutex_unlock(mutex);

  sthread_disable();
  int res = sthread_mutex_unlock((sthread_mutex_t*)mutex);
  sthread_enable();
  return res;
}
int pthread_mutex_destroy(pthread_mutex_t* mutex)
{
  if (raw_mutex_destroy == NULL)
    intercepter_init();

  if (sthread_inside_simgrid)
    return raw_mutex_destroy(mutex);

  sthread_disable();
  int res = sthread_mutex_destroy((sthread_mutex_t*)mutex);
  sthread_enable();
  return res;
}
int sem_init(sem_t* sem, int pshared, unsigned int value)
{
  if (raw_sem_init == NULL)
    intercepter_init();

  if (sthread_inside_simgrid)
    return raw_sem_init(sem, pshared, value);

  sthread_disable();
  int res = sthread_sem_init((sthread_sem_t*)sem, pshared, value);
  sthread_enable();
  return res;
}
int sem_destroy(sem_t* sem)
{
  if (raw_sem_destroy == NULL)
    intercepter_init();

  if (sthread_inside_simgrid)
    return raw_sem_destroy(sem);

  sthread_disable();
  int res = sthread_sem_destroy((sthread_sem_t*)sem);
  sthread_enable();
  return res;
}
int sem_post(sem_t* sem)
{
  if (raw_sem_post == NULL)
    intercepter_init();

  if (sthread_inside_simgrid)
    return raw_sem_post(sem);

  sthread_disable();
  int res = sthread_sem_post((sthread_sem_t*)sem);
  sthread_enable();
  return res;
}
int sem_wait(sem_t* sem)
{
  if (raw_sem_wait == NULL)
    intercepter_init();

  if (sthread_inside_simgrid)
    return raw_sem_wait(sem);

  sthread_disable();
  int res = sthread_sem_wait((sthread_sem_t*)sem);
  sthread_enable();
  return res;
}
int sem_trywait(sem_t* sem)
{
  if (raw_sem_trywait == NULL)
    intercepter_init();

  if (sthread_inside_simgrid)
    return raw_sem_trywait(sem);

  sthread_disable();
  int res = sthread_sem_trywait((sthread_sem_t*)sem);
  sthread_enable();
  return res;
}
int sem_timedwait(sem_t* sem, const struct timespec* abs_timeout)
{
  if (raw_sem_timedwait == NULL)
    intercepter_init();

  if (sthread_inside_simgrid)
    return raw_sem_timedwait(sem, abs_timeout);

  sthread_disable();
  int res = sthread_sem_timedwait((sthread_sem_t*)sem, abs_timeout);
  sthread_enable();
  return res;
}

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

int gettimeofday(struct timeval* tv, XBT_ATTRIB_UNUSED TIMEZONE_TYPE* tz)
{
  if (raw_gettimeofday == NULL)
    intercepter_init();

  if (sthread_inside_simgrid)
    return raw_gettimeofday(tv, tz);

  sthread_disable();
  int res = sthread_gettimeofday(tv);
  sthread_enable();
  return res;
}

unsigned int sleep(unsigned int seconds)
{
  if (raw_sleep == NULL)
    intercepter_init();

  if (sthread_inside_simgrid)
    return raw_sleep(seconds);

  sthread_disable();
  sthread_sleep(seconds);
  sthread_enable();
  return 0;
}

int usleep(useconds_t usec)
{
  if (raw_usleep == NULL)
    intercepter_init();

  if (sthread_inside_simgrid)
    return raw_usleep(usec);

  sthread_disable();
  sthread_sleep(((double)usec) / 1000000.);
  sthread_enable();
  return 0;
}

#if 0
int pthread_cond_init(pthread_cond_t *cond, pthread_condattr_t *cond_attr) {
    *cond = sg_cond_init();
    return 0;
}

int pthread_cond_signal(pthread_cond_t *cond) {
	sg_cond_notify_one(*cond);
    return 0;
}

int pthread_cond_broadcast(pthread_cond_t *cond) {
	sg_cond_notify_all(*cond);
    return 0;
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
	sg_cond_wait(*cond, *mutex);
    return 0;
}

int pthread_cond_destroy(pthread_cond_t *cond) {
	sg_cond_destroy(*cond);
    return 0;
}
#endif

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
