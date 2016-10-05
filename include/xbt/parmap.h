/* A thread pool.                                          */

/* Copyright (c) 2007, 2009-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_PARMAP_H
#define _XBT_PARMAP_H

#include "xbt/misc.h"           /* SG_BEGIN_DECL */
#include "xbt/function_types.h"
#include "xbt/dynar.h"

SG_BEGIN_DECL()

/** \addtogroup XBT_parmap
  * \ingroup XBT_misc  
  * \brief Parallel map.
  *
  * A function is applied to all elements of a dynar in parallel with n worker threads.
  * The worker threads are persistent until the destruction of the parmap.
  *
  * If there are more than n elements in the dynar, the worker threads are allowed to fetch themselves remaining work
  * with xbt_parmap_next() and execute it.
  *
  * \{
  */

/** \brief Parallel map data type (opaque type) */
typedef struct s_xbt_parmap *xbt_parmap_t;

/** \brief Synchronization mode of the worker threads of a parmap. */
typedef enum {
  XBT_PARMAP_POSIX,          /**< use POSIX synchronization primitives */
  XBT_PARMAP_FUTEX,          /**< use Linux futex system call */
  XBT_PARMAP_BUSY_WAIT,      /**< busy waits (no system calls, maximum CPU usage) */
  XBT_PARMAP_DEFAULT         /**< futex if available, posix otherwise */
} e_xbt_parmap_mode_t;

XBT_PUBLIC(xbt_parmap_t) xbt_parmap_new(unsigned int num_workers, e_xbt_parmap_mode_t mode);
XBT_PUBLIC(void) xbt_parmap_destroy(xbt_parmap_t parmap);
XBT_PUBLIC(void) xbt_parmap_apply(xbt_parmap_t parmap, void_f_pvoid_t fun, xbt_dynar_t data);
XBT_PUBLIC(void*) xbt_parmap_next(xbt_parmap_t parmap);

/** \} */

SG_END_DECL()

#endif
