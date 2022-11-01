/* Copyright (c) 2015-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/api/RemoteApp.hpp"
#include "src/internal_config.h" // HAVE_SMPI
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/mc_config.hpp"
#include "xbt/asserts.h"
#if HAVE_SMPI
#include "smpi/smpi.h"
#include "src/smpi/include/private.hpp"
#endif
#include "signal.h"
#include "src/mc/api/State.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/mc/mc_private.hpp"
#include "xbt/log.h"
#include "xbt/system_error.hpp"

#include <array>
#include <boost/tokenizer.hpp>
#include <memory>
#include <string>

#include <fcntl.h>
#ifdef __linux__
#include <sys/prctl.h>
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

static void run_child_process(int socket, const std::vector<char*>& args)
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
#if HAVE_SMPI
  smpi_init_options(); // only performed once
  xbt_assert(smpi_cfg_privatization() != SmpiPrivStrategies::MMAP,
             "Please use the dlopen privatization schema when model-checking SMPI code");
#endif

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

  xbt_assert(mc_model_checker == nullptr, "Did you manage to start the MC twice in this process?");

  auto process   = std::make_unique<simgrid::mc::RemoteProcess>(pid);
  model_checker_ = std::make_unique<simgrid::mc::ModelChecker>(std::move(process), sockets[1]);

  mc_model_checker = model_checker_.get();
  model_checker_->start();

  /* Take the initial snapshot */
  model_checker_->wait_for_requests();
  initial_snapshot_ = std::make_shared<simgrid::mc::Snapshot>(0);
}

RemoteApp::~RemoteApp()
{
  initial_snapshot_ = nullptr;
  if (model_checker_) {
    model_checker_->shutdown();
    model_checker_   = nullptr;
    mc_model_checker = nullptr;
  }
}

void RemoteApp::restore_initial_state() const
{
  this->initial_snapshot_->restore(&model_checker_->get_remote_process());
}

unsigned long RemoteApp::get_maxpid() const
{
  return model_checker_->get_remote_process().get_maxpid();
}

void RemoteApp::get_actors_status(std::map<aid_t, ActorState>& whereto) const
{
  s_mc_message_t msg;
  memset(&msg, 0, sizeof msg);
  msg.type = simgrid::mc::MessageType::ACTORS_STATUS;
  model_checker_->channel().send(msg);

  s_mc_message_actors_status_answer_t answer;
  ssize_t received = model_checker_->channel().receive(answer);
  xbt_assert(received != -1, "Could not receive message");
  xbt_assert(received == sizeof(answer) && answer.type == MessageType::ACTORS_STATUS_REPLY,
             "Received unexpected message %s (%i, size=%i) "
             "expected MessageType::ACTORS_STATUS_REPLY (%i, size=%i)",
             to_c_str(answer.type), (int)answer.type, (int)received, (int)MessageType::ACTORS_STATUS_REPLY,
             (int)sizeof(answer));

  std::vector<s_mc_message_actors_status_one_t> status(answer.count);
  if (answer.count > 0) {
    size_t size = status.size() * sizeof(s_mc_message_actors_status_one_t);
    received    = model_checker_->channel().receive(status.data(), size);
    xbt_assert(static_cast<size_t>(received) == size);
  }

  whereto.clear();
  for (auto const& actor : status)
    whereto.try_emplace(actor.aid, actor.aid, actor.enabled, actor.max_considered);
}

void RemoteApp::check_deadlock() const
{
  xbt_assert(model_checker_->channel().send(MessageType::DEADLOCK_CHECK) == 0, "Could not check deadlock state");
  s_mc_message_int_t message;
  ssize_t s = model_checker_->channel().receive(message);
  xbt_assert(s != -1, "Could not receive message");
  xbt_assert(s == sizeof(message) && message.type == MessageType::DEADLOCK_CHECK_REPLY,
             "Received unexpected message %s (%i, size=%i) "
             "expected MessageType::DEADLOCK_CHECK_REPLY (%i, size=%i)",
             to_c_str(message.type), (int)message.type, (int)s, (int)MessageType::DEADLOCK_CHECK_REPLY,
             (int)sizeof(message));

  if (message.value != 0) {
    XBT_CINFO(mc_global, "**************************");
    XBT_CINFO(mc_global, "*** DEADLOCK DETECTED ***");
    XBT_CINFO(mc_global, "**************************");
    XBT_CINFO(mc_global, "Counter-example execution trace:");
    for (auto const& frame : model_checker_->get_exploration()->get_textual_trace())
      XBT_CINFO(mc_global, "  %s", frame.c_str());
    XBT_INFO("You can debug the problem (and see the whole details) by rerunning out of simgrid-mc with "
             "--cfg=model-check/replay:'%s'",
             model_checker_->get_exploration()->get_record_trace().to_string().c_str());
    model_checker_->get_exploration()->log_state();
    throw DeadlockError();
  }
}
} // namespace simgrid::mc
