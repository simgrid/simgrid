/* A thread pool.                                          */

/* Copyright (c) 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_PARMAP_H
#define _XBT_PARMAP_H

#include "xbt/misc.h"           /* SG_BEGIN_DECL */
#include "xbt/function_types.h"
#include "xbt/dynar.h"

SG_BEGIN_DECL()

/** @addtogroup XBT_parmap
  * @brief Parallel map.
  *
  * A function is applied to the n first elements of a dynar in parallel,
  * where n is the number of threads. The threads are persistent until the
  * destruction of the parmap object.
  *
  * If there are more than n elements in the dynar, the worker threads should
  * fetch themselves remaining work with xbt_parmap_next() and execute it.
  *
  * @{
  */
  /** \brief Queue data type (opaque type) */

typedef struct s_xbt_parmap *xbt_parmap_t;

XBT_PUBLIC(xbt_parmap_t) xbt_parmap_new(unsigned int num_workers);
XBT_PUBLIC(void) xbt_parmap_destroy(xbt_parmap_t parmap);

XBT_PUBLIC(void) xbt_parmap_apply(xbt_parmap_t parmap,
                                  void_f_pvoid_t fun,
                                  xbt_dynar_t data);
XBT_PUBLIC(void*) xbt_parmap_next(xbt_parmap_t parmap);
XBT_PUBLIC(unsigned long) xbt_parmap_get_worker_id(xbt_parmap_t parmap);

/** @} */

SG_END_DECL()

#endif
