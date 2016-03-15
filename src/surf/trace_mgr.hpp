/* Copyright (c) 2004-2007, 2009-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_TMGR_H
#define _SURF_TMGR_H

#include "xbt/heap.h"
#include "simgrid/forward.h"

SG_BEGIN_DECL()
#include "xbt/heap.h"

typedef struct tmgr_event {
  double delta;
  double value;
} s_tmgr_event_t, *tmgr_event_t;

/* Iterator within a trace */
typedef struct tmgr_trace_iterator {
  tmgr_trace_t trace;
  unsigned int idx;
  sg_resource_t resource;
  int free_me;
} s_tmgr_trace_event_t;
typedef struct tmgr_trace_iterator *tmgr_trace_iterator_t;

/* Creation functions */
XBT_PUBLIC(tmgr_trace_t) tmgr_empty_trace_new(void);
XBT_PUBLIC(void) tmgr_trace_free(tmgr_trace_t trace);
/**
 * \brief Free a trace event structure
 *
 * This function frees a trace_event if it can be freed, ie, if it has the free_me flag set to 1.
 * This flag indicates whether the structure is still used somewhere or not.
 * When the structure is freed, the argument is set to nullptr
*/
XBT_PUBLIC(void) tmgr_trace_event_unref(tmgr_trace_iterator_t *trace_event);

XBT_PUBLIC(void) tmgr_finalize(void);

XBT_PUBLIC(tmgr_trace_t) tmgr_trace_new_from_file(const char *filename);
XBT_PUBLIC(tmgr_trace_t) tmgr_trace_new_from_string(const char *id, const char *input, double periodicity);

SG_END_DECL()

#ifdef __cplusplus
namespace simgrid {
/** @brief Modeling of the resource variations, such as those due to an external load
 *
 * There is 3 main concepts in this module:
 * - #trace: a set of dated values, ie a list of pair <timestamp, value>
 * - #trace_iterator: links a given trace to a given simgrid resource. A Cpu for example has 2 iterators: state (ie, is it ON/OFF) and speed, while a link has 3 iterators: state, bandwidth and latency.
 * - #future_evt_set: makes it easy to find the next occuring event of all traces
 */
  namespace trace_mgr {

/** @brief A trace_iterator links a trace to a resource */
XBT_PUBLIC_CLASS trace_iterator {

};

/** @brief A trace is a set of timed values, encoding the value that a variable takes at what time *
 *
 * It is useful to model dynamic platforms, where an external load that makes the resource availability change over time.
 * To model that, you have to set several traces per resource: one for the on/off state and one for each numerical value (computational speed, bandwidt and latency).
 */
XBT_PUBLIC_CLASS trace {
public:
  /**  Creates an empty trace */
  trace();
  virtual ~trace();
//private:
  xbt_dynar_t event_list;
};

/** @brief Future Event Set (collection of iterators over the traces)
 * That's useful to quickly know which is the next occurring event in a set of traces. */
XBT_PUBLIC_CLASS future_evt_set {
public:
  future_evt_set();
  virtual ~future_evt_set();
  double next_date() const;
  tmgr_trace_iterator_t pop_leq(double date, double *value, simgrid::surf::Resource** resource);
  tmgr_trace_iterator_t add_trace(tmgr_trace_t trace, double start_time, simgrid::surf::Resource *resource);

private:
  // TODO: use a boost type for the heap (or a ladder queue)
  xbt_heap_t p_heap = xbt_heap_new(8, xbt_free_f); /* Content: only trace_events (yep, 8 is an arbitrary value) */
};

}} // namespace simgrid::trace_mgr
#endif /* C++ only */

#endif                          /* _SURF_TMGR_H */
