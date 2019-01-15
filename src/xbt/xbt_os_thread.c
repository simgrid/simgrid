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
