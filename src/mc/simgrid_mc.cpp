/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <exception>

#include <cstdlib>
#include <cstdio>
#include <cstring>

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

static int do_child(int socket, char** argv)
{
  XBT_DEBUG("Inside the child process PID=%i", (int) getpid());

#ifdef __linux__
  // Make sure we do not outlive our parent:
  if (prctl(PR_SET_PDEATHSIG, SIGHUP) != 0) {
    std::perror("simgrid-mc");
    return SIMGRID_MC_EXIT_ERROR;
  }
#endif

  int res;

  // Remove CLOEXEC in order to pass the socket to the exec-ed program:
  int fdflags = fcntl(socket, F_GETFD, 0);
  if (fdflags == -1) {
    std::perror("simgrid-mc");
    return SIMGRID_MC_EXIT_ERROR;
  }
  if (fcntl(socket, F_SETFD, fdflags & ~FD_CLOEXEC) == -1) {
    std::perror("simgrid-mc");
    return SIMGRID_MC_EXIT_ERROR;
  }

  XBT_DEBUG("CLOEXEC removed on socket %i", socket);

  // Set environment:
  setenv(MC_ENV_VARIABLE, "1", 1);

  // Disable lazy relocation in the model-ched process.
  // We don't want the model-checked process to modify its .got.plt during
  // snapshot.
  setenv("LC_BIND_NOW", "1", 1);

  char buffer[64];
  res = std::snprintf(buffer, sizeof(buffer), "%i", socket);
  if ((size_t) res >= sizeof(buffer) || res == -1)
    return SIMGRID_MC_EXIT_ERROR;
  setenv(MC_ENV_SOCKET_FD, buffer, 1);

  execvp(argv[1], argv+1);
  XBT_ERROR("Could not execute the child process");
  return SIMGRID_MC_EXIT_ERROR;
}

static int do_parent(int socket, pid_t child)
{
  XBT_DEBUG("Inside the parent process");
  if (mc_model_checker)
    xbt_die("MC server already present");
  try {
    mc_mode = MC_MODE_SERVER;
    mc_model_checker = new simgrid::mc::ModelChecker(child, socket);
    mc_model_checker->start();
    int res = 0;
    if (_sg_mc_comms_determinism || _sg_mc_send_determinism)
      res = MC_modelcheck_comm_determinism();
    else if (!_sg_mc_property_file || _sg_mc_property_file[0] == '\0')
      res = MC_modelcheck_safety();
    else
      res = MC_modelcheck_liveness();
    mc_model_checker->shutdown();
    return res;
  }
  catch(std::exception& e) {
    XBT_ERROR("Exception: %s", e.what());
    return SIMGRID_MC_EXIT_ERROR;
  }
}

static char** argvdup(int argc, char** argv)
{
  char** argv_copy = xbt_new(char*, argc+1);
  std::memcpy(argv_copy, argv, sizeof(char*) * argc);
  argv_copy[argc] = NULL;
  return argv_copy;
}

int main(int argc, char** argv)
{
  _sg_do_model_check = 1;

  // We need to keep the original parameters in order to pass them to the
  // model-checked process:
  int argc_copy = argc;
  char** argv_copy = argvdup(argc, argv);
  xbt_log_init(&argc_copy, argv_copy);
  sg_config_init(&argc_copy, argv_copy);

  if (argc < 2)
    xbt_die("Missing arguments.\n");

  // Create a AF_LOCAL socketpair:
  int res;

  int sockets[2];
  res = socketpair(AF_LOCAL, SOCK_DGRAM | SOCK_CLOEXEC, 0, sockets);
  if (res == -1) {
    perror("simgrid-mc");
    return SIMGRID_MC_EXIT_ERROR;
  }

  XBT_DEBUG("Created socketpair");

  pid_t pid = fork();
  if (pid < 0) {
    perror("simgrid-mc");
    return SIMGRID_MC_EXIT_ERROR;
  } else if (pid == 0) {
    close(sockets[1]);
    int res = do_child(sockets[0], argv);
    XBT_DEBUG("Error in the child process creation");
    return res;
  } else {
    close(sockets[0]);
    return do_parent(sockets[1], pid);
  }

  return 0;
}
