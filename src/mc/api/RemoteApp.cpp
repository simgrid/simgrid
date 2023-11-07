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
#include <limits.h>
#include <memory>
#include <numeric>
#include <string>
#include <sys/ptrace.h>
#include <sys/un.h>
#include <sys/wait.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_Session, mc, "Model-checker session");
XBT_LOG_EXTERNAL_CATEGORY(mc_global);

namespace simgrid::mc {

static std::string master_socket_name;

RemoteApp::RemoteApp(const std::vector<char*>& args) : app_args_(args)
{
  master_socket_ = socket(AF_UNIX,
#ifdef __APPLE__
                          SOCK_STREAM, /* Mac OSX does not have AF_UNIX + SOCK_SEQPACKET, even if that's faster */
#else
                          SOCK_SEQPACKET,
#endif
                          0);
    xbt_assert(master_socket_ != -1, "Cannot create the master socket: %s", strerror(errno));

    master_socket_name = "/tmp/simgrid-mc-" + std::to_string(getpid());
    master_socket_name.resize(MC_SOCKET_NAME_LEN); // truncate socket name if it's too long
    master_socket_name.back() = '\0';              // ensure the data are null-terminated
#ifdef __linux__
    master_socket_name[0] = '\0'; // abstract socket, automatically removed after close
#else
    unlink(master_socket_name.c_str()); // remove possible stale socket before bind
    atexit([]() {
      if (not master_socket_name.empty())
        unlink(master_socket_name.c_str());
      master_socket_name.clear();
    });
#endif

    struct sockaddr_un serv_addr = {};
    serv_addr.sun_family         = AF_UNIX;
    master_socket_name.copy(serv_addr.sun_path, MC_SOCKET_NAME_LEN);

    xbt_assert(bind(master_socket_, (struct sockaddr*)&serv_addr, sizeof serv_addr) >= 0,
               "Cannot bind the master socket to %c%s: %s.", (serv_addr.sun_path[0] ? serv_addr.sun_path[0] : '@'),
               serv_addr.sun_path + 1, strerror(errno));

    xbt_assert(listen(master_socket_, SOMAXCONN) >= 0, "Cannot listen to the master socket: %s.", strerror(errno));

    application_factory_ = std::make_unique<simgrid::mc::CheckerSide>(app_args_);
    checker_side_        = application_factory_->clone(master_socket_, master_socket_name);
}

void RemoteApp::restore_initial_state()
{
    checker_side_ = application_factory_->clone(master_socket_, master_socket_name);
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
  //                    <----- send ACTORS_STATUS_REPLY_COUNT
  //                    <----- send `N` ACTORS_STATUS_REPLY_TRANSITION (s_mc_message_actors_status_one_t)
  //                    <----- send `M` ACTORS_STATUS_REPLY_SIMCALL (s_mc_message_simcall_probe_one_t)
  //
  // Note that we also receive disabled transitions, because the guiding strategies need them to decide what could
  // unlock actors.

  checker_side_->get_channel().send(MessageType::ACTORS_STATUS);

  s_mc_message_actors_status_answer_t answer;
  ssize_t answer_size = checker_side_->get_channel().receive(answer);
  xbt_assert(answer_size != -1, "Could not receive message");
  xbt_assert(answer_size == sizeof answer, "Broken message (size=%zd; expected %zu)", answer_size, sizeof answer);
  xbt_assert(answer.type == MessageType::ACTORS_STATUS_REPLY_COUNT,
             "%d Received unexpected message %s (%i); expected MessageType::ACTORS_STATUS_REPLY_COUNT (%i)", getpid(),
             to_c_str(answer.type), (int)answer.type, (int)MessageType::ACTORS_STATUS_REPLY_COUNT);

  // Message sanity checks
  xbt_assert(answer.count >= 0, "Received an ACTORS_STATUS_REPLY_COUNT message with an actor count of '%d' < 0",
             answer.count);

  std::vector<s_mc_message_actors_status_one_t> status(answer.count);
  if (answer.count > 0) {
    size_t size      = status.size() * sizeof(s_mc_message_actors_status_one_t);
    ssize_t received = checker_side_->get_channel().receive(status.data(), size);
    xbt_assert(static_cast<size_t>(received) == size);
  }

  whereto.clear();

  for (const auto& actor : status) {
    std::vector<std::shared_ptr<Transition>> actor_transitions;
    int n_transitions = actor.max_considered;
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

    XBT_DEBUG("Received %zu transitions for actor %ld. The first one is %s", actor_transitions.size(), actor.aid,
              (actor_transitions.size() > 0 ? actor_transitions[0]->to_string().c_str() : "null"));
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
    throw McError(ExitStatus::DEADLOCK);
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

  if (checker_side_->running())
    checker_side_->dispatch_events(); // The app may send messages while processing the transition

  s_mc_message_simcall_execute_answer_t answer;
  ssize_t s = checker_side_->get_channel().receive(answer);
  xbt_assert(s != -1, "Could not receive message");
  xbt_assert(s > 0 && answer.type == MessageType::SIMCALL_EXECUTE_REPLY,
             "%d Received unexpected message %s (%i); expected MessageType::SIMCALL_EXECUTE_REPLY (%i)", getpid(),
             to_c_str(answer.type), (int)answer.type, (int)MessageType::SIMCALL_EXECUTE_REPLY);
  xbt_assert(s == sizeof answer, "Broken message (size=%zd; expected %zu)", s, sizeof answer);

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
