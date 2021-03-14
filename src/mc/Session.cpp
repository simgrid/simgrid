/* Copyright (c) 2015-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/Session.hpp"
#include "src/mc/checker/Checker.hpp"
#include "src/mc/mc_config.hpp"
#include "src/internal_config.h" // HAVE_SMPI
#if HAVE_SMPI
#include "smpi/smpi.h"
#endif
#include "src/mc/mc_private.hpp"
#include "src/mc/mc_state.hpp"
#include "xbt/log.h"
#include "xbt/system_error.hpp"

#include <array>
#include <memory>
#include <string>

#include <fcntl.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_Session, mc, "Model-checker session");

namespace simgrid {
namespace mc {

template <class Code> void run_child_process(int socket, Code code)
{
  /* On startup, simix_global_init() calls simgrid::mc::Client::initialize(), which checks whether the MC_ENV_SOCKET_FD
   * env variable is set. If so, MC mode is assumed, and the client is setup from its side
   */

#ifdef __linux__
  // Make sure we do not outlive our parent
  sigset_t mask;
  sigemptyset (&mask);
  xbt_assert(sigprocmask(SIG_SETMASK, &mask, nullptr) >= 0, "Could not unblock signals");
  xbt_assert(prctl(PR_SET_PDEATHSIG, SIGHUP) == 0, "Could not PR_SET_PDEATHSIG");
#endif

  // Remove CLOEXEC to pass the socket to the application
  int fdflags = fcntl(socket, F_GETFD, 0);
  xbt_assert(fdflags != -1 && fcntl(socket, F_SETFD, fdflags & ~FD_CLOEXEC) != -1,
             "Could not remove CLOEXEC for socket");

  // Disable lazy relocation in the model-checked process to prevent the application from
  // modifying its .got.plt during snapshot.
  setenv("LC_BIND_NOW", "1", 1);

  setenv(MC_ENV_SOCKET_FD, std::to_string(socket).c_str(), 1);

  code();
}

Session::Session(const std::function<void()>& code)
{
#if HAVE_SMPI
  smpi_init_options();//only performed once
  xbt_assert(smpi_cfg_privatization() != SmpiPrivStrategies::MMAP,
             "Please use the dlopen privatization schema when model-checking SMPI code");
#endif

  // Create an AF_LOCAL socketpair used for exchanging messages
  // between the model-checker process (ourselves) and the model-checked
  // process:
  int sockets[2];
  int res = socketpair(AF_LOCAL, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, sockets);
  xbt_assert(res != -1, "Could not create socketpair");

  pid_t pid = fork();
  xbt_assert(pid >= 0, "Could not fork model-checked process");

  if (pid == 0) { // Child
    ::close(sockets[1]);
    run_child_process(sockets[0], code);
    DIE_IMPOSSIBLE;
  }

  // Parent (model-checker):
  ::close(sockets[0]);

  xbt_assert(mc_model_checker == nullptr, "Did you manage to start the MC twice in this process?");

  auto process   = std::make_unique<simgrid::mc::RemoteProcess>(pid);
  model_checker_ = std::make_unique<simgrid::mc::ModelChecker>(std::move(process), sockets[1]);

  mc_model_checker = model_checker_.get();
  model_checker_->start();
}

Session::~Session()
{
  this->close();
}

/** Take the initial snapshot of the application, that must be stopped. */
void Session::initialize()
{
  xbt_assert(initial_snapshot_ == nullptr);
  model_checker_->wait_for_requests();
  initial_snapshot_ = std::make_shared<simgrid::mc::Snapshot>(0);
}

void Session::execute(Transition const& transition) const
{
  model_checker_->handle_simcall(transition);
  model_checker_->wait_for_requests();
}

void Session::restore_initial_state() const
{
  this->initial_snapshot_->restore(&model_checker_->get_remote_process());
}

void Session::log_state() const
{
  model_checker_->getChecker()->log_state();

  if (not _sg_mc_dot_output_file.get().empty()) {
    fprintf(dot_output, "}\n");
    fclose(dot_output);
  }
  if (getenv("SIMGRID_MC_SYSTEM_STATISTICS")){
    int ret=system("free");
    if (ret != 0)
      XBT_WARN("Call to system(free) did not return 0, but %d", ret);
  }
}

void Session::close()
{
  initial_snapshot_ = nullptr;
  if (model_checker_) {
    model_checker_->shutdown();
    model_checker_   = nullptr;
    mc_model_checker = nullptr;
  }
}

bool Session::actor_is_enabled(aid_t pid) const
{
  s_mc_message_actor_enabled_t msg{};
  msg.type = simgrid::mc::MessageType::ACTOR_ENABLED;
  msg.aid  = pid;
  model_checker_->channel().send(msg);
  std::array<char, MC_MESSAGE_LENGTH> buff;
  ssize_t received = model_checker_->channel().receive(buff.data(), buff.size(), true);
  xbt_assert(received == sizeof(s_mc_message_int_t), "Unexpected size in answer to ACTOR_ENABLED");
  return ((s_mc_message_int_t*)buff.data())->value;
}

simgrid::mc::Session* session;

}
}
