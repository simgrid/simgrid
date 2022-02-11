/* Copyright (c) 2014-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/mc_record.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/context/Context.hpp"
#include "src/mc/api/Transition.hpp"
#include "src/mc/mc_base.hpp"
#include "src/mc/mc_replay.hpp"

#if SIMGRID_HAVE_MC
#include "src/mc/api/State.hpp"
#include "src/mc/checker/Checker.hpp"
#include "src/mc/mc_private.hpp"
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_record, mc, "Logging specific to MC record/replay facility");

namespace simgrid {
namespace mc {

void replay(RecordTrace const& trace)
{
  simgrid::mc::execute_actors();

  for (simgrid::mc::Transition const& transition : trace) {
    XBT_DEBUG("Executing %ld$%i", transition.aid_, transition.times_considered_);

    // Choose a request:
    kernel::actor::ActorImpl* actor = kernel::actor::ActorImpl::by_pid(transition.aid_);
    xbt_assert(actor != nullptr, "Unexpected actor (id:%ld).", transition.aid_);
    const s_smx_simcall* simcall = &(actor->simcall_);
    xbt_assert(simcall->call_ != simix::Simcall::NONE, "No simcall for process %ld.", transition.aid_);
    xbt_assert(simgrid::mc::request_is_visible(simcall) && simgrid::mc::actor_is_enabled(actor), "Unexpected simcall.");

    // Execute the request:
    simcall->issuer_->simcall_handle(transition.times_considered_);
    simgrid::mc::execute_actors();
  }
}

void replay(const std::string& path_string)
{
  simgrid::mc::processes_time.resize(simgrid::kernel::actor::get_maxpid());
  simgrid::mc::RecordTrace trace = simgrid::mc::parseRecordTrace(path_string.c_str());
  simgrid::mc::replay(trace);
  simgrid::mc::processes_time.clear();
}

RecordTrace parseRecordTrace(const char* data)
{
  RecordTrace res;
  XBT_INFO("path=%s", data);
  if (data == nullptr || data[0] == '\0')
    throw std::invalid_argument("Could not parse record path");

  const char* current = data;
  while (*current) {
    long aid;
    int times_considered;
    int count = sscanf(current, "%ld/%d", &aid, &times_considered);
    simgrid::mc::Transition item(aid, times_considered);

    if(count != 2 && count != 1)
      throw std::invalid_argument("Could not parse record path");
    res.push_back(item);

    // Find next chunk:
    const char* end = std::strchr(current, ';');
    if(end == nullptr)
      break;
    else
      current = end + 1;
  }

  return res;
}

#if SIMGRID_HAVE_MC

std::string traceToString(simgrid::mc::RecordTrace const& trace)
{
  std::ostringstream stream;
  for (auto i = trace.begin(); i != trace.end(); ++i) {
    if (i != trace.begin())
      stream << ';';
    stream << i->aid_;
    if (i->times_considered_ > 0)
      stream << '/' << i->times_considered_;
  }
  return stream.str();
}

void dumpRecordPath()
{
  RecordTrace trace = mc_model_checker->getChecker()->get_record_trace();
  XBT_INFO("Path = %s", traceToString(trace).c_str());
}

#endif

}
}
