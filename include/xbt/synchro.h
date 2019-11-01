/* xbt/synchro.h -- Simulated synchronization                               */

/* Copyright (c) 2009-2019. The SimGrid Team.                               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_THREAD_H
#define XBT_THREAD_H

#include "simgrid/forward.h"
#include <xbt/function_types.h>
#include <xbt/misc.h> /* SG_BEGIN_DECL */

SG_BEGIN_DECL

/** @addtogroup XBT_synchro
 *  @brief XBT synchronization tools
 *
 *  This section describes the simulated synchronization mechanisms,
 *  that you can use in your simulation without deadlocks. See @ref
 *  faq_MIA_thread_synchronization for details.
 *
 *  @{
 */

/** @brief Thread mutex data type (opaque object)
 *  @hideinitializer
 */
#ifdef __cplusplus
typedef simgrid::kernel::activity::MutexImpl* xbt_mutex_t;
#else
typedef struct s_smx_mutex_* xbt_mutex_t;
#endif

/** @brief Creates a new mutex variable */
XBT_ATTRIB_DEPRECATED_v328("Please use sg_mutex_init") XBT_PUBLIC xbt_mutex_t xbt_mutex_init(void);

/** @brief Blocks onto the given mutex variable */
XBT_ATTRIB_DEPRECATED_v328("Please use sg_mutex_lock") XBT_PUBLIC void xbt_mutex_acquire(xbt_mutex_t mutex);

/** @brief Tries to block onto the given mutex variable
 * Tries to lock a mutex, return 1 if the mutex is unlocked, else 0.
 * This function does not block and wait for the mutex to be unlocked.
 * @param mutex The mutex
 * @return 1 - mutex free, 0 - mutex used
 */
XBT_ATTRIB_DEPRECATED_v328("Please use sg_mutex_try_lock") XBT_PUBLIC int xbt_mutex_try_acquire(xbt_mutex_t mutex);

/** @brief Releases the given mutex variable */
XBT_ATTRIB_DEPRECATED_v328("Please use sg_mutex_unlock") XBT_PUBLIC void xbt_mutex_release(xbt_mutex_t mutex);

/** @brief Destroys the given mutex variable */
XBT_ATTRIB_DEPRECATED_v328("Please use sg_mutex_destroy") XBT_PUBLIC void xbt_mutex_destroy(xbt_mutex_t mutex);

/** @brief Thread condition data type (opaque object)
 *  @hideinitializer
 */
#ifdef __cplusplus
typedef simgrid::kernel::activity::ConditionVariableImpl* xbt_cond_t;
#else
typedef struct s_smx_cond_* xbt_cond_t;
#endif

/** @brief Creates a condition variable */
XBT_ATTRIB_DEPRECATED_v328("Please use sg_cond_init") XBT_PUBLIC xbt_cond_t xbt_cond_init(void);

/** @brief Blocks onto the given condition variable */
XBT_ATTRIB_DEPRECATED_v328("Please use sg_cond_wait") XBT_PUBLIC void xbt_cond_wait(xbt_cond_t cond, xbt_mutex_t mutex);
/** @brief Blocks onto the given condition variable, but only for the given amount of time.
 *  @return 0 on success, 1 on timeout */
XBT_ATTRIB_DEPRECATED_v328("Please use sg_cond_wait_for") XBT_PUBLIC
    int xbt_cond_timedwait(xbt_cond_t cond, xbt_mutex_t mutex, double delay);
/** @brief Signals the given mutex variable */
XBT_ATTRIB_DEPRECATED_v328("Please use sg_cond_notify_one") XBT_PUBLIC void xbt_cond_signal(xbt_cond_t cond);
/** @brief Broadcasts the given mutex variable */
XBT_ATTRIB_DEPRECATED_v328("Please use sg_cond_notify_all") XBT_PUBLIC void xbt_cond_broadcast(xbt_cond_t cond);
/** @brief Destroys the given mutex variable */
XBT_ATTRIB_DEPRECATED_v328("Please use sg_cond_destroy") XBT_PUBLIC void xbt_cond_destroy(xbt_cond_t cond);

/** @} */

SG_END_DECL
#endif /* _XBT_THREAD_H */
