/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/dict.h"
#include "trace_mgr_private.h"
#include "surf_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_trace, surf, "Surf trace management");

static xbt_dict_t trace_list = NULL;

tmgr_history_t tmgr_history_new(void)
{
  tmgr_history_t h;

  h = xbt_new0(s_tmgr_history_t, 1);

  h->heap = xbt_heap_new(8, xbt_free_f);        /* Why 8 ? Well, why not... */

  return h;
}

void tmgr_history_free(tmgr_history_t h)
{
  xbt_heap_free(h->heap);
  free(h);
}

tmgr_trace_t tmgr_trace_new_from_string(const char *id, const char *input,
                                        double periodicity, double timestep)
{
  tmgr_trace_t trace = NULL;
  int linecount = 0;
  s_tmgr_event_t event;
  tmgr_event_t last_event = NULL;
  xbt_dynar_t list;
  unsigned int cpt;
  char *val;

  if (trace_list) {
    trace = xbt_dict_get_or_null(trace_list, id);
    if (trace) {
      WARN1("Ignoring redefinition of trace %s", id);
      return trace;
    }
  }

  xbt_assert1(periodicity >= 0,
              "Invalid periodicity %lg (must be positive)", periodicity);

  trace = xbt_new0(s_tmgr_trace_t, 1);
  trace->event_list = xbt_dynar_new(sizeof(s_tmgr_event_t), NULL);

  list = xbt_str_split(input, "\n\r");

  xbt_dynar_foreach(list, cpt, val) {
    linecount++;
    xbt_str_trim(val, " \t\n\r\x0B");
    if (val[0] == '#' || val[0] == '\0' || val[0] == '%')
      continue;

    if (sscanf(val, "PERIODICITY " "%lg" "\n", &periodicity) == 1)
      continue;

    if (sscanf(val, "TIMESTEP " "%lg" "\n", &timestep) == 1)
      continue;

    if (sscanf(val, "%lg" " " "%lg" "\n", &event.delta, &event.value) != 2)
      xbt_die(bprintf
              ("%s:%d: Syntax error in trace\n%s", id, linecount, input));

    if (last_event) {
      if (last_event->delta > event.delta) {
        xbt_die(bprintf
                ("%s:%d: Invalid trace: Events must be sorted, but time %lg > time %lg.\n%s",
                 id, linecount, last_event->delta, event.delta, input));
      }
      last_event->delta = event.delta - last_event->delta;
    }
    xbt_dynar_push(trace->event_list, &event);
    last_event =
      xbt_dynar_get_ptr(trace->event_list,
                        xbt_dynar_length(trace->event_list) - 1);
  }
  if (last_event)
    last_event->delta = periodicity;

  trace->timestep = timestep;

  if (!trace_list)
    trace_list = xbt_dict_new();

  xbt_dict_set(trace_list, id, (void *) trace,
               (void (*)(void *)) tmgr_trace_free);

  xbt_dynar_free(&list);
  return trace;
}

tmgr_trace_t tmgr_trace_new(const char *filename)
{
  FILE *f = NULL;
  tmgr_trace_t trace = NULL;

  if ((!filename) || (strcmp(filename, "") == 0))
    return NULL;

  if (trace_list) {
    trace = xbt_dict_get_or_null(trace_list, filename);
    if (trace) {
      WARN1("Ignoring redefinition of trace %s", filename);
      return trace;
    }
  }

  f = surf_fopen(filename, "r");
  xbt_assert2(f!=NULL, "Cannot open file '%s' (path=%s)", filename,
       xbt_str_join(surf_path,":"));

  char *tstr = xbt_str_from_file(f);
  fclose(f);
  trace = tmgr_trace_new_from_string(filename, tstr, 0., 10.);
  xbt_free(tstr);

  return trace;
}

tmgr_trace_t tmgr_empty_trace_new(void)
{
  tmgr_trace_t trace = NULL;
  s_tmgr_event_t event;

  trace = xbt_new0(s_tmgr_trace_t, 1);
  trace->event_list = xbt_dynar_new(sizeof(s_tmgr_event_t), NULL);

  event.delta = 0.0;
  event.value = 0.0;
  xbt_dynar_push(trace->event_list, &event);

  return trace;
}

void tmgr_trace_free(tmgr_trace_t trace)
{
  if (!trace)
    return;
  xbt_dynar_free(&(trace->event_list));
  free(trace);
}

tmgr_trace_event_t tmgr_history_add_trace(tmgr_history_t h,
                                          tmgr_trace_t trace,
                                          double start_time,
                                          unsigned int offset, void *model)
{
  tmgr_trace_event_t trace_event = NULL;

  trace_event = xbt_new0(s_tmgr_trace_event_t, 1);
  trace_event->trace = trace;
  trace_event->idx = offset;
  trace_event->model = model;

  xbt_assert0((trace_event->idx < xbt_dynar_length(trace->event_list)),
              "You're referring to an event that does not exist!");

  xbt_heap_push(h->heap, trace_event, start_time);

  return trace_event;
}

double tmgr_history_next_date(tmgr_history_t h)
{
  if (xbt_heap_size(h->heap))
    return (xbt_heap_maxkey(h->heap));
  else
    return -1.0;
}

tmgr_trace_event_t tmgr_history_get_next_event_leq(tmgr_history_t h,
                                                   double date,
                                                   double *value,
                                                   void **model)
{
  double event_date = tmgr_history_next_date(h);
  tmgr_trace_event_t trace_event = NULL;
  tmgr_event_t event = NULL;
  tmgr_trace_t trace = NULL;

  if (event_date > date)
    return NULL;

  if (!(trace_event = xbt_heap_pop(h->heap)))
    return NULL;

  trace = trace_event->trace;
  event = xbt_dynar_get_ptr(trace->event_list, trace_event->idx);

  *value = event->value;
  *model = trace_event->model;

  if (trace_event->idx < xbt_dynar_length(trace->event_list) - 1) {
    xbt_heap_push(h->heap, trace_event, event_date + event->delta);
    trace_event->idx++;
  } else if (event->delta > 0) {        /* Last element, checking for periodicity */
    xbt_heap_push(h->heap, trace_event, event_date + event->delta);
    trace_event->idx = 0;
  } else {                      /* We don't need this trace_event anymore */
    trace_event->free_me = 1;
  }

  return trace_event;
}

void tmgr_finalize(void)
{
  xbt_dict_free(&trace_list);
}

int tmgr_trace_event_free(tmgr_trace_event_t trace_event)
{
  if (trace_event->free_me) {
    xbt_free(trace_event);
    return 1;
  }
  return 0;
}
