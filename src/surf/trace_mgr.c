/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/error.h"
#include "xbt/dict.h"
#include "trace_mgr_private.h"
#include "surf_private.h"
#include <stdlib.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(trace, surf,
				"Logging specific to the SURF trace module");

static xbt_dict_t trace_list = NULL;
static void _tmgr_trace_free(void *trace)
{
  tmgr_trace_free(trace);
}

tmgr_history_t tmgr_history_new(void)
{
  tmgr_history_t h;

  h = xbt_new0(s_tmgr_history_t, 1);

  h->heap = xbt_heap_new(8, free);	/* Why 8 ? Well, why not... */

  return h;
}

void tmgr_history_free(tmgr_history_t h)
{
  xbt_heap_free(h->heap);
  free(h);
}

tmgr_trace_t tmgr_trace_new(const char *filename)
{
  tmgr_trace_t trace = NULL;
  FILE *f = NULL;
  int linecount = 0;
  char line[256];
  double periodicity = -1.0;	/* No periodicity by default */
  s_tmgr_event_t event;
  tmgr_event_t last_event = NULL;

  if (trace_list) {
    xbt_dict_get(trace_list, filename, (void **) &trace);
    if (trace)
      return trace;
  }

  if ((f = surf_fopen(filename, "r")) == NULL) {
    xbt_assert1(0, "Cannot open file '%s'", filename);
  }

  trace = xbt_new0(s_tmgr_trace_t, 1);
  trace->event_list = xbt_dynar_new(sizeof(s_tmgr_event_t), NULL);

  while (fgets(line, 256, f)) {
    linecount++;
    if ((line[0] == '#') || (line[0] == '\n') || (line[0] == '%'))
      continue;

    if (sscanf(line, "PERIODICITY " "%lg" "\n", &(periodicity))
	== 1) {
      if (periodicity <= 0) {
	xbt_assert2(0,
		    "%s,%d: Syntax error. Periodicity has to be positive",
		    filename, linecount);
      }
      continue;
    }

    if (sscanf
	(line, "%lg" " " "%lg" "\n", &event.delta, &event.value) != 2) {
      xbt_assert2(0, "%s,%d: Syntax error", filename, linecount);
    }

    if (last_event) {
      if ((last_event->delta = event.delta - last_event->delta) <= 0) {
	xbt_assert2(0,
		    "%s,%d: Invalid trace value, events have to be sorted",
		    filename, linecount);
      }
    }
    xbt_dynar_push(trace->event_list, &event);
    last_event = xbt_dynar_get_ptr(trace->event_list,
				   xbt_dynar_length(trace->event_list) -
				   1);
  }

  if (periodicity > 0) {
    if (last_event)
      last_event->delta = periodicity;
  }

  if (!trace_list)
    trace_list = xbt_dict_new();

  xbt_dict_set(trace_list, filename, (void *) trace, _tmgr_trace_free);

  fclose(f);

  return trace;
}

tmgr_trace_t tmgr_empty_trace_new(void)
{
  tmgr_trace_t trace = NULL;
  double periodicity = -1.0;	/* No periodicity by default */
  s_tmgr_event_t event;
  tmgr_event_t last_event = NULL;

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
					  double start_time, int offset,
					  void *resource)
{
  tmgr_trace_event_t trace_event = NULL;


  trace_event = xbt_new0(s_tmgr_trace_event_t, 1);
  trace_event->trace = trace;
  trace_event->idx = offset;
  trace_event->resource = resource;

  xbt_assert0((trace_event->idx < xbt_dynar_length(trace->event_list)),
	      "You're refering to an event that does not exist!");

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
						   void **resource)
{
  double event_date = xbt_heap_maxkey(h->heap);
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
  *resource = trace_event->resource;

  if (trace_event->idx < xbt_dynar_length(trace->event_list) - 1) {
    xbt_heap_push(h->heap, trace_event, event_date + event->delta);
    trace_event->idx++;
  } else if (event->delta > 0) {	/* Last element, checking for periodicity */
    xbt_heap_push(h->heap, trace_event, event_date + event->delta);
    trace_event->idx = 0;
  } else {			/* We don't need this trace_event anymore */
    free(trace_event);
  }

  return trace_event;
}

void tmgr_finalize(void)
{
  xbt_dict_free(&trace_list);
}
