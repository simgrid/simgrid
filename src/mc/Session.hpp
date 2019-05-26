/* Copyright (c) 2016-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SESSION_HPP
#define SIMGRID_MC_SESSION_HPP

#include "src/mc/ModelChecker.hpp"

#include <functional>

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
  std::shared_ptr<simgrid::mc::Snapshot> initialSnapshot_;

  Session(pid_t pid, int socket);

  // No copy:
  Session(Session const&) = delete;
  Session& operator=(Session const&) = delete;

public:
  ~Session();
  void close();

  void initialize();
  void execute(Transition const& transition);
  void logState();

  void restoreInitialState();

  // static constructors

  /** Create a new session by forking
   *
   *  This sets up the environment for the model-checked process
   *  (environment variables, sockets, etc.).
   *
   *  The code is expected to `exec` the model-checker program.
   */
  static Session* fork(const std::function<void()>& code);

  /** Spawn a model-checked process
   *
   *  @param path full path of the executable
   *  @param argv arguments for the model-checked process (NULL-terminated)
   */
  static Session* spawnv(const char *path, char *const argv[]);

  /** Spawn a model-checked process (using PATH)
   *
   *  @param file file name of the executable (found using `PATH`)
   *  @param argv arguments for the model-checked process (NULL-terminated)
   */
  static Session* spawnvp(const char *file, char *const argv[]);
};

// Temporary :)
extern simgrid::mc::Session* session;

}
}

#endif
