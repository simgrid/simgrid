/* Copyright (c) 2004-2005, 2007, 2009-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/dict.h"
#include "xbt/log.h"
#include "xbt/str.h"

#include "src/surf/surf_interface.hpp"
#include "src/surf/trace_mgr.hpp"
#include "surf_private.h"
#include "xbt/RngStream.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <fstream>
#include <math.h>
#include <sstream>
#include <unordered_map>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_trace, surf, "Surf trace management");

namespace tmgr = simgrid::trace_mgr;

static std::unordered_map<const char*, tmgr::trace*> trace_list;

static inline bool doubleEq(double d1, double d2)
{
  return fabs(d1 - d2) < 0.0001;
}
namespace simgrid {
namespace trace_mgr {

bool DatedValue::operator==(DatedValue e2)
{
  return (doubleEq(date_, e2.date_)) && (doubleEq(value_, e2.value_));
}
std::ostream& operator<<(std::ostream& out, const DatedValue& e)
{
  out << e.date_ << " " << e.value_;
  return out;
}

trace::trace()
{
  /* Add the first fake event storing the time at which the trace begins */
  tmgr::DatedValue val(0, -1);
  event_list.push_back(val);
}
trace::~trace()                  = default;
future_evt_set::future_evt_set() = default;
simgrid::trace_mgr::future_evt_set::~future_evt_set()
{
  xbt_heap_free(heap_);
}
}
}

tmgr_trace_t tmgr_trace_new_from_string(const char* name, std::string input, double periodicity)
{
  int linecount = 0;
  tmgr_trace_t trace           = new simgrid::trace_mgr::trace();
  tmgr::DatedValue* last_event = &(trace->event_list.back());

  xbt_assert(trace_list.find(name) == trace_list.end(), "Refusing to define trace %s twice", name);

  std::vector<std::string> list;
  boost::split(list, input, boost::is_any_of("\n\r"));
  for (auto val : list) {
    tmgr::DatedValue event;
    linecount++;
    boost::trim(val);
    if (val[0] == '#' || val[0] == '\0' || val[0] == '%') // pass comments
      continue;
    if (sscanf(val.c_str(), "PERIODICITY %lg\n", &periodicity) == 1)
      continue;
    if (sscanf(val.c_str(), "LOOPAFTER %lg\n", &periodicity) == 1)
      continue;

    xbt_assert(sscanf(val.c_str(), "%lg  %lg\n", &event.date_, &event.value_) == 2, "%s:%d: Syntax error in trace\n%s",
               name, linecount, input.c_str());

    xbt_assert(last_event->date_ <= event.date_,
               "%s:%d: Invalid trace: Events must be sorted, but time %g > time %g.\n%s", name, linecount,
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

  trace_list.insert({xbt_strdup(name), trace});

  return trace;
}

tmgr_trace_t tmgr_trace_new_from_file(const char *filename)
{
  xbt_assert(filename && filename[0], "Cannot parse a trace from the null or empty filename");
  xbt_assert(trace_list.find(filename) == trace_list.end(), "Refusing to define trace %s twice", filename);

  std::ifstream* f = surf_ifsopen(filename);
  xbt_assert(!f->fail(), "Cannot open file '%s' (path=%s)", filename, (boost::join(surf_path, ":")).c_str());

  std::stringstream buffer;
  buffer << f->rdbuf();
  delete f;

  return tmgr_trace_new_from_string(filename, buffer.str(), -1);
}

/** @brief Registers a new trace into the future event set, and get an iterator over the integrated trace  */
tmgr_trace_event_t simgrid::trace_mgr::future_evt_set::add_trace(tmgr_trace_t trace, surf::Resource* resource)
{
  tmgr_trace_event_t trace_iterator = nullptr;

  trace_iterator = xbt_new0(s_tmgr_trace_event_t, 1);
  trace_iterator->trace = trace;
  trace_iterator->idx = 0;
  trace_iterator->resource = resource;

  xbt_assert((trace_iterator->idx < trace->event_list.size()), "Your trace should have at least one event!");

  xbt_heap_push(heap_, trace_iterator, 0. /*start_time*/);

  return trace_iterator;
}

/** @brief returns the date of the next occurring event (pure function) */
double simgrid::trace_mgr::future_evt_set::next_date() const
{
  if (xbt_heap_size(heap_))
    return (xbt_heap_maxkey(heap_));
  return -1.0;
}

/** @brief Retrieves the next occurring event, or nullptr if none happens before #date */
tmgr_trace_event_t simgrid::trace_mgr::future_evt_set::pop_leq(double date, double* value,
                                                               simgrid::surf::Resource** resource)
{
  double event_date = next_date();
  if (event_date > date)
    return nullptr;

  tmgr_trace_event_t trace_iterator = (tmgr_trace_event_t)xbt_heap_pop(heap_);
  if (trace_iterator == nullptr)
    return nullptr;

  tmgr_trace_t trace = trace_iterator->trace;
  *resource = trace_iterator->resource;

  tmgr::DatedValue dateVal = trace->event_list.at(trace_iterator->idx);

  *value = dateVal.value_;

  if (trace_iterator->idx < trace->event_list.size() - 1) {
    xbt_heap_push(heap_, trace_iterator, event_date + dateVal.date_);
    trace_iterator->idx++;
  } else if (dateVal.date_ > 0) { /* Last element. Shall we loop? */
    xbt_heap_push(heap_, trace_iterator, event_date + dateVal.date_);
    trace_iterator->idx = 1; /* idx=0 is a placeholder to store when events really start */
  } else {                   /* If we don't loop, we don't need this trace_event anymore */
    trace_iterator->free_me = 1;
  }

  return trace_iterator;
}

void tmgr_finalize()
{
  for (auto kv : trace_list) {
    xbt_free((char*)kv.first);
    delete kv.second;
  }
}

void tmgr_trace_event_unref(tmgr_trace_event_t* trace_event)
{
  if ((*trace_event)->free_me) {
    xbt_free(*trace_event);
    *trace_event = nullptr;
  }
}
