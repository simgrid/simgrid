/* xbt/xbt_os_thread.h -- Thread portability layer                          */

/* Copyright (c) 2007-2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_OS_THREAD_H
#define _XBT_OS_THREAD_H

#include "simgrid_config.h" /* Windows or Posix */
#include "xbt/function_types.h"

SG_BEGIN_DECL()

/** @addtogroup XBT_thread
 *  @brief Thread portability layer
 * 
 *  This section describes the thread portability layer. It defines types and 
 *  functions very close to the pthread API, but it's portable to windows too.
 * 
 *  @{
 */
  /** \brief Thread data type (opaque structure) */
typedef struct xbt_os_thread_ *xbt_os_thread_t;

#include <pthread.h>

#ifdef _XBT_WIN32 /* defined if this is a windows system, 32bits or 64bits) */
#include <WinDef.h>
typedef DWORD xbt_os_thread_key_t;
#else /* assume that every non-windows system is POSIX-compatible */
typedef pthread_key_t xbt_os_thread_key_t;
#endif

/** Calls pthread_atfork() if present, and raise an exception otherwise.
 *
 * The only known user of this wrapper is mmalloc_preinit(), but it is absolutely mandatory there:
 * when used with tesh, mmalloc *must* be mutex protected and resistant to forks.
 * This functionality is the only way to get it working (by ensuring that the mutex is consistently released on forks)
 */
XBT_PUBLIC(int) xbt_os_thread_atfork(void (*prepare)(void),
                                     void (*parent)(void),
                                     void (*child)(void));

XBT_PUBLIC(int) xbt_os_get_numcores(void);

XBT_PUBLIC(xbt_os_thread_t) xbt_os_thread_create(const char *name,
                                                 pvoid_f_pvoid_t start_routine,
                                                 void *param,
                                                 void *data);

XBT_PUBLIC(void) xbt_os_thread_exit(int *retcode);
XBT_PUBLIC(void) xbt_os_thread_detach(xbt_os_thread_t thread);
XBT_PUBLIC(xbt_os_thread_t) xbt_os_thread_self(void);
XBT_PUBLIC(const char *) xbt_os_thread_self_name(void);
XBT_PUBLIC(const char *) xbt_os_thread_name(xbt_os_thread_t);
XBT_PUBLIC(void) xbt_os_thread_set_extra_data(void *data);
XBT_PUBLIC(void *) xbt_os_thread_get_extra_data(void);
XBT_PUBLIC(void) xbt_os_thread_key_create(xbt_os_thread_key_t* key);
XBT_PUBLIC(void) xbt_os_thread_set_specific(xbt_os_thread_key_t key, void* value);
XBT_PUBLIC(void*) xbt_os_thread_get_specific(xbt_os_thread_key_t key);
  /* xbt_os_thread_join frees the joined thread (ie the XBT wrapper around it, the OS frees the rest) */
XBT_PUBLIC(void) xbt_os_thread_join(xbt_os_thread_t thread,
                                    void **thread_return);
XBT_PUBLIC(void) xbt_os_thread_yield(void);
XBT_PUBLIC(void) xbt_os_thread_cancel(xbt_os_thread_t thread);
XBT_PUBLIC(void *) xbt_os_thread_getparam(void);
XBT_PUBLIC(void) xbt_os_thread_setstacksize(int stack_size);

  /** \brief Thread mutex data type (opaque structure) */
typedef struct xbt_os_mutex_ *xbt_os_mutex_t;

XBT_PUBLIC(xbt_os_mutex_t) xbt_os_mutex_init(void);
XBT_PUBLIC(void) xbt_os_mutex_acquire(xbt_os_mutex_t mutex);
XBT_PUBLIC(void) xbt_os_mutex_timedacquire(xbt_os_mutex_t mutex,
                                           double delay);
XBT_PUBLIC(void) xbt_os_mutex_release(xbt_os_mutex_t mutex);
XBT_PUBLIC(void) xbt_os_mutex_destroy(xbt_os_mutex_t mutex);

/** \brief Thread reentrant mutex data type (opaque structure) */
typedef struct xbt_os_rmutex_ *xbt_os_rmutex_t;

XBT_PUBLIC(xbt_os_rmutex_t) xbt_os_rmutex_init(void);
XBT_PUBLIC(void) xbt_os_rmutex_acquire(xbt_os_rmutex_t rmutex);
XBT_PUBLIC(void) xbt_os_rmutex_release(xbt_os_rmutex_t rmutex);
XBT_PUBLIC(void) xbt_os_rmutex_destroy(xbt_os_rmutex_t rmutex);


  /** \brief Thread condition data type (opaque structure) */
typedef struct xbt_os_cond_ *xbt_os_cond_t;

XBT_PUBLIC(xbt_os_cond_t) xbt_os_cond_init(void);
XBT_PUBLIC(void) xbt_os_cond_wait(xbt_os_cond_t cond,
                                  xbt_os_mutex_t mutex);
XBT_PUBLIC(void) xbt_os_cond_timedwait(xbt_os_cond_t cond,
                                       xbt_os_mutex_t mutex, double delay);
XBT_PUBLIC(void) xbt_os_cond_signal(xbt_os_cond_t cond);
XBT_PUBLIC(void) xbt_os_cond_broadcast(xbt_os_cond_t cond);
XBT_PUBLIC(void) xbt_os_cond_destroy(xbt_os_cond_t cond);

   /** \brief Semaphore data type (opaque structure) */
typedef struct xbt_os_sem_ *xbt_os_sem_t;

XBT_PUBLIC(xbt_os_sem_t) xbt_os_sem_init(unsigned int value);
XBT_PUBLIC(void) xbt_os_sem_acquire(xbt_os_sem_t sem);
XBT_PUBLIC(void) xbt_os_sem_timedacquire(xbt_os_sem_t sem, double timeout);
XBT_PUBLIC(void) xbt_os_sem_release(xbt_os_sem_t sem);
XBT_PUBLIC(void) xbt_os_sem_destroy(xbt_os_sem_t sem);
XBT_PUBLIC(void) xbt_os_sem_get_value(xbt_os_sem_t sem, int *svalue);


/** @} */

SG_END_DECL()
#endif                          /* _XBT_OS_THREAD_H */
