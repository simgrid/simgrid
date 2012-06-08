/* Copyright (c) 2004, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

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

enum e_trace_type {
  e_trace_list, e_trace_uniform, e_trace_exponential, e_trace_weibull
};

typedef struct tmgr_trace {
  enum e_trace_type type;
  union {
    struct {
      xbt_dynar_t event_list;
    } s_list;
    struct {
      double alpha;
      double beta;
      s_tmgr_event_t next_event;
      /* and probably other things */
    } s_uniform;
    struct {
      double lambda;
      s_tmgr_event_t next_event;
      /* and probably other things */
    } s_exponential;
    struct {
      double lambda;
      double k;
      s_tmgr_event_t next_event;
      /* and probably other things */
    } s_weibull;
  };
} s_tmgr_trace_t;


typedef struct tmgr_trace_event {
  tmgr_trace_t trace;
  unsigned int idx;
  void *model;
  int free_me;
} s_tmgr_trace_event_t;

typedef struct tmgr_history {
  xbt_heap_t heap;
} s_tmgr_history_t;

#endif                          /* _SURF_TMGR_PRIVATE_H */
