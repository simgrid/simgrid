/* Copyright (c) 2004-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"
#include "xbt/sysdep.h"

#include "src/kernel/resource/profile/trace_mgr.hpp"
#include "src/surf/surf_interface.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <cmath>
#include <fstream>
#include <sstream>
#include <unordered_map>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(profile, resource, "Surf profile management");

static std::unordered_map<std::string, simgrid::kernel::profile::Profile*> trace_list;

namespace simgrid {
namespace kernel {
namespace profile {

bool DatedValue::operator==(DatedValue const& e2) const
{
  return (fabs(date_ - e2.date_) < 0.0001) && (fabs(value_ - e2.value_) < 0.0001);
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

/** @brief Register this profile for that resource onto that FES,
 * and get an iterator over the integrated trace  */
Event* Profile::schedule(FutureEvtSet* fes, resource::Resource* resource)
{
  Event* event    = new Event();
  event->profile  = this;
  event->idx      = 0;
  event->resource = resource;
  event->free_me  = false;

  xbt_assert((event->idx < event_list.size()), "Your profile should have at least one event!");

  fes_ = fes;
  fes_->add_event(0.0 /* start time */, event);

  return event;
}

/** @brief Gets the next event from a profile */
DatedValue Profile::next(Event* event)
{
  double event_date  = fes_->next_date();
  DatedValue dateVal = event_list.at(event->idx);

  if (event->idx < event_list.size() - 1) {
    fes_->add_event(event_date + dateVal.date_, event);
    event->idx++;
  } else if (dateVal.date_ > 0) { /* Last element. Shall we loop? */
    fes_->add_event(event_date + dateVal.date_, event);
    event->idx = 1; /* idx=0 is a placeholder to store when events really start */
  } else {          /* If we don't loop, we don't need this event anymore */
    event->free_me = true;
  }

  return dateVal;
}

Profile* Profile::from_string(std::string name, std::string input, double periodicity)
{
  int linecount                                    = 0;
  simgrid::kernel::profile::Profile* profile       = new simgrid::kernel::profile::Profile();
  simgrid::kernel::profile::DatedValue* last_event = &(profile->event_list.back());

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

    profile->event_list.push_back(event);
    last_event = &(profile->event_list.back());
  }
  if (last_event) {
    if (periodicity > 0) {
      last_event->date_ = periodicity + profile->event_list.at(0).date_;
    } else {
      last_event->date_ = -1;
    }
  }

  trace_list.insert({name, profile});

  return profile;
}
Profile* Profile::from_file(std::string path)
{
  xbt_assert(not path.empty(), "Cannot parse a trace from an empty filename");
  xbt_assert(trace_list.find(path) == trace_list.end(), "Refusing to define trace %s twice", path.c_str());

  std::ifstream* f = surf_ifsopen(path);
  xbt_assert(not f->fail(), "Cannot open file '%s' (path=%s)", path.c_str(), (boost::join(surf_path, ":")).c_str());

  std::stringstream buffer;
  buffer << f->rdbuf();
  delete f;

  return Profile::from_string(path, buffer.str(), -1);
}
FutureEvtSet::FutureEvtSet() = default;
FutureEvtSet::~FutureEvtSet()
{
  while (not heap_.empty()) {
    delete heap_.top().second;
    heap_.pop();
  }
}

/** @brief Schedules an event to a future date */
void FutureEvtSet::add_event(double date, Event* evt)
{
  heap_.emplace(date, evt);
}

/** @brief returns the date of the next occurring event (or -1 if empty) */
double FutureEvtSet::next_date() const
{
  return heap_.empty() ? -1.0 : heap_.top().first;
}

/** @brief Retrieves the next occurring event, or nullptr if none happens before date */
Event* FutureEvtSet::pop_leq(double date, double* value, resource::Resource** resource)
{
  double event_date = next_date();
  if (event_date > date || heap_.empty())
    return nullptr;

  Event* event = heap_.top().second;
  Profile* profile = event->profile;
  DatedValue dateVal = profile->next(event);

  *resource = event->resource;
  *value = dateVal.value_;

  heap_.pop();

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
