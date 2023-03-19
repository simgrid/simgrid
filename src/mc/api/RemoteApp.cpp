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
#include <boost/tokenizer.hpp>
#include <memory>
#include <numeric>
#include <string>

#include <fcntl.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
#include <sys/ptrace.h>
#include <sys/wait.h>

#ifdef __linux__
#define WAITPID_CHECKED_FLAGS __WALL
#else
#define WAITPID_CHECKED_FLAGS 0
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_Session, mc, "Model-checker session");
XBT_LOG_EXTERNAL_CATEGORY(mc_global);

static simgrid::config::Flag<std::string> _sg_mc_setenv{
    "model-check/setenv", "Extra environment variables to pass to the child process (ex: 'AZE=aze;QWE=qwe').", "",
    [](std::string_view value) {
      xbt_assert(value.empty() || value.find('=', 0) != std::string_view::npos,
                 "The 'model-check/setenv' parameter must be like 'AZE=aze', but it does not contain an equal sign.");
    }};

namespace simgrid::mc {

XBT_ATTRIB_NORETURN static void run_child_process(int socket, const std::vector<char*>& args)
{
  /* On startup, simix_global_init() calls simgrid::mc::Client::initialize(), which checks whether the MC_ENV_SOCKET_FD
   * env variable is set. If so, MC mode is assumed, and the client is setup from its side
   */

#ifdef __linux__
  // Make sure we do not outlive our parent
  sigset_t mask;
  sigemptyset(&mask);
  xbt_assert(sigprocmask(SIG_SETMASK, &mask, nullptr) >= 0, "Could not unblock signals");
  xbt_assert(prctl(PR_SET_PDEATHSIG, SIGHUP) == 0, "Could not PR_SET_PDEATHSIG");
#endif

  // Remove CLOEXEC to pass the socket to the application
  int fdflags = fcntl(socket, F_GETFD, 0);
  xbt_assert(fdflags != -1 && fcntl(socket, F_SETFD, fdflags & ~FD_CLOEXEC) != -1,
             "Could not remove CLOEXEC for socket");

  setenv(MC_ENV_SOCKET_FD, std::to_string(socket).c_str(), 1);

  /* Setup the tokenizer that parses the cfg:model-check/setenv parameter */
  using Tokenizer = boost::tokenizer<boost::char_separator<char>>;
  boost::char_separator<char> semicol_sep(";");
  boost::char_separator<char> equal_sep("=");
  Tokenizer token_vars(_sg_mc_setenv.get(), semicol_sep); /* Iterate over all FOO=foo parts */
  for (const auto& token : token_vars) {
    std::vector<std::string> kv;
    Tokenizer token_kv(token, equal_sep);
    for (const auto& t : token_kv) /* Iterate over 'FOO' and then 'foo' in that 'FOO=foo' */
      kv.push_back(t);
    xbt_assert(kv.size() == 2, "Parse error on 'model-check/setenv' value %s. Does it contain an equal sign?",
               token.c_str());
    XBT_INFO("setenv '%s'='%s'", kv[0].c_str(), kv[1].c_str());
    setenv(kv[0].c_str(), kv[1].c_str(), 1);
  }

  /* And now, exec the child process */
  int i = 1;
  while (args[i] != nullptr && args[i][0] == '-')
    i++;

  xbt_assert(args[i] != nullptr,
             "Unable to find a binary to exec on the command line. Did you only pass config flags?");

  execvp(args[i], args.data() + i);
  XBT_CRITICAL("The model-checked process failed to exec(%s): %s.\n"
               "        Make sure that your binary exists on disk and is executable.",
               args[i], strerror(errno));
  if (strchr(args[i], '=') != nullptr)
    XBT_CRITICAL("If you want to pass environment variables to the application, please use --cfg=model-check/setenv:%s",
                 args[i]);

  xbt_die("Aborting now.");
}

RemoteApp::RemoteApp(const std::vector<char*>& args)
{
  // Create an AF_LOCAL socketpair used for exchanging messages
  // between the model-checker process (ourselves) and the model-checked
  // process:
  int sockets[2];
  xbt_assert(socketpair(AF_LOCAL, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, sockets) != -1, "Could not create socketpair");

  pid_t pid = fork();
  xbt_assert(pid >= 0, "Could not fork model-checked process");

  if (pid == 0) { // Child
    ::close(sockets[1]);
    run_child_process(sockets[0], args);
    DIE_IMPOSSIBLE;
  }

  // Parent (model-checker):
  ::close(sockets[0]);

  checker_side_ = std::make_unique<simgrid::mc::CheckerSide>(sockets[1], pid);

  start();

  /* Take the initial snapshot */
  wait_for_requests();
  initial_snapshot_ = std::make_shared<simgrid::mc::Snapshot>(0, page_store_, checker_side_->get_remote_memory());
}

RemoteApp::~RemoteApp()
{
  initial_snapshot_ = nullptr;
  shutdown();
}
void RemoteApp::start()
{
  XBT_DEBUG("Waiting for the model-checked process");
  int status;

  // The model-checked process SIGSTOP itself to signal it's ready:
  const pid_t pid = checker_side_->get_pid();

  xbt_assert(waitpid(pid, &status, WAITPID_CHECKED_FLAGS) == pid && WIFSTOPPED(status) && WSTOPSIG(status) == SIGSTOP,
             "Could not wait model-checked process");

  errno = 0;
#ifdef __linux__
  ptrace(PTRACE_SETOPTIONS, pid, nullptr, PTRACE_O_TRACEEXIT);
  ptrace(PTRACE_CONT, pid, 0, 0);
#elif defined BSD
  ptrace(PT_CONTINUE, pid, (caddr_t)1, 0);
#else
#error "no ptrace equivalent coded for this platform"
#endif
  xbt_assert(errno == 0,
             "Ptrace does not seem to be usable in your setup (errno: %d). "
             "If you run from within a docker, adding `--cap-add SYS_PTRACE` to the docker line may help. "
             "If it does not help, please report this bug.",
             errno);
}
void RemoteApp::restore_initial_state() const
{
  this->initial_snapshot_->restore(checker_side_->get_remote_memory());
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
  xbt_assert(answer.transition_count >= 0, "Received an ACTOR_STATUS_REPLY message with transition_count '%d' < 0",
             answer.transition_count);
  xbt_assert(answer.transition_count == 0 || answer.count >= 0,
             "Received an ACTOR_STATUS_REPLY message with no actor data "
             "but with transition data nonetheless");

  std::vector<s_mc_message_actors_status_one_t> status(answer.count);
  if (answer.count > 0) {
    size_t size      = status.size() * sizeof(s_mc_message_actors_status_one_t);
    ssize_t received = checker_side_->get_channel().receive(status.data(), size);
    xbt_assert(static_cast<size_t>(received) == size);
  }

  // Ensures that each actor sends precisely `answer.transition_count` transitions. While technically
  // this doesn't catch the edge case where actor A sends 3 instead of 2 and actor B sends 2 instead
  // of 3 transitions, that is ignored here since that invariant needs to be enforced on the AppSide
  const auto expected_transitions = std::accumulate(
      status.begin(), status.end(), 0, [](int total, const auto& actor) { return total + actor.n_transitions; });
  xbt_assert(expected_transitions == answer.transition_count,
             "Expected to receive %d transition(s) but was only notified of %d by the app side", expected_transitions,
             answer.transition_count);

  std::vector<s_mc_message_simcall_probe_one_t> probes(answer.transition_count);
  if (answer.transition_count > 0) {
    for (auto& probe : probes) {
      ssize_t received = checker_side_->get_channel().receive(probe);
      xbt_assert(received >= 0, "Could not receive response to ACTORS_PROBE message (%s)", strerror(errno));
      xbt_assert(static_cast<size_t>(received) == sizeof probe,
                 "Could not receive response to ACTORS_PROBE message (%zd bytes received != %zu bytes expected",
                 received, sizeof probe);
    }
  }

  whereto.clear();
  std::move_iterator probes_iter(probes.begin());

  for (const auto& actor : status) {
    xbt_assert(actor.n_transitions == 0 || actor.n_transitions == actor.max_considered,
               "If any transitions are serialized for an actor, it must match the "
               "total number of transitions that can be considered for the actor "
               "(currently %d), but only %d transition(s) was/were said to be encoded",
               actor.max_considered, actor.n_transitions);

    std::vector<std::shared_ptr<Transition>> actor_transitions;
    for (int times_considered = 0; times_considered < actor.n_transitions; times_considered++, probes_iter++) {
      std::stringstream stream((*probes_iter).buffer.data());
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
  /* Resume the application */
  if (checker_side_->get_channel().send(MessageType::CONTINUE) != 0)
    throw xbt::errno_error();
  checker_side_->clear_memory_cache();

  if (checker_side_->running())
    checker_side_->dispatch_events();
}

void RemoteApp::shutdown()
{
  XBT_DEBUG("Shutting down model-checker");

  if (checker_side_->running()) {
    XBT_DEBUG("Killing process");
    finalize_app(true);
    kill(checker_side_->get_pid(), SIGKILL);
    checker_side_->terminate();
  }
}

Transition* RemoteApp::handle_simcall(aid_t aid, int times_considered, bool new_transition)
{
  s_mc_message_simcall_execute_t m = {};
  m.type                           = MessageType::SIMCALL_EXECUTE;
  m.aid_                           = aid;
  m.times_considered_              = times_considered;
  checker_side_->get_channel().send(m);

  get_remote_process_memory().clear_cache();
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
  s_mc_message_int_t m = {};
  m.type               = MessageType::FINALIZE;
  m.value              = terminate_asap;
  xbt_assert(checker_side_->get_channel().send(m) == 0, "Could not ask the app to finalize on need");

  s_mc_message_t answer;
  ssize_t s = checker_side_->get_channel().receive(answer);
  xbt_assert(s != -1, "Could not receive answer to FINALIZE");
  xbt_assert(s == sizeof answer, "Broken message (size=%zd; expected %zu)", s, sizeof answer);
  xbt_assert(answer.type == MessageType::FINALIZE_REPLY,
             "Received unexpected message %s (%i); expected MessageType::FINALIZE_REPLY (%i)", to_c_str(answer.type),
             (int)answer.type, (int)MessageType::FINALIZE_REPLY);
}

} // namespace simgrid::mc
