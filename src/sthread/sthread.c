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

/* We don't want to intercept pthread within simgrid. Instead we should provide the real implem to simgrid */
static int (*raw_pthread_create)(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*);
static int (*raw_pthread_join)(pthread_t, void**);
static int (*raw_mutex_init)(pthread_mutex_t*, const pthread_mutexattr_t*) = NULL;
static int (*raw_mutex_lock)(pthread_mutex_t*)                             = NULL;
static int (*raw_mutex_trylock)(pthread_mutex_t*)                          = NULL;
static int (*raw_mutex_unlock)(pthread_mutex_t*)                           = NULL;
static int (*raw_mutex_destroy)(pthread_mutex_t*)                          = NULL;

static unsigned int (*raw_sleep)(unsigned int)         = NULL;
static int (*raw_usleep)(useconds_t)                   = NULL;
static int (*raw_gettimeofday)(struct timeval*, void*) = NULL;

static sem_t* (*raw_sem_open)(const char*, int)                            = NULL;
static int (*raw_sem_init)(sem_t*, int, unsigned int)                      = NULL;
static int (*raw_sem_wait)(sem_t*)                                         = NULL;
static int (*raw_sem_post)(sem_t*)                                         = NULL;

static void intercepter_init()
{
  raw_pthread_create = (typeof(raw_pthread_create))dlsym(RTLD_NEXT, "pthread_create");
  raw_pthread_join   = (typeof(raw_pthread_join))dlsym(RTLD_NEXT, "pthread_join");
  raw_mutex_init    = (int (*)(pthread_mutex_t*, const pthread_mutexattr_t*))dlsym(RTLD_NEXT, "pthread_mutex_init");
  raw_mutex_lock    = (int (*)(pthread_mutex_t*))dlsym(RTLD_NEXT, "pthread_mutex_lock");
  raw_mutex_trylock = (int (*)(pthread_mutex_t*))dlsym(RTLD_NEXT, "pthread_mutex_trylock");
  raw_mutex_unlock  = (int (*)(pthread_mutex_t*))dlsym(RTLD_NEXT, "pthread_mutex_unlock");
  raw_mutex_destroy = (int (*)(pthread_mutex_t*))dlsym(RTLD_NEXT, "pthread_mutex_destroy");

  raw_sleep        = (unsigned int (*)(unsigned int))dlsym(RTLD_NEXT, "sleep");
  raw_usleep       = (int (*)(useconds_t usec))dlsym(RTLD_NEXT, "usleep");
  raw_gettimeofday = (int (*)(struct timeval*, void*))dlsym(RTLD_NEXT, "gettimeofday");

  raw_sem_open = (sem_t * (*)(const char*, int)) dlsym(RTLD_NEXT, "sem_open");
  raw_sem_init = (int (*)(sem_t*, int, unsigned int))dlsym(RTLD_NEXT, "sem_init");
  raw_sem_wait = (int (*)(sem_t*))dlsym(RTLD_NEXT, "sem_wait");
  raw_sem_post = (int (*)(sem_t*))dlsym(RTLD_NEXT, "sem_post");
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

  sthread_inside_simgrid = 1;
  int res                = sthread_create((sthread_t*)thread, attr, start_routine, arg);
  sthread_inside_simgrid = 0;
  return res;
}
int pthread_join(pthread_t thread, void** retval)
{
  if (raw_pthread_join == NULL)
    intercepter_init();

  if (sthread_inside_simgrid)
    return raw_pthread_join(thread, retval);

  sthread_inside_simgrid = 1;
  int res                = sthread_join((sthread_t)thread, retval);
  sthread_inside_simgrid = 0;
  return res;
}

int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr)
{
  if (raw_mutex_init == NULL)
    intercepter_init();

  if (sthread_inside_simgrid)
    return raw_mutex_init(mutex, attr);

  sthread_inside_simgrid = 1;
  int res                = sthread_mutex_init((sthread_mutex_t*)mutex, attr);
  sthread_inside_simgrid = 0;
  return res;
}

int pthread_mutex_lock(pthread_mutex_t* mutex)
{
  if (raw_mutex_lock == NULL)
    intercepter_init();

  if (sthread_inside_simgrid)
    return raw_mutex_lock(mutex);

  sthread_inside_simgrid = 1;
  int res                = sthread_mutex_lock((sthread_mutex_t*)mutex);
  sthread_inside_simgrid = 0;
  return res;
}

int pthread_mutex_trylock(pthread_mutex_t* mutex)
{
  if (raw_mutex_trylock == NULL)
    intercepter_init();

  if (sthread_inside_simgrid)
    return raw_mutex_trylock(mutex);

  sthread_inside_simgrid = 1;
  int res                = sthread_mutex_trylock((sthread_mutex_t*)mutex);
  sthread_inside_simgrid = 0;
  return res;
}

int pthread_mutex_unlock(pthread_mutex_t* mutex)
{
  if (raw_mutex_unlock == NULL)
    intercepter_init();

  if (sthread_inside_simgrid)
    return raw_mutex_unlock(mutex);

  sthread_inside_simgrid = 1;
  int res                = sthread_mutex_unlock((sthread_mutex_t*)mutex);
  sthread_inside_simgrid = 0;
  return res;
}
int pthread_mutex_destroy(pthread_mutex_t* mutex)
{
  if (raw_mutex_destroy == NULL)
    intercepter_init();

  if (sthread_inside_simgrid)
    return raw_mutex_destroy(mutex);

  sthread_inside_simgrid = 1;
  int res                = sthread_mutex_destroy((sthread_mutex_t*)mutex);
  sthread_inside_simgrid = 0;
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

  sthread_inside_simgrid = 1;
  int res                = sthread_gettimeofday(tv);
  sthread_inside_simgrid = 0;
  return res;
}

unsigned int sleep(unsigned int seconds)
{
  if (raw_sleep == NULL)
    intercepter_init();

  if (sthread_inside_simgrid)
    return raw_sleep(seconds);

  sthread_inside_simgrid = 1;
  sthread_sleep(seconds);
  sthread_inside_simgrid = 0;
  return 0;
}

int usleep(useconds_t usec)
{
  if (raw_usleep == NULL)
    intercepter_init();

  if (sthread_inside_simgrid)
    return raw_usleep(usec);

  sthread_inside_simgrid = 1;
  sthread_sleep(((double)usec) / 1000000.);
  sthread_inside_simgrid = 0;
  return 0;
}

#if 0
int sem_init(sem_t *sem, int pshared, unsigned int value) {
	int res;

	res=raw_sem_init(sem,pshared,value);
	return res;
}

int sem_wait(sem_t *sem) {
	int res;

	res = raw_sem_wait(sem);
	return res;
}

int sem_post(sem_t *sem) {
	return raw_sem_post(sem);
}

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
