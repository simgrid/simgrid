/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_TMGR_H
#define _SURF_TMGR_H

#include "xbt/heap.h"
#include "xbt/dynar.h"
#include "surf/maxmin.h"

typedef struct tmgr_history *tmgr_history_t;
typedef struct tmgr_trace *tmgr_trace_t;

/* Creation functions */
tmgr_history_t tmgr_history_new(void);
void tmgr_history_free(tmgr_history_t history);

tmgr_trace_t tmgr_trace_new(const char *filename);
void tmgr_trace_free(tmgr_trace_t trace);

void tmgr_history_add_trace(tmgr_history_t history, tmgr_trace_t trace, 
			    xbt_heap_float_t start_time, int offset, 
			    void *resource);

/* Access functions */
xbt_heap_float_t tmgr_history_next_date(tmgr_history_t history);
int tmgr_history_get_next_event_leq(tmgr_history_t history,
				    xbt_heap_float_t date,
				    xbt_maxmin_float_t *value,
				    void **resource);

#endif				/* _SURF_TMGR_H */
