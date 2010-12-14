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
  PARMAP_WORK = 0,
  PARMAP_DESTROY
} e_xbt_parmap_flag_t;

typedef struct s_xbt_barrier{
  int futex;
  unsigned int thread_count;
  unsigned int threads_to_wait;
} s_xbt_barrier_t, *xbt_barrier_t;

/* Wait for at least num_threads threads to arrive to the barrier */
void xbt_barrier_init(xbt_barrier_t barrier, unsigned int threads_to_wait);
void xbt_barrier_wait(xbt_barrier_t barrier);


typedef struct s_xbt_parmap {
  e_xbt_parmap_flag_t status;
  xbt_barrier_t workers_ready;
  xbt_barrier_t workers_done;
  unsigned int num_workers;
  unsigned int workers_max_id;
  void_f_pvoid_t fun;
  xbt_dynar_t data;
} s_xbt_parmap_t;


#endif
