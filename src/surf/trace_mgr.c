/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/error.h"
#include "xbt/dict.h"
#include "trace_mgr_private.h"
#include <stdlib.h>

static xbt_dict_t trace_list = NULL;
static void _tmgr_trace_free(void *trace)
{
  tmgr_trace_free(trace);
}

tmgr_history_t tmgr_history_new(void)
{
  tmgr_history_t history;

  history = xbt_new0(s_tmgr_history_t, 1);

  history->heap = xbt_heap_new(8, xbt_free);	/* Why 8 ? Well, why not... */

  return history;
}

void tmgr_history_free(tmgr_history_t history)
{
  xbt_heap_free(history->heap);
  xbt_free(history);
}

tmgr_trace_t tmgr_trace_new(const char *filename)
{
  tmgr_trace_t trace = NULL;
  FILE *f = NULL;
  int linecount = 0;
  char line[256];
  xbt_heap_float_t current_time = 0.0, previous_time = 0.0;
  xbt_maxmin_float_t value = -1.0;
  xbt_heap_float_t periodicity = -1.0;	/* No periodicity by default */
  s_tmgr_event_t event;
  tmgr_event_t last_event = NULL;

  if (trace_list) {
    xbt_dict_get(trace_list, filename, (void **) &trace);
    if (trace)
      return trace;
  }

  /* Parsing et création de la trace */

  if ((f = fopen(filename, "r")) == NULL) {
    fprintf(stderr, "Cannot open file '%s'\n", filename);
    return NULL;
  }

  trace = xbt_new0(s_tmgr_trace_t, 1);
  trace->event_list = xbt_dynar_new(sizeof(s_tmgr_event_t), NULL);

  while (fgets(line, 256, f)) {
    linecount++;
    if ((line[0] == '#') || (line[0] == '\n') || (line[0] == '%'))
      continue;

    if (sscanf(line, "PERIODICITY " XBT_HEAP_FLOAT_T "\n", &(periodicity))
	== 1) {
      if (periodicity <= 0) {
	fprintf(stderr,
		"%s,%d: Syntax error. Periodicity has to be positive\n",
		filename, linecount);
	abort();
/* 	xbt_dynar_free(&(trace->event_list)); */
/* 	xbt_free(trace);	 */
/* 	return NULL; */
      }
      continue;
    }

    if (sscanf
	(line, XBT_HEAP_FLOAT_T " " XBT_MAXMIN_FLOAT_T "\n", &event.delta,
	 &event.value) != 2) {
      fprintf(stderr, "%s,%d: Syntax error\n", filename, linecount);
      abort();
/*       xbt_dynar_free(&(trace->event_list)); */
/*       xbt_free(trace);	 */
/*       return NULL; */
    }

    if (last_event) {
      if ((last_event->delta = event.delta - last_event->delta) <= 0) {
	fprintf(stderr,
		"%s,%d: Invalid trace value, events have to be sorted\n",
		filename, linecount);
	abort();
      }
    }
    xbt_dynar_push(trace->event_list, &event);
    last_event = xbt_dynar_get_ptr(trace->event_list,
				   xbt_dynar_length(trace->event_list) -
				   1);
    printf(XBT_HEAP_FLOAT_T " " XBT_MAXMIN_FLOAT_T "\n", event.delta,
	   event.value);
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


void tmgr_trace_free(tmgr_trace_t trace)
{
  if (!trace)
    return;
  xbt_dynar_free(&(trace->event_list));
  xbt_free(trace);
}

void tmgr_history_add_trace(tmgr_history_t history, tmgr_trace_t trace,
			    xbt_heap_float_t start_time, int offset,
			    void *resource)
{
  tmgr_trace_event_t trace_event = NULL;


  trace_event = xbt_new0(s_tmgr_trace_event_t, 1);
  trace_event->trace = trace;
  trace_event->idx = offset;
  trace_event->resource = resource;

  if (trace_event->idx >= xbt_dynar_length(trace->event_list))
    abort();

  xbt_heap_push(history->heap, trace_event, start_time);
}

xbt_heap_float_t tmgr_history_next_date(tmgr_history_t history)
{
  if (xbt_heap_size(history->heap))
    return (xbt_heap_maxkey(history->heap));
  else
    return -1.0;
}

int tmgr_history_get_next_event_leq(tmgr_history_t history,
				    xbt_heap_float_t date,
				    xbt_maxmin_float_t * value,
				    void **resource)
{
  xbt_heap_float_t event_date = xbt_heap_maxkey(history->heap);
  tmgr_trace_event_t trace_event = NULL;
  tmgr_event_t event = NULL;
  tmgr_trace_t trace = NULL;

  if (event_date > date)
    return 0;

  if (!(trace_event = xbt_heap_pop(history->heap)))
    return 0;

  trace = trace_event->trace;
  event = xbt_dynar_get_ptr(trace->event_list, trace_event->idx);


  *value = event->value;
  *resource = trace_event->resource;

  if (trace_event->idx < xbt_dynar_length(trace->event_list) - 1) {
    xbt_heap_push(history->heap, trace_event, event_date + event->delta);
    trace_event->idx++;
  } else if (event->delta > 0) {	/* Last element, checking for periodicity */
    xbt_heap_push(history->heap, trace_event, event_date + event->delta);
    trace_event->idx = 0;
  } else {			/* We don't need this trace_event anymore */
    xbt_free(trace_event);
  }

  return 1;
}
