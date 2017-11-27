/* Copyright (c) 2015-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <csignal>
#include <fcntl.h>

#include <functional>

#include "xbt/log.h"
#include "xbt/system_error.hpp"
#include <mc/mc.h>
#include <simgrid/modelchecker.h>
#include <simgrid/sg_config.h>

#include "src/mc/Session.hpp"
#include "src/mc/checker/Checker.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/mc_state.hpp"

#include "src/smpi/include/private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_Session, mc, "Model-checker session");
extern std::string _sg_mc_dot_output_file;

namespace simgrid {
namespace mc {

static void setup_child_environment(int socket)
{
#ifdef __linux__
  // Make sure we do not outlive our parent:
  sigset_t mask;
  sigemptyset (&mask);
  if (sigprocmask(SIG_SETMASK, &mask, nullptr) < 0)
    throw simgrid::xbt::errno_error("Could not unblock signals");
  if (prctl(PR_SET_PDEATHSIG, SIGHUP) != 0)
    throw simgrid::xbt::errno_error("Could not PR_SET_PDEATHSIG");
#endif

  int res;

  // Remove CLOEXEC in order to pass the socket to the exec-ed program:
  int fdflags = fcntl(socket, F_GETFD, 0);
  if (fdflags == -1 || fcntl(socket, F_SETFD, fdflags & ~FD_CLOEXEC) == -1)
    throw simgrid::xbt::errno_error("Could not remove CLOEXEC for socket");

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
    throw simgrid::xbt::errno_error("Could not fork model-checked process");
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
  std::unique_ptr<simgrid::mc::RemoteClient> process(new simgrid::mc::RemoteClient(pid, socket));
  // TODO, automatic detection of the config from the process
  process->privatized(smpi_privatize_global_variables != SMPI_PRIVATIZE_NONE);
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

void Session::initialize()
{
  xbt_assert(initialSnapshot_ == nullptr);
  mc_model_checker->wait_for_requests();
  initialSnapshot_ = simgrid::mc::take_snapshot(0);
}

void Session::execute(Transition const& transition)
{
  modelChecker_->handle_simcall(transition);
  modelChecker_->wait_for_requests();
}

void Session::restoreInitialState()
{
  simgrid::mc::restore_snapshot(this->initialSnapshot_);
}

void Session::logState()
{
  mc_model_checker->getChecker()->logState();

  if (not _sg_mc_dot_output_file.empty()) {
    fprintf(dot_output, "}\n");
    fclose(dot_output);
  }
  if (getenv("SIMGRID_MC_SYSTEM_STATISTICS")){
    int ret=system("free");
    if(ret!=0)XBT_WARN("system call did not return 0, but %d",ret);
  }
}

// static
Session* Session::fork(std::function<void()> code)
{
  // Create a AF_LOCAL socketpair used for exchanging messages
  // between the model-checker process (ourselves) and the model-checked
  // process:
  int res;
  int sockets[2];
  res = socketpair(AF_LOCAL, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, sockets);
  if (res == -1)
    throw simgrid::xbt::errno_error("Could not create socketpair");

  pid_t pid = do_fork([sockets, &code] {
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
  return Session::fork([path, argv] {
    execv(path, argv);
  });
}

// static
Session* Session::spawnvp(const char *file, char *const argv[])
{
  return Session::fork([file, argv] {
    execvp(file, argv);
  });
}

void Session::close()
{
  initialSnapshot_ = nullptr;
  if (modelChecker_) {
    modelChecker_->shutdown();
    modelChecker_ = nullptr;
    mc_model_checker = nullptr;
  }
}

simgrid::mc::Session* session;

}
}
