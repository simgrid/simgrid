/* Copyright (c) 2004-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/resource/profile/Profile.hpp"
#include "simgrid/forward.h"
#include "src/kernel/resource/profile/DatedValue.hpp"
#include "src/kernel/resource/profile/Event.hpp"
#include "src/kernel/resource/profile/FutureEvtSet.hpp"
#include "src/surf/surf_interface.hpp"

#include <boost/algorithm/string.hpp>
#include <fstream>
#include <ostream>
#include <sstream>
#include <unordered_map>
#include <vector>

static std::unordered_map<std::string, simgrid::kernel::profile::Profile*> trace_list;

namespace simgrid {
namespace kernel {
namespace profile {

Profile::Profile()
{
  /* Add the first fake event storing the time at which the trace begins */
  DatedValue val(0, -1);
  event_list.push_back(val);
}
Profile::~Profile() = default;

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

Profile* Profile::from_string(const std::string& name, const std::string& input, double periodicity)
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

    XBT_ATTRIB_UNUSED int res = sscanf(val.c_str(), "%lg  %lg\n", &event.date_, &event.value_);
    xbt_assert(res == 2, "%s:%d: Syntax error in trace\n%s", name.c_str(), linecount, input.c_str());

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
Profile* Profile::from_file(const std::string& path)
{
  xbt_assert(not path.empty(), "Cannot parse a trace from an empty filename");
  xbt_assert(trace_list.find(path) == trace_list.end(), "Refusing to define trace %s twice", path.c_str());

  const std::ifstream* f = surf_ifsopen(path);
  xbt_assert(not f->fail(), "Cannot open file '%s' (path=%s)", path.c_str(), (boost::join(surf_path, ":")).c_str());

  std::stringstream buffer;
  buffer << f->rdbuf();
  delete f;

  return Profile::from_string(path, buffer.str(), -1);
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
