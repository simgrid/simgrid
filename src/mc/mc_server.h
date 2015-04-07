/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_SERVER_H
#define MC_SERVER_H

#include <stdint.h>
#include <stdbool.h>

#include <sys/signalfd.h>
#include <sys/types.h>

#include <xbt/misc.h>
 
#include "mc_process.h"

SG_BEGIN_DECL()

#define MC_SERVER_ERROR 127

typedef struct s_mc_server s_mc_server_t, *mc_server_t;

extern mc_server_t mc_server;

void MC_server_wait_client(mc_process_t process);
void MC_server_simcall_handle(mc_process_t process, unsigned long pid, int value);

void MC_server_loop(mc_server_t server);

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
  void resume(mc_process_t process);
  void loop();
  bool handle_events();
protected:
  void handle_signals();
  void handle_waitpid();
  void on_signal(const struct signalfd_siginfo* info);
};

#endif

#endif
