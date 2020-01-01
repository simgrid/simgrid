/* A thread pool.                                          */

/* Copyright (c) 2007-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_PARMAP_H
#define XBT_PARMAP_H

/** @addtogroup XBT_parmap
 * @ingroup XBT_misc
 * @brief Parallel map.
 *
 * A function is applied to all elements of a std::vector in parallel with n worker threads.  The worker threads are
 * persistent until the destruction of the parmap.
 *
 * If there are more than n elements in the vector, the worker threads are allowed to fetch themselves remaining work
 * with method next() and execute it.
 *
 * @{
 */

/** @brief Synchronization mode of the worker threads of a parmap. */
typedef enum {
  XBT_PARMAP_POSIX,          /**< use POSIX synchronization primitives */
  XBT_PARMAP_FUTEX,          /**< use Linux futex system call */
  XBT_PARMAP_BUSY_WAIT,      /**< busy waits (no system calls, maximum CPU usage) */
  XBT_PARMAP_DEFAULT         /**< futex if available, posix otherwise */
} e_xbt_parmap_mode_t;

/** @} */

#endif
