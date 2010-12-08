/* Copyright (c) 2004, 2005, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_THREADPOOL_PRIVATE_H
#define _XBT_THREADPOOL_PRIVATE_H

#include "xbt/threadpool.h"
#include "xbt/xbt_os_thread.h"
#include "xbt/sysdep.h"
#include "xbt/dynar.h"
#include "xbt/log.h"

typedef enum{
  TPOOL_WAIT = 0,
  TPOOL_DESTROY
} e_xbt_tpool_flag_t;

typedef struct s_xbt_tpool {
  xbt_os_mutex_t mutex;           /* pool's mutex */
  xbt_os_cond_t job_posted;       /* job is posted */
  xbt_os_cond_t job_taken;        /* job is taken */
  xbt_os_cond_t job_done;         /* job is done */
  xbt_dynar_t jobs_queue;
  e_xbt_tpool_flag_t flags;
  unsigned int num_workers;
  unsigned int num_idle_workers;
  unsigned int max_jobs;
} s_xbt_tpool_t;

typedef struct s_xbt_tpool_job {
  void_f_pvoid_t fun;
  void *fun_arg;
} s_xbt_tpool_job_t, *xbt_tpool_job_t;

#endif
