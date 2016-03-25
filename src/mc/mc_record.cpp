/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <stdexcept>
#include <sstream>
#include <string>

#include <xbt/fifo.h>
#include <xbt/log.h>
#include <xbt/sysdep.h>

#include "simgrid/simix.h"

#include "src/simix/smx_private.h"
#include "src/simix/smx_process_private.h"

#include "src/mc/mc_replay.h"
#include "src/mc/mc_record.h"
#include "src/mc/mc_base.h"

#if HAVE_MC
#include "src/mc/mc_request.h"
#include "src/mc/mc_private.h"
#include "src/mc/mc_state.h"
#include "src/mc/mc_smx.h"
#include "src/mc/LivenessChecker.hpp"
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_record, mc,
  " Logging specific to MC record/replay facility");

extern "C" {
char* MC_record_path = nullptr;
}

namespace simgrid {
namespace mc {

void replay(RecordTrace const& trace)
{
  simgrid::mc::wait_for_requests();

  for (auto& item : trace) {
    XBT_DEBUG("Executing %i$%i", item.pid, item.value);

    // Choose a request:
    smx_process_t process = SIMIX_process_from_PID(item.pid);
    if (!process)
      xbt_die("Unexpected process.");
    smx_simcall_t simcall = &(process->simcall);
    if(!simcall || simcall->call == SIMCALL_NONE)
      xbt_die("No simcall for this process.");
    if (!simgrid::mc::request_is_visible(simcall)
        || !simgrid::mc::request_is_enabled(simcall))
      xbt_die("Unexpected simcall.");

    // Execute the request:
    SIMIX_simcall_handle(simcall, item.value);
    simgrid::mc::wait_for_requests();
  }
}

void replay(const char* path_string)
{
  simgrid::mc::processes_time.resize(simix_process_maxpid);
  simgrid::mc::RecordTrace trace = simgrid::mc::parseRecordTrace(path_string);
  simgrid::mc::replay(trace);
  simgrid::mc::processes_time.clear();
}

RecordTrace parseRecordTrace(const char* data)
{
  RecordTrace res;
  XBT_INFO("path=%s", data);
  if (!data || !data[0])
    throw std::runtime_error("Could not parse record path");

  const char* current = data;
  while (*current) {

    simgrid::mc::RecordTraceElement item;
    int count = sscanf(current, "%u/%u", &item.pid, &item.value);
    if(count != 2 && count != 1)
      throw std::runtime_error("Could not parse record path");
    res.push_back(item);

    // Find next chunk:
    const char* end = std::strchr(current, ';');
    if(end == nullptr)
      break;
    else
      current = end + 1;
  }

  return std::move(res);
}

#if HAVE_MC

std::string traceToString(simgrid::mc::RecordTrace const& trace)
{
  std::ostringstream stream;
  for (auto i = trace.begin(); i != trace.end(); ++i) {
    if (i != trace.begin())
      stream << ';';
    stream << i->pid;
    if (i->value)
      stream << '/' << i->value;
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
