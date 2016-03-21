/* Copyright (c) 2015-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <fcntl.h>
#include <signal.h>

#include <functional>

#include <xbt/system_error.hpp>
#include <simgrid/sg_config.h>
#include <simgrid/modelchecker.h>

#include "src/mc/Session.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_Session, mc, "Model-checker session");

namespace simgrid {
namespace mc {

static void setup_child_environment(int socket)
{
#ifdef __linux__
  // Make sure we do not outlive our parent:
  sigset_t mask;
  sigemptyset (&mask);
  if (sigprocmask(SIG_SETMASK, &mask, nullptr) < 0)
    throw simgrid::xbt::errno_error(errno, "Could not unblock signals");
  if (prctl(PR_SET_PDEATHSIG, SIGHUP) != 0)
    throw simgrid::xbt::errno_error(errno, "Could not PR_SET_PDEATHSIG");
#endif

  int res;

  // Remove CLOEXEC in order to pass the socket to the exec-ed program:
  int fdflags = fcntl(socket, F_GETFD, 0);
  if (fdflags == -1 || fcntl(socket, F_SETFD, fdflags & ~FD_CLOEXEC) == -1)
    throw simgrid::xbt::errno_error(errno, "Could not remove CLOEXEC for socket");

  // Set environment:
  setenv(MC_ENV_VARIABLE, "1", 1);

  // Disable lazy relocation in the model-checked process.
  // We don't want the model-checked process to modify its .got.plt during
  // snapshot.
  setenv("LC_BIND_NOW", "1", 1);

  char buffer[64];
  res = std::snprintf(buffer, sizeof(buffer), "%i", socket);
  if ((size_t) res >= sizeof(buffer) || res == -1)
    std::abort();
  setenv(MC_ENV_SOCKET_FD, buffer, 1);
}

/** Execute some code in a forked process */
template<class F>
static inline
pid_t do_fork(F code)
{
  pid_t pid = fork();
  if (pid < 0)
    throw simgrid::xbt::errno_error(errno, "Could not fork model-checked process");
  if (pid != 0)
    return pid;

  // Child-process:
  try {
    code();
    _exit(EXIT_SUCCESS);
  }
  catch(...) {
    // The callback should catch exceptions:
    std::terminate();
  }
}

Session::Session(pid_t pid, int socket)
{
  std::unique_ptr<simgrid::mc::Process> process(new simgrid::mc::Process(pid, socket));
  // TODO, automatic detection of the config from the process
  process->privatized(
    sg_cfg_get_boolean("smpi/privatize_global_variables"));
  modelChecker_ = std::unique_ptr<ModelChecker>(
    new simgrid::mc::ModelChecker(std::move(process)));
  xbt_assert(mc_model_checker == nullptr);
  mc_model_checker = modelChecker_.get();
  mc_model_checker->start();
}

Session::~Session()
{
  this->close();
}

// static
Session* Session::fork(std::function<void(void)> code)
{
  // Create a AF_LOCAL socketpair used for exchanging messages
  // bewteen the model-checker process (ourselves) and the model-checked
  // process:
  int res;
  int sockets[2];
  res = socketpair(AF_LOCAL, SOCK_DGRAM | SOCK_CLOEXEC, 0, sockets);
  if (res == -1)
    throw simgrid::xbt::errno_error(errno, "Could not create socketpair");

  pid_t pid = do_fork([&] {
    ::close(sockets[1]);
    setup_child_environment(sockets[0]);
    code();
    xbt_die("The model-checked process failed to exec()");
  });

  // Parent (model-checker):
  ::close(sockets[0]);

  return new Session(pid, sockets[1]);
}

// static
Session* Session::spawnv(const char *path, char *const argv[])
{
  return Session::fork([&] {
    execv(path, argv);
  });
}

// static
Session* Session::spawnvp(const char *path, char *const argv[])
{
  return Session::fork([&] {
    execvp(path, argv);
  });
}

void Session::close()
{
  if (modelChecker_) {
    modelChecker_->shutdown();
    modelChecker_ = nullptr;
    mc_model_checker = nullptr;
  }
}

simgrid::mc::Session* session;

}
}