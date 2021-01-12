/* xbt/synchro.h -- Simulated synchronization                               */

/* Copyright (c) 2009-2021. The SimGrid Team.                               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_SYNCHRO_H
#define XBT_SYNCHRO_H

#include "simgrid/cond.h"
#include "simgrid/mutex.h"
#include <xbt/misc.h> /* SG_BEGIN_DECL */

#warning xbt/synchro.h is deprecated and will be removed in v3.28.

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
typedef sg_mutex_t xbt_mutex_t;

/** @brief Creates a new mutex variable */
XBT_ATTRIB_DEPRECATED_v328("Please use sg_mutex_init") static inline xbt_mutex_t xbt_mutex_init(void)
{
  return sg_mutex_init();
}

/** @brief Blocks onto the given mutex variable */
XBT_ATTRIB_DEPRECATED_v328("Please use sg_mutex_lock") static inline void xbt_mutex_acquire(xbt_mutex_t mutex)
{
  sg_mutex_lock(mutex);
}

/** @brief Tries to block onto the given mutex variable
 * Tries to lock a mutex, return 1 if the mutex is unlocked, else 0.
 * This function does not block and wait for the mutex to be unlocked.
 * @param mutex The mutex
 * @return 1 - mutex free, 0 - mutex used
 */
XBT_ATTRIB_DEPRECATED_v328("Please use sg_mutex_try_lock") static inline int xbt_mutex_try_acquire(xbt_mutex_t mutex)
{
  return sg_mutex_try_lock(mutex);
}

/** @brief Releases the given mutex variable */
XBT_ATTRIB_DEPRECATED_v328("Please use sg_mutex_unlock") static inline void xbt_mutex_release(xbt_mutex_t mutex)
{
  sg_mutex_unlock(mutex);
}

/** @brief Destroys the given mutex variable */
XBT_ATTRIB_DEPRECATED_v328("Please use sg_mutex_destroy") static inline void xbt_mutex_destroy(xbt_mutex_t mutex)
{
  sg_mutex_destroy(mutex);
}

/** @brief Thread condition data type (opaque object)
 *  @hideinitializer
 */
typedef sg_cond_t xbt_cond_t;

/** @brief Creates a condition variable */
XBT_ATTRIB_DEPRECATED_v328("Please use sg_cond_init") static inline xbt_cond_t xbt_cond_init(void)
{
  return sg_cond_init();
}

/** @brief Blocks onto the given condition variable */
XBT_ATTRIB_DEPRECATED_v328("Please use sg_cond_wait") static inline void xbt_cond_wait(xbt_cond_t cond,
                                                                                       xbt_mutex_t mutex)
{
  sg_cond_wait(cond, mutex);
}

/** @brief Blocks onto the given condition variable, but only for the given amount of time.
 *  @return 0 on success, 1 on timeout */
XBT_ATTRIB_DEPRECATED_v328("Please use sg_cond_wait_for") static inline int xbt_cond_timedwait(xbt_cond_t cond,
                                                                                               xbt_mutex_t mutex,
                                                                                               double delay)
{
  return sg_cond_wait_for(cond, mutex, delay);
}

/** @brief Signals the given mutex variable */
XBT_ATTRIB_DEPRECATED_v328("Please use sg_cond_notify_one") static inline void xbt_cond_signal(xbt_cond_t cond)
{
  sg_cond_notify_one(cond);
}

/** @brief Broadcasts the given mutex variable */
XBT_ATTRIB_DEPRECATED_v328("Please use sg_cond_notify_all") static inline void xbt_cond_broadcast(xbt_cond_t cond)
{
  sg_cond_notify_all(cond);
}

/** @brief Destroys the given mutex variable */
XBT_ATTRIB_DEPRECATED_v328("Please use sg_cond_destroy") static inline void xbt_cond_destroy(xbt_cond_t cond)
{
  sg_cond_destroy(cond);
}

/** @} */

SG_END_DECL
#endif /* XBT_SYNCHRO_H */
