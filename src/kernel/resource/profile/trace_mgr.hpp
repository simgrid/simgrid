/* Copyright (c) 2004-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_PMGR_H
#define SURF_PMGR_H

#include "simgrid/forward.h"
#include "xbt/sysdep.h"

#include <queue>
#include <vector>

/* Iterator within a trace */
namespace simgrid {
namespace kernel {
namespace profile {
/** @brief Links a profile to a resource */
class Event {
public:
  Profile* profile;
  unsigned int idx;
  resource::Resource* resource;
  bool free_me;
};

} // namespace profile
} // namespace kernel
} // namespace simgrid
extern XBT_PRIVATE simgrid::kernel::profile::FutureEvtSet future_evt_set;

/**
 * @brief Free a trace event structure
 *
 * This function frees a trace_event if it can be freed, ie, if it has the free_me flag set to 1.
 * This flag indicates whether the structure is still used somewhere or not.
 * When the structure is freed, the argument is set to nullptr
 */
XBT_PUBLIC void tmgr_trace_event_unref(simgrid::kernel::profile::Event** trace_event);

XBT_PUBLIC void tmgr_finalize();

XBT_PUBLIC simgrid::kernel::profile::Profile* tmgr_trace_new_from_file(std::string filename);
XBT_PUBLIC simgrid::kernel::profile::Profile* tmgr_trace_new_from_string(std::string id, std::string input,
                                                                         double periodicity);

namespace simgrid {
namespace kernel {
namespace profile {

/** @brief Modeling of the availability profile (due to an external load) or the churn
 *
 * There is 4 main concepts in this module:
 * - #simgrid::kernel::profile::DatedValue: a pair <timestamp, value> (both are of type double)
 * - #simgrid::kernel::profile::Profile: a list of dated values
 * - #simgrid::kernel::profile::Event: links a given trace to a given SimGrid resource.
 *   A Cpu for example has 2 kinds of events: state (ie, is it ON/OFF) and speed,
 *   while a link has 3 iterators: state, bandwidth and latency.
 * - #simgrid::kernel::profile::FutureEvtSet: makes it easy to find the next occuring event of all profiles
 */
class XBT_PUBLIC DatedValue {
public:
  double date_          = 0;
  double value_         = 0;
  explicit DatedValue() = default;
  explicit DatedValue(double d, double v) : date_(d), value_(v) {}
  bool operator==(DatedValue const& e2) const;
  bool operator!=(DatedValue const& e2) const { return not(*this == e2); }
};
std::ostream& operator<<(std::ostream& out, const DatedValue& e);

/** @brief A profile is a set of timed values, encoding the value that a variable takes at what time
 *
 * It is useful to model dynamic platforms, where an external load that makes the resource availability change over
 * time. To model that, you have to set several profiles per resource: one for the on/off state and one for each
 * numerical value (computational speed, bandwidth and/or latency).
 */
class XBT_PUBLIC Profile {
public:
  /**  Creates an empty trace */
  explicit Profile();
  virtual ~Profile();
  // private:
  std::vector<DatedValue> event_list;
};

/** @brief Future Event Set (collection of iterators over the traces)
 * That's useful to quickly know which is the next occurring event in a set of traces. */
class XBT_PUBLIC FutureEvtSet {
public:
  FutureEvtSet();
  virtual ~FutureEvtSet();
  double next_date() const;
  Event* pop_leq(double date, double* value, resource::Resource** resource);
  Event* add_trace(Profile* profile, resource::Resource* resource);

private:
  typedef std::pair<double, Event*> Qelt;
  std::priority_queue<Qelt, std::vector<Qelt>, std::greater<Qelt>> heap_;
};

} // namespace profile
} // namespace kernel
} // namespace simgrid

#endif /* SURF_PMGR_H */
