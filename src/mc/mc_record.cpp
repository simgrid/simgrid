/* Copyright (c) 2014-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/mc_record.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/context/Context.hpp"
#include "src/mc/Transition.hpp"
#include "src/mc/mc_base.h"
#include "src/mc/mc_replay.hpp"

#if SIMGRID_HAVE_MC
#include "src/mc/checker/Checker.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/mc_request.hpp"
#include "src/mc/mc_smx.hpp"
#include "src/mc/mc_state.hpp"
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_record, mc, "Logging specific to MC record/replay facility");

namespace simgrid {
namespace mc {

void replay(RecordTrace const& trace)
{
  simgrid::mc::wait_for_requests();

  for (simgrid::mc::Transition const& transition : trace) {
    XBT_DEBUG("Executing %i$%i", transition.pid_, transition.argument_);

    // Choose a request:
    kernel::actor::ActorImpl* actor = kernel::actor::ActorImpl::by_PID(transition.pid_);
    if (actor == nullptr)
      xbt_die("Unexpected actor (id:%d).", transition.pid_);
    const s_smx_simcall* simcall = &(actor->simcall_);
    if (simcall->call_ == simix::Simcall::NONE)
      xbt_die("No simcall for process %d.", transition.pid_);
    if (not simgrid::mc::request_is_visible(simcall) || not simgrid::mc::actor_is_enabled(actor))
      xbt_die("Unexpected simcall.");

    // Execute the request:
    simcall->issuer_->simcall_handle(transition.argument_);
    simgrid::mc::wait_for_requests();
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
    simgrid::mc::Transition item;
    int count = sscanf(current, "%d/%d", &item.pid_, &item.argument_);

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
    stream << i->pid_;
    if (i->argument_)
      stream << '/' << i->argument_;
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
