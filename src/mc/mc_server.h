/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_SERVER_H
#define MC_SERVER_H

#include <xbt/misc.h>

SG_BEGIN_DECL()

#define MC_SERVER_ERROR 127

typedef struct s_mc_server s_mc_server_t, *mc_server_t;

extern mc_server_t mc_server;

/** Initialise MC server
 *
 * @param  PID of the model-checked process
 * @param socket file descriptor for communication with the model-checked process
 * @return 0 on success
 */
int MC_server_init(pid_t pid, int socket);

/** Execute the MC server
 *
 *  @return Status code (can be used with `exit()`)
 */
void MC_server_run(void);

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
  void handle_events();
protected:
  void handle_signals();
  void handle_waitpid();
  void on_signal(const struct signalfd_siginfo* info);
};

#endif

#endif
