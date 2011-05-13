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
#include "xbt/xbt_os_time.h"

typedef enum{
  PARMAP_WORK = 0,
  PARMAP_DESTROY
} e_xbt_parmap_flag_t;

#ifdef HAVE_FUTEX_H
typedef struct s_xbt_event{
  int work;
  int done;
  unsigned int thread_counter;
  unsigned int threads_to_wait;
}s_xbt_event_t, *xbt_event_t;

void xbt_event_init(xbt_event_t event);
void xbt_event_signal(xbt_event_t event);
void xbt_event_wait(xbt_event_t event);
void xbt_event_end(xbt_event_t event);
#endif

typedef struct s_xbt_parmap {
  e_xbt_parmap_flag_t status;
#ifdef HAVE_FUTEX_H
  xbt_event_t sync_event;
#endif
  unsigned int num_workers;
  unsigned int workers_max_id;
  void_f_pvoid_t fun;
  xbt_dynar_t data;
  unsigned int index;
} s_xbt_parmap_t;

#endif
