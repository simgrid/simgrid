/* Copyright (c) 2004-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/log.h"

#include "src/surf/surf_interface.hpp"
#include "src/surf/trace_mgr.hpp"
#include "surf_private.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <cmath>
#include <fstream>
#include <sstream>
#include <unordered_map>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_trace, surf, "Surf trace management");

static std::unordered_map<std::string, simgrid::kernel::profile::Profile*> trace_list;

static inline bool doubleEq(double d1, double d2)
{
  return fabs(d1 - d2) < 0.0001;
}
namespace simgrid {
namespace kernel {
namespace profile {

bool DatedValue::operator==(DatedValue e2)
{
  return (doubleEq(date_, e2.date_)) && (doubleEq(value_, e2.value_));
}
std::ostream& operator<<(std::ostream& out, const DatedValue& e)
{
  out << e.date_ << " " << e.value_;
  return out;
}

Profile::Profile()
{
  /* Add the first fake event storing the time at which the trace begins */
  DatedValue val(0, -1);
  event_list.push_back(val);
}
Profile::~Profile()          = default;
FutureEvtSet::FutureEvtSet() = default;
FutureEvtSet::~FutureEvtSet()
{
  while (not heap_.empty()) {
    delete heap_.top().second;
    heap_.pop();
  }
}
} // namespace profile
} // namespace kernel
} // namespace simgrid

simgrid::kernel::profile::Profile* tmgr_trace_new_from_string(std::string name, std::string input, double periodicity)
{
  int linecount = 0;
  simgrid::kernel::profile::Profile* trace         = new simgrid::kernel::profile::Profile();
  simgrid::kernel::profile::DatedValue* last_event = &(trace->event_list.back());

  xbt_assert(trace_list.find(name) == trace_list.end(), "Refusing to define trace %s twice", name.c_str());

  std::vector<std::string> list;
  boost::split(list, input, boost::is_any_of("\n\r"));
  for (auto val : list) {
    simgrid::kernel::profile::DatedValue event;
    linecount++;
    boost::trim(val);
    if (val[0] == '#' || val[0] == '\0' || val[0] == '%') // pass comments
      continue;
    if (sscanf(val.c_str(), "PERIODICITY %lg\n", &periodicity) == 1)
      continue;
    if (sscanf(val.c_str(), "LOOPAFTER %lg\n", &periodicity) == 1)
      continue;

    xbt_assert(sscanf(val.c_str(), "%lg  %lg\n", &event.date_, &event.value_) == 2, "%s:%d: Syntax error in trace\n%s",
               name.c_str(), linecount, input.c_str());

    xbt_assert(last_event->date_ <= event.date_,
               "%s:%d: Invalid trace: Events must be sorted, but time %g > time %g.\n%s", name.c_str(), linecount,
               last_event->date_, event.date_, input.c_str());
    last_event->date_ = event.date_ - last_event->date_;

    trace->event_list.push_back(event);
    last_event = &(trace->event_list.back());
  }
  if (last_event) {
    if (periodicity > 0) {
      last_event->date_ = periodicity + trace->event_list.at(0).date_;
    } else {
      last_event->date_ = -1;
    }
  }

  trace_list.insert({name, trace});

  return trace;
}

simgrid::kernel::profile::Profile* tmgr_trace_new_from_file(std::string filename)
{
  xbt_assert(not filename.empty(), "Cannot parse a trace from an empty filename");
  xbt_assert(trace_list.find(filename) == trace_list.end(), "Refusing to define trace %s twice", filename.c_str());

  std::ifstream* f = surf_ifsopen(filename);
  xbt_assert(not f->fail(), "Cannot open file '%s' (path=%s)", filename.c_str(), (boost::join(surf_path, ":")).c_str());

  std::stringstream buffer;
  buffer << f->rdbuf();
  delete f;

  return tmgr_trace_new_from_string(filename, buffer.str(), -1);
}
namespace simgrid {
namespace kernel {
namespace profile {

/** @brief Registers a new trace into the future event set, and get an iterator over the integrated trace  */
Event* FutureEvtSet::add_trace(Profile* profile, resource::Resource* resource)
{
  Event* event    = new Event();
  event->profile  = profile;
  event->idx      = 0;
  event->resource = resource;
  event->free_me  = false;

  xbt_assert((event->idx < profile->event_list.size()), "Your profile should have at least one event!");

  heap_.emplace(0.0 /* start time */, event);

  return event;
}

/** @brief returns the date of the next occurring event */
double FutureEvtSet::next_date() const
{
  return heap_.empty() ? -1.0 : heap_.top().first;
}

/** @brief Retrieves the next occurring event, or nullptr if none happens before date */
Event* FutureEvtSet::pop_leq(double date, double* value, resource::Resource** resource)
{
  double event_date = next_date();
  if (event_date > date)
    return nullptr;

  if (heap_.empty())
    return nullptr;
  Event* event = heap_.top().second;
  heap_.pop();

  Profile* profile = event->profile;
  *resource        = event->resource;

  DatedValue dateVal = profile->event_list.at(event->idx);

  *value = dateVal.value_;

  if (event->idx < profile->event_list.size() - 1) {
    heap_.emplace(event_date + dateVal.date_, event);
    event->idx++;
  } else if (dateVal.date_ > 0) { /* Last element. Shall we loop? */
    heap_.emplace(event_date + dateVal.date_, event);
    event->idx = 1; /* idx=0 is a placeholder to store when events really start */
  } else {          /* If we don't loop, we don't need this event anymore */
    event->free_me = true;
  }

  return event;
}
} // namespace profile
} // namespace kernel
} // namespace simgrid
void tmgr_finalize()
{
  for (auto const& kv : trace_list)
    delete kv.second;
  trace_list.clear();
}

void tmgr_trace_event_unref(simgrid::kernel::profile::Event** event)
{
  if ((*event)->free_me) {
    delete *event;
    *event = nullptr;
  }
}
