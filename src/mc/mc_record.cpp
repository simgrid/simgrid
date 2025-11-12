/* Copyright (c) 2014-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/mc_record.hpp"
#include "simgrid/forward.h"
#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/activity/MemoryImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/mc_base.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/mc/transition/Transition.hpp"
#include "xbt/asserts.h"
#include <memory>
#include <sys/socket.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_record, mc, "Logging specific to MC record/replay facility");

namespace simgrid::mc {

// Create a model check transition with the memory access so we can feed the Execution structure
// which executes the FastTrack algorithm in order to find data races
static std::shared_ptr<Transition> create_mc_transition(kernel::actor::ActorImpl* actor, Channel& app_side,
                                                        Channel& checker_side)
{

  actor->simcall_.observer_->serialize(app_side);
  actor->recorded_memory_accesses_->serialize(app_side);
  xbt_assert(app_side.send() == 0, "Could not send response: %s", strerror(errno));

  auto* t = deserialize_transition(actor->get_pid(), actor->get_restart_count(), checker_side);
  t->deserialize_memory_operations(checker_side);

  return std::shared_ptr<Transition>(t);
}

void RecordTrace::replay() const
{
  simgrid::mc::execute_actors();
  auto* engine = kernel::EngineImpl::get_instance();

  int frame_count = 1;
  if (xbt_log_no_loc)
    XBT_INFO("The backtrace of each transition will not be shown because of --log=no_loc");
  else
    simgrid_mc_replay_show_backtraces = true;

  // Creating two non-blocking sockets locally so we can simulate the application talking to MC
  // This way we can use everything already in place in the model checker code despite being in the App side
  int sockets[2];
  xbt_assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != -1,
             "Could not create socketpair: %s.\nPlease increase the file limit with `ulimit -n 10000`.",
             strerror(errno));
  auto app_side     = std::make_unique<Channel>(sockets[0]);
  auto checker_side = std::make_unique<Channel>(sockets[1]);

  auto exec = std::make_unique<odpor::Execution>();

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

    xbt_assert(simcall->observer_ != nullptr, "null observer, :'(");

#if SIMGRID_HAVE_MC
    auto t = create_mc_transition(actor, *app_side.get(), *checker_side.get());
    try {
      exec->push_transition(t); // Race detection is done here
    } catch (const McDataRace& e) {
      XBT_INFO("Found a datarace at location %p", e.location_);
      // Printing the epoch is not very interesting for the user
      XBT_DEBUG("Race between %ld@%ld and %ld@%ld", e.first_mem_op_.second, e.first_mem_op_.first,
                e.second_mem_op_.second, e.second_mem_op_.first);

      XBT_INFO("First operation was a WRITE made by actor %ld:\n%s", e.first_mem_op_.first,
               kernel::activity::MemoryAccessImpl::get_info_from_access(
                   e.first_mem_op_.first, e.first_mem_op_.second,
                   kernel::activity::MemoryAccess(MemOpType::WRITE, e.location_))
                   .c_str());

      XBT_INFO("Second operation was a %s made by actor %ld:\n%s",
               e.second_mem_type_ == MemOpType::READ ? "READ" : "WRITE", e.second_mem_op_.first,
               kernel::activity::MemoryAccessImpl::get_info_from_access(
                   e.second_mem_op_.first, e.second_mem_op_.second,
                   kernel::activity::MemoryAccess(e.second_mem_type_, e.location_))
                   .c_str());

      abort();
    }
#else
    if (not xbt_log_no_loc)
      XBT_INFO("(data races cannot be replayed when MC is not enabled, but that's OK if you don't have any race in "
               "your code)");
#endif

    simgrid::mc::execute_actors();
  }

  const auto& actor_list = engine->get_actor_list();
  if (actor_list.empty()) {
    XBT_INFO(
        "The replay of the trace is complete and no actor remains to be executed. The application is terminating.");
  } else if (std::none_of(std::begin(actor_list), std::end(actor_list),
                          [](const auto& kv) { return mc::actor_is_enabled(kv.second); })) {
    XBT_INFO("The replay of the trace is complete. DEADLOCK detected.");
    engine->display_all_actor_status();
  } else {
    XBT_INFO("The replay of the trace is complete. The application could run further:");
    kernel::EngineImpl::get_instance()->display_all_actor_status();
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
