/* xbt/synchro_core.h -- Synchronization tools                              */
/* Usable in simulator, (or in real life when mixing with GRAS)             */

/* Copyright (c) 2009-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* splited away from synchro.h since we are used by dynar.h, and synchro.h uses dynar */


#ifndef _XBT_THREAD_H
#define _XBT_THREAD_H

#include <simgrid/simix.h>

#include "xbt/misc.h"           /* SG_BEGIN_DECL */
#include "xbt/function_types.h"

SG_BEGIN_DECL()

/** @addtogroup XBT_synchro
 *  @brief XBT synchronization tools
 * 
 *  This section describes the XBT synchronization tools.
 *  This is a portability layer (for windows and UNIX) of a pthread-like API. Nice, isn't it?
 * 
 *  @{
 */

/** @brief Thread mutex data type (opaque object)
 *  @hideinitializer
 */
typedef struct s_smx_mutex_ *xbt_mutex_t;

/** @brief Creates a new mutex variable */
XBT_PUBLIC(xbt_mutex_t) xbt_mutex_init(void);

/** @brief Blocks onto the given mutex variable */
XBT_PUBLIC(void) xbt_mutex_acquire(xbt_mutex_t mutex);

/** @brief Tries to block onto the given mutex variable 
 * Tries to lock a mutex, return 1 if the mutex is unlocked, else 0.
 * This function does not block and wait for the mutex to be unlocked.
 * \param mutex The mutex
 * \return 1 - mutex free, 0 - mutex used
 */
XBT_PUBLIC(int) xbt_mutex_try_acquire(xbt_mutex_t mutex);

/** @brief Releases the given mutex variable */
XBT_PUBLIC(void) xbt_mutex_release(xbt_mutex_t mutex);

/** @brief Destroyes the given mutex variable */
XBT_PUBLIC(void) xbt_mutex_destroy(xbt_mutex_t mutex);


/** @brief Thread condition data type (opaque object)
 *  @hideinitializer
 */
typedef struct s_smx_cond_ *xbt_cond_t;

/** @brief Creates a condition variable */
XBT_PUBLIC(xbt_cond_t) xbt_cond_init(void);

/** @brief Blocks onto the given condition variable */
XBT_PUBLIC(void) xbt_cond_wait(xbt_cond_t cond, xbt_mutex_t mutex);
/** @brief Blocks onto the given condition variable, but only for the given amount of time. a timeout exception is
 *   raised if it was impossible to acquire it in the given time frame */
XBT_PUBLIC(void) xbt_cond_timedwait(xbt_cond_t cond, xbt_mutex_t mutex, double delay);
/** @brief Signals the given mutex variable */
XBT_PUBLIC(void) xbt_cond_signal(xbt_cond_t cond);
/** @brief Broadcasts the given mutex variable */
XBT_PUBLIC(void) xbt_cond_broadcast(xbt_cond_t cond);
/** @brief Destroys the given mutex variable */
XBT_PUBLIC(void) xbt_cond_destroy(xbt_cond_t cond);

#define XBT_BARRIER_SERIAL_PROCESS -1
typedef struct s_xbt_bar_ *xbt_bar_t;
XBT_PUBLIC(xbt_bar_t) xbt_barrier_init( unsigned int count);
XBT_PUBLIC(void) xbt_barrier_destroy(xbt_bar_t bar);
XBT_PUBLIC(int) xbt_barrier_wait(xbt_bar_t bar);

/** @} */

SG_END_DECL()
#endif                          /* _XBT_THREAD_H */
