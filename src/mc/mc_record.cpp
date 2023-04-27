/* Copyright (c) 2014-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/mc_record.hpp"
#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/mc/mc_base.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/mc/transition/Transition.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_record, mc, "Logging specific to MC record/replay facility");

namespace simgrid::mc {

void RecordTrace::replay() const
{
  simgrid::mc::execute_actors();
  auto* engine = kernel::EngineImpl::get_instance();

  int frame_count = 1;
  if (xbt_log_no_loc)
    XBT_INFO("The backtrace of each transition will not be shown because of --log=no_loc");
  else
    simgrid_mc_replay_show_backtraces = true;

  for (const simgrid::mc::Transition* transition : transitions_) {
    kernel::actor::ActorImpl* actor = engine->get_actor_by_pid(transition->aid_);
    xbt_assert(actor != nullptr, "Unexpected actor (id:%ld).", transition->aid_);
    const kernel::actor::Simcall* simcall = &(actor->simcall_);
    xbt_assert(simgrid::mc::request_is_visible(simcall), "Simcall %s of actor %s is not visible.", simcall->get_cname(),
               actor->get_cname());

    XBT_INFO("***********************************************************************************");
    XBT_INFO("* Path chunk #%d '%ld/%i' Actor %s(pid:%ld): %s", frame_count++, transition->aid_,
             transition->times_considered_, simcall->issuer_->get_cname(), simcall->issuer_->get_pid(),
             simcall->observer_->to_string().c_str());
    XBT_INFO("***********************************************************************************");
    if (not mc::actor_is_enabled(actor))
      simgrid::kernel::EngineImpl::get_instance()->display_all_actor_status();

    xbt_assert(simgrid::mc::actor_is_enabled(actor), "Actor %s (simcall %s) is not enabled.", actor->get_cname(),
               simcall->get_cname());

    // Execute the request:
    simcall->issuer_->simcall_handle(transition->times_considered_);
    simgrid::mc::execute_actors();
  }

  const auto& actor_list = engine->get_actor_list();
  if (actor_list.empty()) {
    XBT_INFO("The replay of the trace is complete. The application is terminating.");
  } else if (std::none_of(std::begin(actor_list), std::end(actor_list),
                          [](const auto& kv) { return mc::actor_is_enabled(kv.second); })) {
    XBT_INFO("The replay of the trace is complete. DEADLOCK detected.");
    engine->display_all_actor_status();
  } else {
    XBT_INFO("The replay of the trace is complete. The application could run further.");
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
    int times_considered = 0;

    if (int count = sscanf(current, "%ld/%d", &aid, &times_considered); count != 2 && count != 1)
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

std::string simgrid::mc::RecordTrace::to_string() const
{
  std::ostringstream stream;
  for (auto i = transitions_.begin(); i != transitions_.end(); ++i) {
    if (*i == nullptr)
      continue;
    if (i != transitions_.begin())
      stream << ';';
    stream << (*i)->aid_;
    if ((*i)->times_considered_ > 0)
      stream << '/' << (*i)->times_considered_;
  }
  return stream.str();
}
} // namespace simgrid::mc
