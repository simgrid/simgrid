/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <exception>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <utility>

#include <fcntl.h>
#include <signal.h>
#include <poll.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/ptrace.h>

#ifdef __linux__
#include <sys/prctl.h>
#endif

#include <xbt/log.h>
#include <xbt/sysdep.h>
#include <xbt/system_error.hpp>

#include "simgrid/sg_config.h"
#include "src/xbt_modinter.h"

#include "src/mc/mc_base.h"
#include "src/mc/mc_private.h"
#include "src/mc/mc_protocol.h"
#include "src/mc/mc_safety.h"
#include "src/mc/mc_comm_pattern.h"
#include "src/mc/mc_liveness.h"
#include "src/mc/mc_exit.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_main, mc, "Entry point for simgrid-mc");

/** Execute some code in a forked process */
template<class F>
static inline
pid_t do_fork(F f)
{
  pid_t pid = fork();
  if (pid < 0)
    throw simgrid::xbt::errno_error(errno, "Could not fork model-checked process");
  if (pid != 0)
    return pid;

  // Child-process:
  try {
    f();
    _exit(EXIT_SUCCESS);
  }
  catch(...) {
    // The callback should catch exceptions:
    abort();
  }
}

static
int exec_model_checked(int socket, char** argv)
{
  XBT_DEBUG("Inside the child process PID=%i", (int) getpid());

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

  execvp(argv[1], argv+1);

  XBT_ERROR("Could not run the model-checked program");
  // This is the value used by system() and popen() in this case:
  return 127;
}

static
std::pair<pid_t, int> create_model_checked(char** argv)
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
    close(sockets[1]);
    int res = exec_model_checked(sockets[0], argv);
    XBT_DEBUG("Error in the child process creation");
    _exit(res);
  });

  // Parent (model-checker):
  close(sockets[0]);
  return std::make_pair(pid, sockets[1]);
}

static
char** argvdup(int argc, char** argv)
{
  char** argv_copy = xbt_new(char*, argc+1);
  std::memcpy(argv_copy, argv, sizeof(char*) * argc);
  argv_copy[argc] = nullptr;
  return argv_copy;
}

int main(int argc, char** argv)
{
  try {
    if (argc < 2)
      xbt_die("Missing arguments.\n");

    _sg_do_model_check = 1;

    // The initialisation function can touch argv.
    // We need to keep the original parameters in order to pass them to the
    // model-checked process so we make a copy of them:
    int argc_copy = argc;
    char** argv_copy = argvdup(argc, argv);
    xbt_log_init(&argc_copy, argv_copy);
    sg_config_init(&argc_copy, argv_copy);

    int sock;
    pid_t model_checked_pid;
    std::tie(model_checked_pid, sock) = create_model_checked(argv);
    XBT_DEBUG("Inside the parent process");
    if (mc_model_checker)
      xbt_die("MC server already present");

    mc_mode = MC_MODE_SERVER;
    std::unique_ptr<simgrid::mc::Process> process(new simgrid::mc::Process(model_checked_pid, sock));
    process->privatized(sg_cfg_get_boolean("smpi/privatize_global_variables"));
    mc_model_checker = new simgrid::mc::ModelChecker(std::move(process));
    mc_model_checker->start();
    int res = 0;
    if (_sg_mc_comms_determinism || _sg_mc_send_determinism)
      res = MC_modelcheck_comm_determinism();
    else if (!_sg_mc_property_file || _sg_mc_property_file[0] == '\0')
      res = simgrid::mc::modelcheck_safety();
    else
      res = simgrid::mc::modelcheck_liveness();
    mc_model_checker->shutdown();
    return res;
  }
  catch(std::exception& e) {
    XBT_ERROR("Exception: %s", e.what());
    return SIMGRID_MC_EXIT_ERROR;
  }
  catch(...) {
    XBT_ERROR("Unknown exception");
    return SIMGRID_MC_EXIT_ERROR;
  }
}
