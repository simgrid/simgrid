/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SERVER_H
#define SIMGRID_MC_SERVER_H

#include <poll.h>

#include <stdbool.h>

#include <sys/signalfd.h>
#include <sys/types.h>

#include <xbt/misc.h>
#include <xbt/base.h>
 
#include "src/mc/Process.hpp"
#include "mc_exit.h"

SG_BEGIN_DECL()

#define MC_SERVER_ERROR SIMGRID_ERROR

typedef struct s_mc_server s_mc_server_t, *mc_server_t;

extern mc_server_t mc_server;

SG_END_DECL()

#ifdef __cplusplus

struct s_mc_server {
private:
  pid_t pid;
  int socket;
  struct pollfd fds[2];
public:
  s_mc_server(pid_t pid, int socket);
  void start();
  void shutdown();
  void exit();
  void resume(simgrid::mc::Process& process);
  void loop();
  bool handle_events();
  void wait_client(simgrid::mc::Process& process);
  void simcall_handle(simgrid::mc::Process& process, unsigned long pid, int value);
private:
  bool handle_message(char* buffer, ssize_t size);
  void handle_signals();
  void handle_waitpid();
  void on_signal(const struct signalfd_siginfo* info);
};

#endif

#endif
