/* xbt/synchro_core.h -- Synchronization tools                              */
/* Usable in simulator, (or in real life when mixing with GRAS)             */

/* Copyright (c) 2009-2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* splited away from synchro.h since we are used by dynar.h, and synchro.h uses dynar */


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
  /** @brief Thread data type (opaque object)
   *  @hideinitializer
   */
typedef struct s_xbt_thread_ *xbt_thread_t;

/** @brief Creates a new thread.
 * 
 * @param name          the name used in the logs for the newly created thread
 * @param start_routine function to run
 * @param param         parameter to pass to the function to run
 * @param joinable      whether the new thread should be started joinable or detached
 */
XBT_PUBLIC(xbt_thread_t) xbt_thread_create(const char *name,
                                           void_f_pvoid_t start_routine,
                                           void *param, int joinable);

/** @brief Get a reference to the currently running thread */
XBT_PUBLIC(xbt_thread_t) xbt_thread_self(void);

/** @brief Get the name of a given thread */
XBT_PUBLIC(const char *) xbt_thread_name(xbt_thread_t t);

/** @brief Get a reference to the name of the currently running thread */
XBT_PUBLIC(const char *) xbt_thread_self_name(void);

/** @brief Wait for the termination of the given thread, and free it (ie the XBT wrapper around it, the OS frees the rest) */
XBT_PUBLIC(void) xbt_thread_join(xbt_thread_t thread);

/** @brief Ends the life of the poor victim (not always working if it's computing, but working if it's blocked in the OS) */
XBT_PUBLIC(void) xbt_thread_cancel(xbt_thread_t thread);

/** @brief commit suicide */
XBT_PUBLIC(void) xbt_thread_exit(void);

/** @brief the current thread passes control to any possible thread wanting it */
XBT_PUBLIC(void) xbt_thread_yield(void);



/** @brief Thread mutex data type (opaque object)
 *  @hideinitializer
 */
typedef struct s_xbt_mutex_ *xbt_mutex_t;

/** @brief Creates a new mutex variable */
XBT_PUBLIC(xbt_mutex_t) xbt_mutex_init(void);

/** @brief Blocks onto the given mutex variable */
XBT_PUBLIC(void) xbt_mutex_acquire(xbt_mutex_t mutex);

/** @brief Releases the given mutex variable */
XBT_PUBLIC(void) xbt_mutex_release(xbt_mutex_t mutex);

/** @brief Destroyes the given mutex variable */
XBT_PUBLIC(void) xbt_mutex_destroy(xbt_mutex_t mutex);


/** @brief Thread condition data type (opaque object)
 *  @hideinitializer
 */
typedef struct s_xbt_cond_ *xbt_cond_t;

/** @brief Creates a condition variable */
XBT_PUBLIC(xbt_cond_t) xbt_cond_init(void);

/** @brief Blocks onto the given condition variable */
XBT_PUBLIC(void) xbt_cond_wait(xbt_cond_t cond, xbt_mutex_t mutex);
/** @brief Blocks onto the given condition variable, but only for the given amount of time. a timeout exception is raised if it was impossible to acquire it in the given time frame */
XBT_PUBLIC(void) xbt_cond_timedwait(xbt_cond_t cond,
                                    xbt_mutex_t mutex, double delay);
/** @brief Signals the given mutex variable */
XBT_PUBLIC(void) xbt_cond_signal(xbt_cond_t cond);
/** @brief Broadcasts the given mutex variable */
XBT_PUBLIC(void) xbt_cond_broadcast(xbt_cond_t cond);
/** @brief Destroys the given mutex variable */
XBT_PUBLIC(void) xbt_cond_destroy(xbt_cond_t cond);

/** @} */

SG_END_DECL()
#endif                          /* _XBT_THREAD_H */
