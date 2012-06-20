/* Copyright (c) 2004, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_TMGR_PRIVATE_H
#define _SURF_TMGR_PRIVATE_H

#include "xbt/swag.h"
#include "xbt/heap.h"
#include "surf/trace_mgr.h"
#include "xbt/RngStream.h"

typedef struct tmgr_event {
  double delta;
  double value;
} s_tmgr_event_t, *tmgr_event_t;

enum e_trace_type {
  e_trace_list, e_trace_probabilist
};

enum e_event_generator_type {
  e_generator_uniform, e_generator_exponential, e_generator_weibull
};

typedef struct probabilist_event_generator {
  enum e_event_generator_type type;
  RngStream rng_stream;
  double next_value;
  union {
    struct {
      double min;
      double max;
    } s_uniform_parameters;
    struct {
      double rate;
    } s_exponential_parameters;
    struct {
      double scale;
      double shape;
    } s_weibull_parameters;
  };
} s_probabilist_event_generator_t;

typedef struct tmgr_trace {
  enum e_trace_type type;
  union {
    struct {
      xbt_dynar_t event_list;
    } s_list;
    struct {
      probabilist_event_generator_t event_generator[2];
      int next_event;
    } s_probabilist;
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

double tmgr_event_generator_next_value(probabilist_event_generator_t generator);

#endif                          /* _SURF_TMGR_PRIVATE_H */
