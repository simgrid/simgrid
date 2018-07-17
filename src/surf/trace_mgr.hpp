/* Copyright (c) 2004-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_TMGR_H
#define SURF_TMGR_H

#include "simgrid/forward.h"
#include "xbt/sysdep.h"

#include <queue>
#include <vector>

/* Iterator within a trace */
namespace simgrid {
namespace kernel {
namespace resource {
class TraceEvent {
public:
  tmgr_trace_t trace;
  unsigned int idx;
  Resource* resource;
  bool free_me;
};

} // namespace resource
} // namespace kernel
} // namespace simgrid
typedef simgrid::kernel::resource::TraceEvent* tmgr_trace_event_t;
extern XBT_PRIVATE simgrid::trace_mgr::future_evt_set future_evt_set;

/**
 * @brief Free a trace event structure
 *
 * This function frees a trace_event if it can be freed, ie, if it has the free_me flag set to 1.
 * This flag indicates whether the structure is still used somewhere or not.
 * When the structure is freed, the argument is set to nullptr
 */
XBT_PUBLIC void tmgr_trace_event_unref(tmgr_trace_event_t* trace_event);

XBT_PUBLIC void tmgr_finalize();

XBT_PUBLIC tmgr_trace_t tmgr_trace_new_from_file(std::string filename);
XBT_PUBLIC tmgr_trace_t tmgr_trace_new_from_string(std::string id, std::string input, double periodicity);

namespace simgrid {
/** @brief Modeling of the availability profile (due to an external load) or the churn
 *
 * There is 4 main concepts in this module:
 * - #simgrid::trace_mgr::DatedValue: a pair <timestamp, value> (both are of type double)
 * - #simgrid::trace_mgr::trace: a list of dated values
 * - #simgrid::trace_mgr::trace_event: links a given trace to a given SimGrid resource.
 *   A Cpu for example has 2 kinds of events: state (ie, is it ON/OFF) and speed,
 *   while a link has 3 iterators: state, bandwidth and latency.
 * - #simgrid::trace_mgr::future_evt_set: makes it easy to find the next occuring event of all traces
 */
namespace trace_mgr {
class XBT_PUBLIC DatedValue {
public:
  double date_          = 0;
  double value_         = 0;
  explicit DatedValue() = default;
  explicit DatedValue(double d, double v) : date_(d), value_(v) {}
  bool operator==(DatedValue e2);
  bool operator!=(DatedValue e2) { return not(*this == e2); }
};
std::ostream& operator<<(std::ostream& out, const DatedValue& e);

/** @brief A trace_iterator links a trace to a resource */
class XBT_PUBLIC trace_event {
};

/** @brief A trace is a set of timed values, encoding the value that a variable takes at what time *
 *
 * It is useful to model dynamic platforms, where an external load that makes the resource availability change over time.
 * To model that, you have to set several traces per resource: one for the on/off state and one for each numerical value (computational speed, bandwidth and latency).
 */
class XBT_PUBLIC trace {
public:
  /**  Creates an empty trace */
  explicit trace();
  virtual ~trace();
//private:
  std::vector<DatedValue> event_list;
};

/** @brief Future Event Set (collection of iterators over the traces)
 * That's useful to quickly know which is the next occurring event in a set of traces. */
class XBT_PUBLIC future_evt_set {
public:
  future_evt_set();
  virtual ~future_evt_set();
  double next_date() const;
  tmgr_trace_event_t pop_leq(double date, double* value, simgrid::kernel::resource::Resource** resource);
  tmgr_trace_event_t add_trace(tmgr_trace_t trace, simgrid::kernel::resource::Resource * resource);

private:
  typedef std::pair<double, tmgr_trace_event_t> Qelt;
  std::priority_queue<Qelt, std::vector<Qelt>, std::greater<Qelt>> heap_;
};

}} // namespace simgrid::trace_mgr

#endif /* SURF_TMGR_H */
