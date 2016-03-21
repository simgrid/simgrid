/* Copyright (c) 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SESSION_HPP
#define SIMGRID_MC_SESSION_HPP

#ifdef __linux__
#include <sys/prctl.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <xbt/sysdep.h>
#include <xbt/system_error.hpp>

#include <functional>

#include <xbt/log.h>

#include "src/mc/mc_forward.hpp"
#include "src/mc/ModelChecker.hpp"

namespace simgrid {
namespace mc {

/** A model-checking session
 *
 *  This is expected to become the interface used by model-checking
 *  algorithms to control the execution of the model-checked process
 *  and the exploration of the execution graph. Model-checking
 *  algorithms should be able to be written in high-level languages
 *  (e.g. Python) using bindings on this interface.
 */
class Session {
private:
  std::unique_ptr<ModelChecker> modelChecker_;

private: //
  Session(pid_t pid, int socket);

  // No copy:
  Session(Session const&) = delete;
  Session& operator=(Session const&) = delete;

public:
  ~Session();
  void close();

public: // static constructors

  /** Create a new session by forking
   *
   *  The code is expected to `exec` the model-checker program.
   */
  static Session* fork(std::function<void(void)> code);

  /** Create a session using `execv` */
  static Session* spawnv(const char *path, char *const argv[]);

  /** Create a session using `execvp` */
  static Session* spawnvp(const char *path, char *const argv[]);
};

// Temporary
extern simgrid::mc::Session* session;

}
}

#endif
