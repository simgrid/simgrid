/* Copyright (c) 2004-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/resource/profile/Profile.hpp"
#include "simgrid/forward.h"
#include "src/kernel/resource/profile/DatedValue.hpp"
#include "src/kernel/resource/profile/Event.hpp"
#include "src/kernel/resource/profile/FutureEvtSet.hpp"
#include "src/kernel/resource/profile/StochasticDatedValue.hpp"
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
  StochasticDatedValue stoval(0, -1);
  event_list.emplace_back(val);
  stochastic_event_list.emplace_back(stoval);
}
Profile::~Profile() = default;

/** @brief Register this profile for that resource onto that FES,
 * and get an iterator over the integrated trace  */
Event* Profile::schedule(FutureEvtSet* fes, resource::Resource* resource)
{
  auto* event     = new Event();
  event->profile  = this;
  event->idx      = 0;
  event->resource = resource;
  event->free_me  = false;

  xbt_assert((event->idx < event_list.size()), "Your profile should have at least one event!");

  fes_ = fes;
  fes_->add_event(0.0 /* start time */, event);
  if (stochastic) {
    xbt_assert(event->idx < stochastic_event_list.size(), "Your profile should have at least one stochastic event!");
    futureDV = stochastic_event_list.at(event->idx).get_datedvalue();
  }

  return event;
}

/** @brief Gets the next event from a profile */
DatedValue Profile::next(Event* event)
{
  double event_date  = fes_->next_date();

  if (not stochastic) {
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
  } else {
    DatedValue dateVal = futureDV;
    if (event->idx < stochastic_event_list.size() - 1) {
      event->idx++;
    } else if (stochasticloop) { /* We have reached the last element and we have to loop. */
      event->idx = 1;
    } else {
      event->free_me = true; /* We have reached the last element, but we don't need to loop. */
    }

    if (not event->free_me) { // In the case there is an element, we draw the next event
      futureDV = stochastic_event_list.at(event->idx).get_datedvalue();
      fes_->add_event(event_date + futureDV.date_, event);
    }
    return dateVal;
  }
}

Profile* Profile::from_string(const std::string& name, const std::string& input, double periodicity)
{
  int linecount                                    = 0;
  auto* profile                                    = new simgrid::kernel::profile::Profile();
  simgrid::kernel::profile::DatedValue* last_event = &(profile->event_list.back());

  xbt_assert(trace_list.find(name) == trace_list.end(), "Refusing to define trace %s twice", name.c_str());

  std::vector<std::string> list;
  boost::split(list, input, boost::is_any_of("\n\r"));
  for (auto val : list) {
    simgrid::kernel::profile::DatedValue event;
    simgrid::kernel::profile::StochasticDatedValue stochevent;
    linecount++;
    boost::trim(val);
    if (val[0] == '#' || val[0] == '\0' || val[0] == '%') // pass comments
      continue;
    if (sscanf(val.c_str(), "PERIODICITY %lg\n", &periodicity) == 1)
      continue;
    if (sscanf(val.c_str(), "LOOPAFTER %lg\n", &periodicity) == 1)
      continue;
    if (val == "STOCHASTIC LOOP") {
      profile->stochastic     = true;
      profile->stochasticloop = true;
      continue;
    }
    if (val == "STOCHASTIC") {
      profile->stochastic = true;
      continue;
    }

    if (profile->stochastic) {
      unsigned int i;
      unsigned int j;
      std::istringstream iss(val);
      std::vector<std::string> splittedval((std::istream_iterator<std::string>(iss)),
                                           std::istream_iterator<std::string>());

      xbt_assert(splittedval.size() > 0, "Invalid profile line");

      if (splittedval[0] == "DET") {
        stochevent.date_law = Distribution::DET;
        i                   = 2;
      } else if (splittedval[0] == "NORM" || splittedval[0] == "NORMAL" || splittedval[0] == "GAUSS" ||
                 splittedval[0] == "GAUSSIAN") {
        stochevent.date_law = Distribution::NORM;
        i                   = 3;
      } else if (splittedval[0] == "EXP" || splittedval[0] == "EXPONENTIAL") {
        stochevent.date_law = Distribution::EXP;
        i                   = 2;
      } else if (splittedval[0] == "UNIF" || splittedval[0] == "UNIFORM") {
        stochevent.date_law = Distribution::UNIF;
        i                   = 3;
      } else {
        xbt_assert(false, "Unknown law %s", splittedval[0].c_str());
        i = 0;
      }

      xbt_assert(splittedval.size() > i, "Invalid profile line");
      if (i == 2) {
        stochevent.date_params = {std::stod(splittedval[1])};
      } else if (i == 3) {
        stochevent.date_params = {std::stod(splittedval[1]), std::stod(splittedval[2])};
      }

      if (splittedval[i] == "DET") {
        stochevent.value_law = Distribution::DET;
        j                    = 1;
      } else if (splittedval[i] == "NORM" || splittedval[i] == "NORMAL" || splittedval[i] == "GAUSS" ||
                 splittedval[i] == "GAUSSIAN") {
        stochevent.value_law = Distribution::NORM;
        j                    = 2;
      } else if (splittedval[i] == "EXP" || splittedval[i] == "EXPONENTIAL") {
        stochevent.value_law = Distribution::EXP;
        j                    = 1;
      } else if (splittedval[i] == "UNIF" || splittedval[i] == "UNIFORM") {
        stochevent.value_law = Distribution::UNIF;
        j                    = 2;
      } else {
        xbt_assert(false, "Unknown law %s", splittedval[i].c_str());
        j = 0;
      }

      xbt_assert(splittedval.size() > i + j, "Invalid profile line");
      if (j == 1) {
        stochevent.value_params = {std::stod(splittedval[i + 1])};
      } else if (j == 2) {
        stochevent.value_params = {std::stod(splittedval[i + 1]), std::stod(splittedval[i + 2])};
      }

      profile->stochastic_event_list.emplace_back(stochevent);
    } else {
      XBT_ATTRIB_UNUSED int res = sscanf(val.c_str(), "%lg  %lg\n", &event.date_, &event.value_);
      xbt_assert(res == 2, "%s:%d: Syntax error in trace\n%s", name.c_str(), linecount, input.c_str());

      xbt_assert(last_event->date_ <= event.date_,
                 "%s:%d: Invalid trace: Events must be sorted, but time %g > time %g.\n%s", name.c_str(), linecount,
                 last_event->date_, event.date_, input.c_str());
      last_event->date_ = event.date_ - last_event->date_;

      profile->event_list.emplace_back(event);
      last_event = &(profile->event_list.back());
    }
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
