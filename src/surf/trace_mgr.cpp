/* Copyright (c) 2004-2005, 2007, 2009-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/dict.h"
#include "src/surf/trace_mgr.hpp"
#include "surf_private.h"
#include "xbt/RngStream.h"
#include <math.h>
#include <unordered_map>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_trace, surf, "Surf trace management");

static std::unordered_map<const char *, simgrid::trace_mgr::trace*> trace_list;

simgrid::trace_mgr::trace::trace()
{
  event_list = xbt_dynar_new(sizeof(s_tmgr_event_t), NULL);
}

simgrid::trace_mgr::trace::~trace()
{
  xbt_dynar_free(&event_list);
}
simgrid::trace_mgr::future_evt_set::future_evt_set()
{
}

simgrid::trace_mgr::future_evt_set::~future_evt_set()
{
  xbt_heap_free(p_heap);
}

tmgr_trace_t tmgr_trace_new_from_string(const char *name, const char *input, double periodicity)
{
  int linecount = 0;
  tmgr_event_t last_event = NULL;
  unsigned int cpt;
  char *val;

  xbt_assert(trace_list.find(name) == trace_list.end(), "Refusing to define trace %s twice", name);
  xbt_assert(periodicity >= 0, "Invalid periodicity %g (must be positive)", periodicity);

  tmgr_trace_t trace = new simgrid::trace_mgr::trace();

  xbt_dynar_t list = xbt_str_split(input, "\n\r");
  xbt_dynar_foreach(list, cpt, val) {
    s_tmgr_event_t event;
    linecount++;
    xbt_str_trim(val, " \t\n\r\x0B");
    if (val[0] == '#' || val[0] == '\0' || val[0] == '%') // pass comments
      continue;

    if (sscanf(val, "PERIODICITY " "%lg" "\n", &periodicity) == 1)
      continue;

    xbt_assert(sscanf(val, "%lg" " " "%lg" "\n", &event.delta, &event.value) == 2,
        "%s:%d: Syntax error in trace\n%s", name, linecount, input);

    if (last_event) {
      xbt_assert(last_event->delta <= event.delta,
          "%s:%d: Invalid trace: Events must be sorted, but time %g > time %g.\n%s",
          name, linecount, last_event->delta, event.delta, input);

      last_event->delta = event.delta - last_event->delta;
    } else {
      if(event.delta > 0.0){
        s_tmgr_event_t first_event;
        first_event.delta=event.delta;
        first_event.value=-1.0;
        xbt_dynar_push(trace->event_list, &first_event);
      }
    }
    xbt_dynar_push(trace->event_list, &event);
    last_event = (tmgr_event_t)xbt_dynar_get_ptr(trace->event_list, xbt_dynar_length(trace->event_list) - 1);
  }
  if (last_event)
    last_event->delta = periodicity;

  trace_list.insert({xbt_strdup(name), trace});

  xbt_dynar_free(&list);
  return trace;
}

tmgr_trace_t tmgr_trace_new_from_file(const char *filename)
{
  xbt_assert(filename && filename[0], "Cannot parse a trace from the null or empty filename");
  xbt_assert(trace_list.find(filename) == trace_list.end(), "Refusing to define trace %s twice", filename);

  FILE *f = surf_fopen(filename, "r");
  xbt_assert(f != NULL,
      "Cannot open file '%s' (path=%s)", filename, xbt_str_join(surf_path, ":"));

  char *tstr = xbt_str_from_file(f);
  fclose(f);
  tmgr_trace_t trace = tmgr_trace_new_from_string(filename, tstr, 0.);
  xbt_free(tstr);

  return trace;
}

tmgr_trace_t tmgr_empty_trace_new(void)
{
  tmgr_trace_t trace = new simgrid::trace_mgr::trace();
  s_tmgr_event_t event;
  event.delta = 0.0;
  event.value = 0.0;
  xbt_dynar_push(trace->event_list, &event);

  return trace;
}

void tmgr_trace_free(tmgr_trace_t trace)
{
  delete trace;
}

/** @brief Registers a new trace into the future event set, and get an iterator over the integrated trace  */
tmgr_trace_iterator_t simgrid::trace_mgr::future_evt_set::add_trace(tmgr_trace_t trace, double start_time, surf::Resource *resource)
{
  tmgr_trace_iterator_t trace_iterator = NULL;

  trace_iterator = xbt_new0(s_tmgr_trace_event_t, 1);
  trace_iterator->trace = trace;
  trace_iterator->idx = 0;
  trace_iterator->resource = resource;

  xbt_assert((trace_iterator->idx < xbt_dynar_length(trace->event_list)), "Your trace should have at least one event!");

  xbt_heap_push(p_heap, trace_iterator, start_time);

  return trace_iterator;
}

/** @brief returns the date of the next occurring event (pure function) */
double simgrid::trace_mgr::future_evt_set::next_date() const
{
  if (xbt_heap_size(p_heap))
    return (xbt_heap_maxkey(p_heap));
  else
    return -1.0;
}

/** @brief Retrieves the next occurring event, or NULL if none happens before #date */
tmgr_trace_iterator_t simgrid::trace_mgr::future_evt_set::pop_leq(
    double date, double *value, simgrid::surf::Resource **resource)
{
  double event_date = next_date();
  if (event_date > date)
    return NULL;

  tmgr_trace_iterator_t trace_iterator = (tmgr_trace_iterator_t)xbt_heap_pop(p_heap);
  if (trace_iterator == NULL)
    return NULL;

  tmgr_trace_t trace = trace_iterator->trace;
  *resource = trace_iterator->resource;

  tmgr_event_t event = (tmgr_event_t)xbt_dynar_get_ptr(trace->event_list, trace_iterator->idx);

  *value = event->value;

  if (trace_iterator->idx < xbt_dynar_length(trace->event_list) - 1) {
    xbt_heap_push(p_heap, trace_iterator, event_date + event->delta);
    trace_iterator->idx++;
  } else if (event->delta > 0) {        /* Last element, checking for periodicity */
    xbt_heap_push(p_heap, trace_iterator, event_date + event->delta);
    trace_iterator->idx = 1; /* not 0 as the first event is a placeholder to handle when events really start */
  } else {                      /* We don't need this trace_event anymore */
    trace_iterator->free_me = 1;
  }

  return trace_iterator;
}

void tmgr_finalize(void)
{
  for (auto kv : trace_list)
    delete kv.second;
}

void tmgr_trace_event_unref(tmgr_trace_iterator_t *trace_event)
{
  if ((*trace_event)->free_me) {
    xbt_free(*trace_event);
    *trace_event = nullptr;
  }
}
