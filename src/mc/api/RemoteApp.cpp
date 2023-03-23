/* Copyright (c) 2015-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/api/RemoteApp.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/mc_config.hpp"
#include "xbt/asserts.h"
#include "src/mc/api/State.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/mc/mc_private.hpp"
#include "xbt/log.h"
#include "xbt/system_error.hpp"
#include <signal.h>

#include <algorithm>
#include <array>
#include <memory>
#include <numeric>
#include <string>

#include <sys/ptrace.h>
#include <sys/wait.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_Session, mc, "Model-checker session");
XBT_LOG_EXTERNAL_CATEGORY(mc_global);

namespace simgrid::mc {

RemoteApp::RemoteApp(const std::vector<char*>& args, bool need_memory_introspection) : app_args_(args)
{
  checker_side_ = std::make_unique<simgrid::mc::CheckerSide>(app_args_, need_memory_introspection);

  if (need_memory_introspection)
    initial_snapshot_ = std::make_shared<simgrid::mc::Snapshot>(0, page_store_, *checker_side_->get_remote_memory());
}

RemoteApp::~RemoteApp()
{
  initial_snapshot_ = nullptr;
  checker_side_     = nullptr;
}

void RemoteApp::restore_initial_state()
{
  if (initial_snapshot_ == nullptr) { // No memory introspection
    // We need to destroy the existing CheckerSide before creating the new one, or libevent gets crazy
    checker_side_.reset(nullptr);
    checker_side_.reset(new simgrid::mc::CheckerSide(app_args_, true));
  } else
    initial_snapshot_->restore(*checker_side_->get_remote_memory());
}

unsigned long RemoteApp::get_maxpid() const
{
  // note: we could maybe cache it and count the actor creation on checker side too.
  // But counting correctly accross state checkpoint/restore would be annoying.

  checker_side_->get_channel().send(MessageType::ACTORS_MAXPID);
  s_mc_message_int_t answer;
  ssize_t answer_size = checker_side_->get_channel().receive(answer);
  xbt_assert(answer_size != -1, "Could not receive message");
  xbt_assert(answer_size == sizeof answer, "Broken message (size=%zd; expected %zu)", answer_size, sizeof answer);
  xbt_assert(answer.type == MessageType::ACTORS_MAXPID_REPLY,
             "Received unexpected message %s (%i); expected MessageType::ACTORS_MAXPID_REPLY (%i)",
             to_c_str(answer.type), (int)answer.type, (int)MessageType::ACTORS_MAXPID_REPLY);

  return answer.value;
}

void RemoteApp::get_actors_status(std::map<aid_t, ActorState>& whereto) const
{
  // The messaging happens as follows:
  //
  // CheckerSide                  AppSide
  // send ACTORS_STATUS ---->
  //                    <----- send ACTORS_STATUS_REPLY
  //                    <----- send `N` `s_mc_message_actors_status_one_t` structs
  //                    <----- send `M` `s_mc_message_simcall_probe_one_t` structs
  checker_side_->get_channel().send(MessageType::ACTORS_STATUS);

  s_mc_message_actors_status_answer_t answer;
  ssize_t answer_size = checker_side_->get_channel().receive(answer);
  xbt_assert(answer_size != -1, "Could not receive message");
  xbt_assert(answer_size == sizeof answer, "Broken message (size=%zd; expected %zu)", answer_size, sizeof answer);
  xbt_assert(answer.type == MessageType::ACTORS_STATUS_REPLY,
             "Received unexpected message %s (%i); expected MessageType::ACTORS_STATUS_REPLY (%i)",
             to_c_str(answer.type), (int)answer.type, (int)MessageType::ACTORS_STATUS_REPLY);

  // Message sanity checks
  xbt_assert(answer.count >= 0, "Received an ACTOR_STATUS_REPLY message with an actor count of '%d' < 0", answer.count);

  std::vector<s_mc_message_actors_status_one_t> status(answer.count);
  if (answer.count > 0) {
    size_t size      = status.size() * sizeof(s_mc_message_actors_status_one_t);
    ssize_t received = checker_side_->get_channel().receive(status.data(), size);
    xbt_assert(static_cast<size_t>(received) == size);
  }

  whereto.clear();

  for (const auto& actor : status) {
    std::vector<std::shared_ptr<Transition>> actor_transitions;
    int n_transitions = actor.enabled ? actor.max_considered : 0;
    for (int times_considered = 0; times_considered < n_transitions; times_considered++) {
      s_mc_message_simcall_probe_one_t probe;
      ssize_t received = checker_side_->get_channel().receive(probe);
      xbt_assert(received >= 0, "Could not receive response to ACTORS_PROBE message (%s)", strerror(errno));
      xbt_assert(static_cast<size_t>(received) == sizeof probe,
                 "Could not receive response to ACTORS_PROBE message (%zd bytes received != %zu bytes expected",
                 received, sizeof probe);

      std::stringstream stream(probe.buffer.data());
      actor_transitions.emplace_back(deserialize_transition(actor.aid, times_considered, stream));
    }

    XBT_DEBUG("Received %zu transitions for actor %ld", actor_transitions.size(), actor.aid);
    whereto.try_emplace(actor.aid, actor.aid, actor.enabled, actor.max_considered, std::move(actor_transitions));
  }
}

void RemoteApp::check_deadlock() const
{
  xbt_assert(checker_side_->get_channel().send(MessageType::DEADLOCK_CHECK) == 0, "Could not check deadlock state");
  s_mc_message_int_t message;
  ssize_t received = checker_side_->get_channel().receive(message);
  xbt_assert(received != -1, "Could not receive message");
  xbt_assert(received == sizeof message, "Broken message (size=%zd; expected %zu)", received, sizeof message);
  xbt_assert(message.type == MessageType::DEADLOCK_CHECK_REPLY,
             "Received unexpected message %s (%i); expected MessageType::DEADLOCK_CHECK_REPLY (%i)",
             to_c_str(message.type), (int)message.type, (int)MessageType::DEADLOCK_CHECK_REPLY);

  if (message.value != 0) {
    auto* explo = Exploration::get_instance();
    XBT_CINFO(mc_global, "Counter-example execution trace:");
    for (auto const& frame : explo->get_textual_trace())
      XBT_CINFO(mc_global, "  %s", frame.c_str());
    XBT_INFO("You can debug the problem (and see the whole details) by rerunning out of simgrid-mc with "
             "--cfg=model-check/replay:'%s'",
             explo->get_record_trace().to_string().c_str());
    explo->log_state();
    throw DeadlockError();
  }
}

void RemoteApp::wait_for_requests()
{
  checker_side_->wait_for_requests();
}

Transition* RemoteApp::handle_simcall(aid_t aid, int times_considered, bool new_transition)
{
  s_mc_message_simcall_execute_t m = {};
  m.type                           = MessageType::SIMCALL_EXECUTE;
  m.aid_                           = aid;
  m.times_considered_              = times_considered;
  checker_side_->get_channel().send(m);

  if (auto* memory = get_remote_process_memory(); memory != nullptr)
    memory->clear_cache();
  if (checker_side_->running())
    checker_side_->dispatch_events(); // The app may send messages while processing the transition

  s_mc_message_simcall_execute_answer_t answer;
  ssize_t s = checker_side_->get_channel().receive(answer);
  xbt_assert(s != -1, "Could not receive message");
  xbt_assert(s == sizeof answer, "Broken message (size=%zd; expected %zu)", s, sizeof answer);
  xbt_assert(answer.type == MessageType::SIMCALL_EXECUTE_ANSWER,
             "Received unexpected message %s (%i); expected MessageType::SIMCALL_EXECUTE_ANSWER (%i)",
             to_c_str(answer.type), (int)answer.type, (int)MessageType::SIMCALL_EXECUTE_ANSWER);

  if (new_transition) {
    std::stringstream stream(answer.buffer.data());
    return deserialize_transition(aid, times_considered, stream);
  } else
    return nullptr;
}

void RemoteApp::finalize_app(bool terminate_asap)
{
  checker_side_->finalize(terminate_asap);
}

} // namespace simgrid::mc
