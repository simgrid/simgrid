/* xbt_os_thread -- portability layer over the pthread API                  */
/* Used in RL to get win/lin portability, and in SG when CONTEXT_THREAD     */
/* in SG, when using HAVE_UCONTEXT_CONTEXTS, xbt_os_thread_stub is used instead   */

/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/internal_config.h"
#if HAVE_PTHREAD_SETAFFINITY
#define _GNU_SOURCE
#include <sched.h>
#endif

#include <pthread.h>

#if defined(__FreeBSD__)
#include "pthread_np.h"
#define cpu_set_t cpuset_t
#endif

#include <limits.h>
#include <semaphore.h>
#include <errno.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__MACH__) && defined(__APPLE__)
#include <stdint.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#else
#include <unistd.h>
#endif

#include "xbt/sysdep.h"
#include "xbt/ex.h"
#include "src/internal_config.h"
#include "xbt/xbt_os_time.h"       /* Portable time facilities */
#include "xbt/xbt_os_thread.h"     /* This module */
#include "src/xbt_modinter.h"      /* Initialization/finalization of this module */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_sync_os, xbt, "Synchronization mechanism (OS-level)");

typedef struct xbt_os_thread_ {
  pthread_t t;
  void *param;
  pvoid_f_pvoid_t start_routine;
} s_xbt_os_thread_t;
static xbt_os_thread_t main_thread = NULL;

/* thread-specific data containing the xbt_os_thread_t structure */
static int thread_mod_inited = 0;

/* defaults attribute for pthreads */
static pthread_attr_t thread_attr;

/* frees the xbt_os_thread_t corresponding to the current thread */
static void xbt_os_thread_free_thread_data(xbt_os_thread_t thread)
{
  if (thread == main_thread)    /* just killed main thread */
    main_thread = NULL;
  free(thread);
}

void xbt_os_thread_mod_preinit(void)
{
  if (thread_mod_inited)
    return;

  main_thread = xbt_new(s_xbt_os_thread_t, 1);
  main_thread->param = NULL;
  main_thread->start_routine = NULL;

  pthread_attr_init(&thread_attr);

  thread_mod_inited = 1;
}

void xbt_os_thread_mod_postexit(void)
{
  free(main_thread);
  main_thread = NULL;
  thread_mod_inited = 0;
}

/** Calls pthread_atfork() if present, and raise an exception otherwise.
 *
 * The only known user of this wrapper is mmalloc_preinit(), but it is absolutely mandatory there:
 * when used with tesh, mmalloc *must* be mutex protected and resistant to forks.
 * This functionality is the only way to get it working (by ensuring that the mutex is consistently released on forks)
 */

/* this function is critical to tesh+mmalloc, don't mess with it */
int xbt_os_thread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void))
{
  return pthread_atfork(prepare, parent, child);
}

static void *wrapper_start_routine(void *s)
{
  xbt_os_thread_t t = s;

  return t->start_routine(t->param);
}

xbt_os_thread_t xbt_os_thread_create(pvoid_f_pvoid_t start_routine, void* param)
{
  xbt_os_thread_t res_thread = xbt_new(s_xbt_os_thread_t, 1);
  res_thread->start_routine = start_routine;
  res_thread->param = param;

  int errcode = pthread_create(&(res_thread->t), &thread_attr, wrapper_start_routine, res_thread);
  xbt_assert(errcode == 0, "pthread_create failed: %s", strerror(errcode));

  return res_thread;
}

/** Bind the thread to the given core, if possible.
 *
 * If pthread_setaffinity_np is not usable on that (non-gnu) platform, this function does nothing.
 */
int xbt_os_thread_bind(XBT_ATTRIB_UNUSED xbt_os_thread_t thread, XBT_ATTRIB_UNUSED int cpu)
{
  int errcode = 0;
#if HAVE_PTHREAD_SETAFFINITY
  pthread_t pthread = thread->t;
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(cpu, &cpuset);
  errcode = pthread_setaffinity_np(pthread, sizeof(cpu_set_t), &cpuset);
#endif
  return errcode;
}

void xbt_os_thread_setstacksize(int stack_size)
{
  size_t alignment[] = {
    xbt_pagesize,
#ifdef PTHREAD_STACK_MIN
    PTHREAD_STACK_MIN,
#endif
    0
  };

  xbt_assert(stack_size >= 0, "stack size %d is negative, maybe it exceeds MAX_INT?", stack_size);

  size_t sz = stack_size;
  int res = pthread_attr_setstacksize(&thread_attr, sz);

  for (int i = 0; res == EINVAL && alignment[i] > 0; i++) {
    /* Invalid size, try again with next multiple of alignment[i]. */
    size_t rem = sz % alignment[i];
    if (rem != 0 || sz == 0) {
      size_t sz2 = sz - rem + alignment[i];
      XBT_DEBUG("pthread_attr_setstacksize failed for %zu, try again with %zu", sz, sz2);
      sz = sz2;
      res = pthread_attr_setstacksize(&thread_attr, sz);
    }
  }

  if (res == EINVAL)
    XBT_WARN("invalid stack size (maybe too big): %zu", sz);
  else if (res != 0)
    XBT_WARN("unknown error %d in pthread stacksize setting: %zu", res, sz);
}

void xbt_os_thread_setguardsize(int guard_size)
{
#ifdef WIN32
  THROW_UNIMPLEMENTED; //pthread_attr_setguardsize is not implemented in pthread.h on windows
#else
  size_t sz = guard_size;
  int res = pthread_attr_setguardsize(&thread_attr, sz);
  if (res)
    XBT_WARN("pthread_attr_setguardsize failed (%d) for size: %zu", res, sz);
#endif
}

void xbt_os_thread_join(xbt_os_thread_t thread, void **thread_return)
{
  int errcode = pthread_join(thread->t, thread_return);

  xbt_assert(errcode==0, "pthread_join failed: %s", strerror(errcode));
  xbt_os_thread_free_thread_data(thread);
}

void xbt_os_thread_exit(int *retval)
{
  pthread_exit(retval);
}

/****** mutex related functions ******/
typedef struct xbt_os_mutex_ {
  pthread_mutex_t m;
} s_xbt_os_mutex_t;

#include <time.h>
#include <math.h>

xbt_os_mutex_t xbt_os_mutex_init(void)
{
  pthread_mutexattr_t Attr;
  pthread_mutexattr_init(&Attr);
  pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_RECURSIVE);

  xbt_os_mutex_t res = xbt_new(s_xbt_os_mutex_t, 1);
  int errcode = pthread_mutex_init(&(res->m), &Attr);
  xbt_assert(errcode==0, "pthread_mutex_init() failed: %s", strerror(errcode));

  return res;
}

void xbt_os_mutex_acquire(xbt_os_mutex_t mutex)
{
  int errcode = pthread_mutex_lock(&(mutex->m));
  xbt_assert(errcode==0, "pthread_mutex_lock(%p) failed: %s", mutex, strerror(errcode));
}

void xbt_os_mutex_release(xbt_os_mutex_t mutex)
{
  int errcode = pthread_mutex_unlock(&(mutex->m));
  xbt_assert(errcode==0, "pthread_mutex_unlock(%p) failed: %s", mutex, strerror(errcode));
}

void xbt_os_mutex_destroy(xbt_os_mutex_t mutex)
{
  if (!mutex)
    return;

  int errcode = pthread_mutex_destroy(&(mutex->m));
  xbt_assert(errcode == 0, "pthread_mutex_destroy(%p) failed: %s", mutex, strerror(errcode));
  free(mutex);
}
