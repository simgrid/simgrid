/* A thread pool.                                          */

/* Copyright (c) 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_THREADPOOL_H
#define _XBT_THREADPOOL_H

#include "xbt/misc.h"           /* SG_BEGIN_DECL */
#include "xbt/function_types.h"

SG_BEGIN_DECL()

/** @addtogroup XBT_threadpool
  * @brief Pool of threads.
  *
  * Jobs can be queued and the dispacher process can wait for the completion
  * of all jobs.
  * The call to "queue job" is non-blocking except the maximum amount of
  * queued jobs is reached. In that case, it will block until a job is taken
  * by a worker
  * @{
  */
  /** \brief Queue data type (opaque type) */

typedef struct s_xbt_tpool *xbt_tpool_t;

XBT_PUBLIC(xbt_tpool_t) xbt_tpool_new(unsigned int num_workers,
                                      unsigned int max_jobs);

XBT_PUBLIC(void) xbt_tpool_queue_job(xbt_tpool_t tpool, 
                                     void_f_pvoid_t fun, 
                                     void* fun_arg);

XBT_PUBLIC(void) xbt_tpool_wait_all(xbt_tpool_t tpool);

XBT_PUBLIC(void) xbt_tpool_destroy(xbt_tpool_t tpool);

/** @} */

SG_END_DECL()

#endif