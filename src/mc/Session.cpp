/* Copyright (c) 2015-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/Session.hpp"
#include "src/mc/checker/Checker.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/mc_state.hpp"
#include "xbt/log.h"
#include "xbt/system_error.hpp"

#include <fcntl.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_Session, mc, "Model-checker session");

namespace simgrid {
namespace mc {

static void setup_child_environment(int socket)
{
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

  // Set environment so that mmalloc gets used in application
  setenv(MC_ENV_VARIABLE, "1", 1);

  // Disable lazy relocation in the model-checked process to prevent the application from
  // modifying its .got.plt during snapshot.
  setenv("LC_BIND_NOW", "1", 1);

  char buffer[64];
  int res = std::snprintf(buffer, sizeof(buffer), "%i", socket);
  xbt_assert((size_t)res < sizeof(buffer) && res != -1);
  setenv(MC_ENV_SOCKET_FD, buffer, 1);
}

Session::Session(const std::function<void()>& code)
{
#if HAVE_SMPI
  xbt_assert(smpi_cfg_privatization() != SmpiPrivStrategies::MMAP,
             "Please use the dlopen privatization schema when model-checking SMPI code");
#endif

  // Create a AF_LOCAL socketpair used for exchanging messages
  // between the model-checker process (ourselves) and the model-checked
  // process:
  int sockets[2];
  int res = socketpair(AF_LOCAL, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, sockets);
  xbt_assert(res != -1, "Could not create socketpair");

  pid_t pid = fork();
  xbt_assert(pid >= 0, "Could not fork model-checked process");

  if (pid == 0) { // Child
    ::close(sockets[1]);
    setup_child_environment(sockets[0]);
    code();
    xbt_die("The model-checked process failed to exec()");
  }

  // Parent (model-checker):
  ::close(sockets[0]);

  xbt_assert(mc_model_checker == nullptr, "Did you manage to start the MC twice in this process?");

  std::unique_ptr<simgrid::mc::RemoteClient> process(new simgrid::mc::RemoteClient(pid, sockets[1]));
  model_checker_.reset(new simgrid::mc::ModelChecker(std::move(process)));

  mc_model_checker = model_checker_.get();
  mc_model_checker->start();
}

Session::~Session()
{
  this->close();
}

void Session::initialize()
{
  xbt_assert(initial_snapshot_ == nullptr);
  mc_model_checker->wait_for_requests();
  initial_snapshot_ = std::make_shared<simgrid::mc::Snapshot>(0);
}

void Session::execute(Transition const& transition)
{
  model_checker_->handle_simcall(transition);
  model_checker_->wait_for_requests();
}

void Session::restore_initial_state()
{
  this->initial_snapshot_->restore(&mc_model_checker->process());
}

void Session::log_state()
{
  mc_model_checker->getChecker()->log_state();

  if (not _sg_mc_dot_output_file.get().empty()) {
    fprintf(dot_output, "}\n");
    fclose(dot_output);
  }
  if (getenv("SIMGRID_MC_SYSTEM_STATISTICS")){
    int ret=system("free");
    if(ret!=0)XBT_WARN("system call did not return 0, but %d",ret);
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

simgrid::mc::Session* session;

}
}
