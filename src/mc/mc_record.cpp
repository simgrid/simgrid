/* Copyright (c) 2014-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <stdexcept>
#include <sstream>
#include <string>

#include "xbt/log.h"
#include "xbt/sysdep.h"

#include "simgrid/simix.h"

#include "src/kernel/context/Context.hpp"
#include "src/mc/mc_record.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/simix/ActorImpl.hpp"
#include "src/simix/smx_private.hpp"

#include "src/mc/mc_base.h"
#include "src/mc/Transition.hpp"

#if SIMGRID_HAVE_MC
#include "src/mc/checker/Checker.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/mc_request.hpp"
#include "src/mc/mc_smx.hpp"
#include "src/mc/mc_state.hpp"
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_record, mc,
  " Logging specific to MC record/replay facility");

std::string MC_record_path;

namespace simgrid {
namespace mc {

void replay(RecordTrace const& trace)
{
  simgrid::mc::wait_for_requests();

  for (simgrid::mc::Transition const& transition : trace) {
    XBT_DEBUG("Executing %i$%i", transition.pid, transition.argument);

    // Choose a request:
    smx_actor_t process = SIMIX_process_from_PID(transition.pid);
    if (not process)
      xbt_die("Unexpected process (pid:%d).", transition.pid);
    smx_simcall_t simcall = &(process->simcall);
    if (not simcall || simcall->call == SIMCALL_NONE)
      xbt_die("No simcall for process %d.", transition.pid);
    if (not simgrid::mc::request_is_visible(simcall) || not simgrid::mc::actor_is_enabled(process))
      xbt_die("Unexpected simcall.");

    // Execute the request:
    SIMIX_simcall_handle(simcall, transition.argument);
    simgrid::mc::wait_for_requests();
  }
}

void replay(std::string path_string)
{
  simgrid::mc::processes_time.resize(SIMIX_process_get_maxpid());
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
    int count = sscanf(current, "%d/%d", &item.pid, &item.argument);
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
    stream << i->pid;
    if (i->argument)
      stream << '/' << i->argument;
  }
  return stream.str();
}

void dumpRecordPath()
{
  if (MC_record_is_active()) {
    RecordTrace trace = mc_model_checker->getChecker()->getRecordTrace();
    XBT_INFO("Path = %s", traceToString(trace).c_str());
  }
}

#endif

}
}
