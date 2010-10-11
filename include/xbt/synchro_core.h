/* xbt/synchro.h -- Synchronization tools                                   */
/* Usable in simulator, (or in real life when mixing with GRAS)             */

/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* splited away from synchro.h since we areused by dynar.h, and synchro.h uses dynar */


#ifndef _XBT_THREAD_H
#define _XBT_THREAD_H

#include "xbt/misc.h"           /* SG_BEGIN_DECL */
#include "xbt/function_types.h"

SG_BEGIN_DECL()

/** @addtogroup XBT_synchro
 *  @brief XBT synchronization tools
 * 
 *  This section describes the XBT synchronization tools. It defines types and 
 *  functions very close to the pthread API, but widly usable. When used from 
 *  the simulator, you will lock simulated processes as expected. When used 
 *  from GRAS programs compiled for in-situ execution, you have synchronization 
 *  mecanism portable to windows and UNIX. Nice, isn't it?
 * 
 *  @{
 */
  /** \brief Thread data type (opaque structure) */
typedef struct s_xbt_thread_ *xbt_thread_t;

XBT_PUBLIC(xbt_thread_t) xbt_thread_create(const char *name,
                                           void_f_pvoid_t start_routine,
                                           void *param, int joinable);
XBT_PUBLIC(xbt_thread_t) xbt_thread_self(void);
XBT_PUBLIC(const char *) xbt_thread_name(xbt_thread_t t);
XBT_PUBLIC(const char *) xbt_thread_self_name(void);
  /* xbt_thread_join frees the joined thread (ie the XBT wrapper around it, the OS frees the rest) */
XBT_PUBLIC(void) xbt_thread_join(xbt_thread_t thread);
  /* Ends the life of the poor victim (not always working if it's computing, but working if it's blocked in the OS) */
XBT_PUBLIC(void) xbt_thread_cancel(xbt_thread_t thread);
  /* suicide */
XBT_PUBLIC(void) xbt_thread_exit(void);
  /* current thread pass control to any possible thread wanting it */
XBT_PUBLIC(void) xbt_thread_yield(void);


  /** \brief Thread mutex data type (opaque structure) */
typedef struct s_xbt_mutex_ *xbt_mutex_t;

XBT_PUBLIC(xbt_mutex_t) xbt_mutex_init(void);
XBT_PUBLIC(void) xbt_mutex_acquire(xbt_mutex_t mutex);
XBT_PUBLIC(void) xbt_mutex_release(xbt_mutex_t mutex);
XBT_PUBLIC(void) xbt_mutex_timedacquire(xbt_mutex_t mutex, double delay);
XBT_PUBLIC(void) xbt_mutex_destroy(xbt_mutex_t mutex);


  /** \brief Thread condition data type (opaque structure) */
typedef struct s_xbt_cond_ *xbt_cond_t;

XBT_PUBLIC(xbt_cond_t) xbt_cond_init(void);
XBT_PUBLIC(void) xbt_cond_wait(xbt_cond_t cond, xbt_mutex_t mutex);
XBT_PUBLIC(void) xbt_cond_timedwait(xbt_cond_t cond,
                                    xbt_mutex_t mutex, double delay);
XBT_PUBLIC(void) xbt_cond_signal(xbt_cond_t cond);
XBT_PUBLIC(void) xbt_cond_broadcast(xbt_cond_t cond);
XBT_PUBLIC(void) xbt_cond_destroy(xbt_cond_t cond);

/** @} */

SG_END_DECL()
#endif                          /* _XBT_THREAD_H */
