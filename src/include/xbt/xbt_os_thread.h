/* $Id$ */

/* xbt/xbt_thread.h -- Thread portability layer                             */

/* Copyright (c) 2007 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#ifndef _XBT_OS_THREAD_H
#define _XBT_OS_THREAD_H

#include "xbt/misc.h"           /* SG_BEGIN_DECL */
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

XBT_PUBLIC(xbt_os_thread_t) xbt_os_thread_create(const char *name,
                                                 pvoid_f_pvoid_t
                                                 start_routine, void *param);
XBT_PUBLIC(void) xbt_os_thread_exit(int *retcode);
XBT_PUBLIC(xbt_os_thread_t) xbt_os_thread_self(void);
XBT_PUBLIC(const char *) xbt_os_thread_self_name(void);
XBT_PUBLIC(const char *) xbt_os_thread_name(xbt_os_thread_t);
  /* xbt_os_thread_join frees the joined thread (ie the XBT wrapper around it, the OS frees the rest) */
XBT_PUBLIC(void) xbt_os_thread_join(xbt_os_thread_t thread,
                                    void **thread_return);
XBT_PUBLIC(void) xbt_os_thread_yield(void);
XBT_PUBLIC(void) xbt_os_thread_cancel(xbt_os_thread_t thread);
XBT_PUBLIC(void *) xbt_os_thread_getparam(void);


  /** \brief Thread mutex data type (opaque structure) */
     typedef struct xbt_os_mutex_ *xbt_os_mutex_t;

XBT_PUBLIC(xbt_os_mutex_t) xbt_os_mutex_init(void);
XBT_PUBLIC(void) xbt_os_mutex_acquire(xbt_os_mutex_t mutex);
XBT_PUBLIC(void) xbt_os_mutex_timedacquire(xbt_os_mutex_t mutex,
                                           double delay);
XBT_PUBLIC(void) xbt_os_mutex_release(xbt_os_mutex_t mutex);
XBT_PUBLIC(void) xbt_os_mutex_destroy(xbt_os_mutex_t mutex);


  /** \brief Thread condition data type (opaque structure) */
     typedef struct xbt_os_cond_ *xbt_os_cond_t;

XBT_PUBLIC(xbt_os_cond_t) xbt_os_cond_init(void);
XBT_PUBLIC(void) xbt_os_cond_wait(xbt_os_cond_t cond, xbt_os_mutex_t mutex);
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
#endif /* _XBT_OS_THREAD_H */
