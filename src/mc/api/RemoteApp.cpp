/* Copyright (c) 2015-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/api/RemoteApp.hpp"
#include "simgrid/forward.h"
#include "src/mc/api/ActorState.hpp"
#include "src/mc/api/states/State.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/remote/CheckerSide.hpp"
#include "src/mc/remote/mc_protocol.h"
#include "xbt/asserts.h"
#include "xbt/backtrace.hpp"
#include "xbt/log.h"
#include "xbt/system_error.hpp"
#include "xbt/thread.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits.h>
#include <limits>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <signal.h>
#include <string>
#include <sys/ptrace.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <vector>

#ifdef __linux__
#include <sys/personality.h>
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_session, mc, "Model-checker session");
XBT_LOG_EXTERNAL_CATEGORY(mc_global);

namespace simgrid::mc {
std::vector<std::string> master_socket_names;
std::mutex mtx_master_socket_names;

RemoteApp::RemoteApp(const std::vector<char*>& args, const std::string additionnal_name) : app_args_(args)
{
  master_socket_ = socket(AF_UNIX, SOCK_STREAM, 0);
  xbt_assert(master_socket_ != -1, "Cannot create the master socket: %s", strerror(errno));

  master_socket_name = "/tmp/simgrid-mc-" + std::to_string(getpid()) + '-' + additionnal_name;
  xbt_assert(master_socket_name.length() < MC_SOCKET_NAME_LEN,
             "The socket name is too long for the affected size, probably because of the tid. Fix Me");

  master_socket_name.resize(MC_SOCKET_NAME_LEN); // truncate socket name if it's too long
  master_socket_name.back() = '\0';              // ensure the data are null-terminated
#ifdef __linux__
  master_socket_name[0] = '\0'; // abstract socket, automatically removed after close
#else
  mtx_master_socket_names.lock();
  unlink(master_socket_name.c_str()); // remove possible stale socket before bind
  if (master_socket_names.empty()) {
    atexit([]() {
      for (auto& master_socket_name : master_socket_names) {
        if (not master_socket_name.empty())
          unlink(master_socket_name.c_str());
        master_socket_name.clear();
      }
    });
  }
  master_socket_names.push_back(master_socket_name);
  mtx_master_socket_names.unlock();
#endif

  struct sockaddr_un serv_addr = {};
  serv_addr.sun_family         = AF_UNIX;
  master_socket_name.copy(serv_addr.sun_path, MC_SOCKET_NAME_LEN);

  xbt_assert(bind(master_socket_, (struct sockaddr*)&serv_addr, sizeof serv_addr) >= 0,
             "Cannot bind the master socket to %c%s: %s.", (serv_addr.sun_path[0] ? serv_addr.sun_path[0] : '@'),
             serv_addr.sun_path + 1, strerror(errno));

  xbt_assert(listen(master_socket_, SOMAXCONN) >= 0, "Cannot listen to the master socket: %s.", strerror(errno));

#ifdef __linux__
  // Reduce the randomness under Linux
  personality(ADDR_NO_RANDOMIZE);
#endif

  if (_sg_mc_nofork) {
    checker_side_ = std::make_unique<simgrid::mc::CheckerSide>(app_args_);
  } else {
    application_factory_ = std::make_unique<simgrid::mc::CheckerSide>(app_args_);
    checker_side_        = application_factory_->clone(master_socket_, master_socket_name);
  }
}

void RemoteApp::restore_checker_side(CheckerSide* from, bool finalize_app)
{
  XBT_DEBUG("Restore the checker side");

  if (checker_side_ and finalize_app)
    checker_side_->finalize(true);
  if (_sg_mc_nofork) {
    checker_side_ = std::make_unique<simgrid::mc::CheckerSide>(app_args_);
  } else if (from == nullptr) {
    checker_side_ = application_factory_->clone(master_socket_, master_socket_name);
  } else {
    checker_side_ = from->clone(master_socket_, master_socket_name);
  }
}
std::unique_ptr<CheckerSide> RemoteApp::clone_checker_side()
{
  xbt_assert(not _sg_mc_nofork, "Cannot clone the checker_side in nofork mode");
  XBT_DEBUG("Clone the checker side, saving an intermediate state");
  return checker_side_->clone(master_socket_, master_socket_name);
}

unsigned long RemoteApp::get_maxpid() const
{
  // note: we could maybe cache it and count the actor creation on checker side too.
  // But counting correctly accross state checkpoint/restore would be annoying.

  checker_side_->get_channel().send(MessageType::ACTORS_MAXPID);

  auto* answer = (s_mc_message_int_t*)checker_side_->get_channel().expect_message(
      sizeof(s_mc_message_int_t), MessageType::ACTORS_MAXPID_REPLY, "Could not receive message");

  return answer->value;
}

void RemoteApp::get_actors_status(std::vector<std::optional<ActorState>>& whereto) const
{
  // The messaging happens as follows:
  //
  // CheckerSide                  AppSide
  // send ACTORS_STATUS ---->
  //                    <----- send ACTORS_STATUS_REPLY_COUNT
  //                    <----- send `N` ACTORS_STATUS_REPLY_TRANSITION (s_mc_message_actors_status_one_t)
  //                    <----- send `M` ACTORS_STATUS_REPLY_SIMCALL (s_mc_message_simcall_probe_one_t)
  //
  // Note that we also receive disabled transitions, because the guiding strategies need them to decide what could
  // unlock actors.

  if (not checker_side_->get_one_way()) {
    s_mc_message_actors_status_t msg = {MessageType::ACTORS_STATUS, Exploration::need_actor_status_transitions()};
    checker_side_->get_channel().send(msg);
  }

  checker_side_->peek_assertion_failure();

  auto* answer = (s_mc_message_actors_status_answer_t*)checker_side_->get_channel().expect_message(
      sizeof(s_mc_message_actors_status_answer_t), MessageType::ACTORS_STATUS_REPLY_COUNT, "Could not receive message");
  const int actor_count = answer->count; // The value is not stable in the buffer because if the buffer gets empty,
                                         // buffer_in_next_ is set to 0 to avoid memcpy() when reaching the buffer end

  // Message sanity checks
  xbt_assert(actor_count >= 0, "Received an ACTORS_STATUS_REPLY_COUNT message with an actor count of '%d' < 0",
             actor_count);
  xbt_assert(actor_count < std::numeric_limits<short>::max(),
             "The applications has more than %d actors, but the model-checker saves aid_t on an "
             "short int to save memory. Such app is probably too big for MC anyway.",
             std::numeric_limits<short>::max());

  if (actor_count > 0) {
    size_t size           = actor_count * sizeof(s_mc_message_actors_status_one_t);
    auto [more_data, got] = checker_side_->get_channel().receive(size);
    xbt_assert(more_data);
    // Copy data away, just in case the buffer decides to move data
    auto* status = (s_mc_message_actors_status_one_t*)alloca(actor_count * sizeof(s_mc_message_actors_status_one_t));
    memcpy(status, got, actor_count * sizeof(s_mc_message_actors_status_one_t));

    for (int i = 0; i < actor_count; i++) {
      const auto& actor = status[i];

      if ((unsigned)actor.aid >= whereto.size())
        whereto.resize(actor.aid + 1);

      std::vector<std::shared_ptr<Transition>> actor_transitions;

      if (Exploration::need_actor_status_transitions()) {
        int n_transitions = actor.max_considered;
        for (int times_considered = 0; times_considered < n_transitions; times_considered++) {
          actor_transitions.emplace_back(
              deserialize_transition(actor.aid, times_considered, checker_side_->get_channel()));
        }
        XBT_DEBUG("Received %zu transitions for actor %ld. The first one is %s", actor_transitions.size(), actor.aid,
                  (actor_transitions.size() > 0 ? actor_transitions[0]->to_string().c_str() : "null"));
      } else {
        XBT_DEBUG("No need for the actor transitions today");
      }

      whereto[actor.aid].emplace(actor.aid, actor.enabled, actor.max_considered, std::move(actor_transitions));
    }
  }
  XBT_DEBUG("Done receiving ACTORS_STATUS_REPLY");
  for (auto state : whereto) {
    if (state.has_value())
      XBT_DEBUG("Actor %hd is %s, %s/%s/%s considered %u/%u with %lu transitions", state.value().get_aid(),
                state.value().is_enabled() ? "enabled" : "disabled", state.value().is_todo() ? "todo" : "-",
                state.value().is_done() ? "done" : "-", state.value().is_unknown() ? "unknown" : "-",
                state.value().get_times_considered(), state.value().get_max_considered(),
                state.value().get_enabled_transitions().size());
  }
}

bool RemoteApp::check_deadlock(bool verbose) const
{

  auto* explo = Exploration::get_instance();
  // While looking for critical transition, we don't want to dilute the output with
  // all the deadlock informations
  if (explo->is_critical_transition_explorer())
    verbose = false;

  s_mc_message_int_t request = {};
  request.type               = MessageType::DEADLOCK_CHECK;
  request.value              = verbose;
  xbt_assert(checker_side_->get_channel().send(request) == 0, "Could not check deadlock state");

  auto answer = (s_mc_message_int_t*)checker_side_->get_channel().expect_message(
      sizeof(s_mc_message_int_t), MessageType::DEADLOCK_CHECK_REPLY, "Could not receive message");

  if (answer->value != 0) {
    if (verbose) {
      XBT_CINFO(mc_global, "Counter-example execution trace:");
      for (auto const& frame : explo->get_textual_trace())
        XBT_CINFO(mc_global, "  %s", frame.c_str());
      XBT_INFO("You can debug the problem (and see the whole details) by rerunning out of simgrid-mc with "
               "--cfg=model-check/replay:'%s'",
               explo->get_record_trace().to_string().c_str());
      explo->log_state();
    }
    return true;
  }
  return false;
}

void RemoteApp::wait_for_requests()
{
  checker_side_->wait_for_requests();
}

Transition* RemoteApp::handle_simcall(aid_t aid, int times_considered, bool new_transition) const
{
  XBT_DEBUG("Handle simcall of pid %d (time considered: %d; %s)", (int)aid, times_considered,
            (new_transition ? "newly considered -- not replay" : "replay"));

  return checker_side_->handle_simcall(aid, times_considered, new_transition);
}

void RemoteApp::replay_sequence(std::deque<std::pair<aid_t, int>> to_replay,
                                std::deque<std::pair<aid_t, int>> to_replay_and_actor_status)
{
  checker_side_->handle_replay(to_replay, to_replay_and_actor_status);
}

void RemoteApp::finalize_app(bool terminate_asap)
{
  XBT_DEBUG("Finalize the application");
  checker_side_->finalize(terminate_asap);
}

} // namespace simgrid::mc
