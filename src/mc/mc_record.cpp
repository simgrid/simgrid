/* Copyright (c) 2014-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/mc_record.hpp"
#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/mc/mc_base.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/mc/transition/Transition.hpp"

#if SIMGRID_HAVE_MC
#include "src/mc/api/State.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/mc_private.hpp"
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_record, mc, "Logging specific to MC record/replay facility");

namespace simgrid {
namespace mc {

void RecordTrace::replay() const
{
  simgrid::mc::execute_actors();

  for (const simgrid::mc::Transition* transition : transitions_) {
    XBT_DEBUG("Executing %ld$%i", transition->aid_, transition->times_considered_);

    // Choose a request:
    kernel::actor::ActorImpl* actor = kernel::EngineImpl::get_instance()->get_actor_by_pid(transition->aid_);
    xbt_assert(actor != nullptr, "Unexpected actor (id:%ld).", transition->aid_);
    const kernel::actor::Simcall* simcall = &(actor->simcall_);
    xbt_assert(simcall->call_ != kernel::actor::Simcall::Type::NONE, "No simcall for process %ld.", transition->aid_);
    xbt_assert(simgrid::mc::request_is_visible(simcall) && simgrid::mc::actor_is_enabled(actor), "Unexpected simcall.");

    // Execute the request:
    simcall->issuer_->simcall_handle(transition->times_considered_);
    simgrid::mc::execute_actors();
  }
}

void simgrid::mc::RecordTrace::replay(const std::string& path_string)
{
  simgrid::mc::processes_time.resize(kernel::actor::ActorImpl::get_maxpid());
  simgrid::mc::RecordTrace trace(path_string.c_str());
  trace.replay();
  for (auto* item : trace.transitions_)
    delete item;
  simgrid::mc::processes_time.clear();
}

simgrid::mc::RecordTrace::RecordTrace(const char* data)
{
  XBT_INFO("path=%s", data);
  if (data == nullptr || data[0] == '\0')
    throw std::invalid_argument("Could not parse record path");

  const char* current = data;
  while (*current) {
    long aid;
    int times_considered;
    int count = sscanf(current, "%ld/%d", &aid, &times_considered);

    if(count != 2 && count != 1)
      throw std::invalid_argument("Could not parse record path");
    push_back(new simgrid::mc::Transition(simgrid::mc::Transition::Type::UNKNOWN, aid, times_considered));

    // Find next chunk:
    const char* end = std::strchr(current, ';');
    if(end == nullptr)
      break;
    else
      current = end + 1;
  }
}

#if SIMGRID_HAVE_MC

std::string simgrid::mc::RecordTrace::to_string() const
{
  std::ostringstream stream;
  for (auto i = transitions_.begin(); i != transitions_.end(); ++i) {
    if (i != transitions_.begin())
      stream << ';';
    stream << (*i)->aid_;
    if ((*i)->times_considered_ > 0)
      stream << '/' << (*i)->times_considered_;
  }
  return stream.str();
}

#endif

}
}
