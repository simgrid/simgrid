/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <signal.h>
#include <poll.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <xbt/log.h>

#include "simgrid/sg_config.h"
#include "xbt_modinter.h"

#include "mc_base.h"
#include "mc_private.h"
#include "mc_protocol.h"
#include "mc_server.h"
#include "mc_model_checker.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_main, mc, "Entry point for simgrid-mc");

static const bool trace = true;

static int do_child(int socket, char** argv)
{
  XBT_DEBUG("Inside the child process PID=%i", (int) getpid());
  int res;

  // Remove CLOEXEC in order to pass the socket to the exec-ed program:
  int fdflags = fcntl(socket, F_GETFD, 0);
  if (fdflags == -1) {
    std::perror("simgrid-mc");
    return MC_SERVER_ERROR;
  }
  if (fcntl(socket, F_SETFD, fdflags & ~FD_CLOEXEC) == -1) {
    std::perror("simgrid-mc");
    return MC_SERVER_ERROR;
  }

  XBT_DEBUG("CLOEXEC removed on socket %i", socket);

  // Set environment:
  setenv(MC_ENV_VARIABLE, "1", 1);

  char buffer[64];
  res = std::snprintf(buffer, sizeof(buffer), "%i", socket);
  if ((size_t) res >= sizeof(buffer) || res == -1)
    return MC_SERVER_ERROR;
  setenv(MC_ENV_SOCKET_FD, buffer, 1);

  execvp(argv[1], argv+1);
  std::perror("simgrid-mc");
  return MC_SERVER_ERROR;
}

static int do_parent(int socket, pid_t child)
{
  XBT_DEBUG("Inside the parent process");
  if (MC_server_init(child, socket))
    return MC_SERVER_ERROR;
  XBT_DEBUG("Server initialized");
  MC_server_run();
  return 0;
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
  // We need to keep the original parameters in order to pass them to the
  // model-checked process:
  int argc_copy = argc;
  char** argv_copy = argvdup(argc, argv);
  xbt_log_init(&argc_copy, argv_copy);
  sg_config_init(&argc_copy, argv_copy);

  if (argc < 2)
    xbt_die("Missing arguments.\n");

  bool server_mode = false;
  char* env = std::getenv("SIMGRID_MC_MODE");
  if (env) {
    if (std::strcmp(env, "server") == 0)
      server_mode = true;
    else if (std::strcmp(env, "standalone") == 0)
      server_mode = false;
    else
      XBT_WARN("Unrecognised value for SIMGRID_MC_MODE (server/standalone)");
  }

  if (!server_mode) {
    setenv(MC_ENV_VARIABLE, "1", 1);
    execvp(argv[1], argv+1);

    std::perror("simgrid-mc");
    return 127;
  }

  // Create a AF_LOCAL socketpair:
  int res;

  int sockets[2];
  res = socketpair(AF_LOCAL, SOCK_DGRAM | SOCK_CLOEXEC, 0, sockets);
  if (res == -1) {
    perror("simgrid-mc");
    return MC_SERVER_ERROR;
  }

  XBT_DEBUG("Created socketpair");

  pid_t pid = fork();
  if (pid < 0) {
    perror("simgrid-mc");
    return MC_SERVER_ERROR;
  } else if (pid == 0) {
    close(sockets[1]);
    return do_child(sockets[0], argv);
  } else {
    close(sockets[0]);
    return do_parent(sockets[1], pid);
  }

  return 0;
}
