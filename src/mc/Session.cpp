/* Copyright (c) 2015-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <fcntl.h>
#include <signal.h>

#include <functional>

#include <xbt/log.h>
#include <xbt/system_error.hpp>
#include <simgrid/sg_config.h>
#include <simgrid/modelchecker.h>
#include <mc/mc.h>

#include "src/mc/Session.hpp"
#include "src/mc/mc_state.h"
#include "src/mc/mc_private.h"

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
    xbt_cfg_get_boolean("smpi/privatize_global_variables"));
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

void Session::execute(Transition const& transition)
{
  modelChecker_->handle_simcall(transition);
  modelChecker_->wait_for_requests();
}

void Session::logState()
{
  if(_sg_mc_comms_determinism) {
    if (!simgrid::mc::initial_global_state->recv_deterministic &&
        simgrid::mc::initial_global_state->send_deterministic){
      XBT_INFO("******************************************************");
      XBT_INFO("**** Only-send-deterministic communication pattern ****");
      XBT_INFO("******************************************************");
      XBT_INFO("%s", simgrid::mc::initial_global_state->recv_diff);
    }else if(!simgrid::mc::initial_global_state->send_deterministic &&
        simgrid::mc::initial_global_state->recv_deterministic) {
      XBT_INFO("******************************************************");
      XBT_INFO("**** Only-recv-deterministic communication pattern ****");
      XBT_INFO("******************************************************");
      XBT_INFO("%s", simgrid::mc::initial_global_state->send_diff);
    }
  }

  if (mc_stats->expanded_pairs == 0) {
    XBT_INFO("Expanded states = %lu", mc_stats->expanded_states);
    XBT_INFO("Visited states = %lu", mc_stats->visited_states);
  } else {
    XBT_INFO("Expanded pairs = %lu", mc_stats->expanded_pairs);
    XBT_INFO("Visited pairs = %lu", mc_stats->visited_pairs);
  }
  XBT_INFO("Executed transitions = %lu", mc_stats->executed_transitions);
  if ((_sg_mc_dot_output_file != nullptr) && (_sg_mc_dot_output_file[0] != '\0')) {
    fprintf(dot_output, "}\n");
    fclose(dot_output);
  }
  if (simgrid::mc::initial_global_state != nullptr
      && (_sg_mc_comms_determinism || _sg_mc_send_determinism)) {
    XBT_INFO("Send-deterministic : %s",
      !simgrid::mc::initial_global_state->send_deterministic ? "No" : "Yes");
    if (_sg_mc_comms_determinism)
      XBT_INFO("Recv-deterministic : %s",
        !simgrid::mc::initial_global_state->recv_deterministic ? "No" : "Yes");
  }
  if (getenv("SIMGRID_MC_SYSTEM_STATISTICS")){
    int ret=system("free");
    if(ret!=0)XBT_WARN("system call did not return 0, but %d",ret);
  }
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
