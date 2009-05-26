/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_TMGR_H
#define _SURF_TMGR_H

#include "xbt/heap.h"
#include "xbt/dynar.h"
#include "surf/maxmin.h"

typedef struct tmgr_history *tmgr_history_t;
typedef struct tmgr_trace *tmgr_trace_t;
typedef struct tmgr_trace_event *tmgr_trace_event_t;

/* Creation functions */
XBT_PUBLIC(tmgr_history_t) tmgr_history_new(void);
XBT_PUBLIC(void) tmgr_history_free(tmgr_history_t history);

XBT_PUBLIC(tmgr_trace_t) tmgr_trace_new(const char *filename);
XBT_PUBLIC(tmgr_trace_t) tmgr_trace_new_from_string(const char *id,
                                                    const char *input,
                                                    double periodicity);
XBT_PUBLIC(tmgr_trace_t) tmgr_empty_trace_new(void);
XBT_PUBLIC(void) tmgr_trace_free(tmgr_trace_t trace);

XBT_PUBLIC(tmgr_trace_event_t) tmgr_history_add_trace(tmgr_history_t history,
                                                      tmgr_trace_t trace,
                                                      double start_time,
                                                      unsigned int offset,
                                                      void *model);

/* Access functions */
XBT_PUBLIC(double) tmgr_history_next_date(tmgr_history_t history);
XBT_PUBLIC(tmgr_trace_event_t) tmgr_history_get_next_event_leq(tmgr_history_t
                                                               history,
                                                               double date,
                                                               double *value,
                                                               void **model);

XBT_PUBLIC(void) tmgr_finalize(void);

#endif /* _SURF_TMGR_H */
