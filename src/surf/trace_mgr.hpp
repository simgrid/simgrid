/* Copyright (c) 2004-2007, 2009-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_TMGR_H
#define _SURF_TMGR_H

#include "xbt/heap.h"
#include "xbt/dynar.h"
#include "surf/maxmin.h"
#include "surf/datatypes.h"
#include "simgrid/platf_interface.h"

SG_BEGIN_DECL()
#include "xbt/base.h"
#include "xbt/swag.h"
#include "xbt/heap.h"
#include "trace_mgr.hpp"
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
      int is_state_trace;
      int next_event;
    } s_probabilist;
  };
} s_tmgr_trace_t;

/* Iterator within a trace */
typedef struct tmgr_trace_iterator {
  tmgr_trace_t trace;
  unsigned int idx;
  void *resource;
  int free_me;
} s_tmgr_trace_event_t;

/* Future Event Set (collection of iterators over the traces)
 * That's useful to quickly know which is the next occurring event in a set of traces. */
typedef struct tmgr_fes {
  xbt_heap_t heap; /* Content: only trace_events */
} s_tmgr_history_t;

XBT_PRIVATE double tmgr_event_generator_next_value(probabilist_event_generator_t generator);

/* Creation functions */
XBT_PUBLIC(tmgr_fes_t) tmgr_history_new(void);
XBT_PUBLIC(void) tmgr_history_free(tmgr_fes_t history);

XBT_PUBLIC(tmgr_trace_t) tmgr_empty_trace_new(void);
XBT_PUBLIC(void) tmgr_trace_free(tmgr_trace_t trace);
/**
 * \brief Free a trace event structure
 *
 * This function frees a trace_event if it can be freed, ie, if it has the free_me flag set to 1. This flag indicates whether the structure is still used somewhere or not.
 * \param  trace_event    Trace event structure
 * \return 1 if the structure was freed, 0 otherwise
*/
XBT_PUBLIC(int) tmgr_trace_event_free(tmgr_trace_iterator_t trace_event);

XBT_PUBLIC(tmgr_trace_iterator_t) tmgr_history_add_trace(tmgr_fes_t
                                                      history,
                                                      tmgr_trace_t trace,
                                                      double start_time,
                                                      unsigned int offset,
                                                      void *model);

/* Access functions */
XBT_PUBLIC(double) tmgr_history_next_date(tmgr_fes_t history);
XBT_PUBLIC(tmgr_trace_iterator_t)
    tmgr_history_get_next_event_leq(tmgr_fes_t history, double date,
                                double *value, void **model);

XBT_PUBLIC(void) tmgr_finalize(void);

SG_END_DECL()

#endif                          /* _SURF_TMGR_H */
