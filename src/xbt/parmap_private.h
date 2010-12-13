/* Copyright (c) 2004, 2005, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_THREADPOOL_PRIVATE_H
#define _XBT_THREADPOOL_PRIVATE_H

#include "xbt/parmap.h"
#include "xbt/xbt_os_thread.h"
#include "xbt/sysdep.h"
#include "xbt/dynar.h"
#include "xbt/log.h"

typedef enum{
  PARMAP_WAIT = 0,
  PARMAP_WORK,
  PARMAP_DESTROY,
} e_xbt_parmap_flag_t;

typedef struct s_xbt_parmap {
  xbt_os_mutex_t mutex;           /* pool's mutex */
  xbt_os_cond_t job_posted;       /* job is posted */
  xbt_os_cond_t all_done;         /* job is done */
  e_xbt_parmap_flag_t *flags;     /* Per thread flag + lastone for the parmap */
  unsigned int num_workers;
  unsigned int num_idle_workers;
  unsigned int workers_max_id;
  void_f_pvoid_t fun;
  xbt_dynar_t data;
} s_xbt_parmap_t;

#endif
