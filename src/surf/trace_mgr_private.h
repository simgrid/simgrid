/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_TMGR_PRIVATE_H
#define _SURF_TMGR_PRIVATE_H

#include "xbt/swag.h"
#include "xbt/heap.h"
#include "surf/trace_mgr.h"

typedef struct tmgr_event {
  double delta;
  double value;
} s_tmgr_event_t, *tmgr_event_t;

typedef struct tmgr_trace {
  xbt_dynar_t event_list;
} s_tmgr_trace_t;

typedef struct tmgr_trace_event {
  tmgr_trace_t trace;
  unsigned int idx;
  void *model;
} s_tmgr_trace_event_t;

typedef struct tmgr_history {
  xbt_heap_t heap;
} s_tmgr_history_t;

#endif /* _SURF_TMGR_PRIVATE_H */
