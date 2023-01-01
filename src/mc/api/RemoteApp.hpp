/* Copyright (c) 2016-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_REMOTE_APP_HPP
#define SIMGRID_MC_REMOTE_APP_HPP

#include "simgrid/forward.h"
#include "src/mc/ModelChecker.hpp"
#include "src/mc/api/ActorState.hpp"
#include "src/mc/remote/RemotePtr.hpp"

#include <functional>

namespace simgrid::mc {

/** High-level view of the verified application, from the model-checker POV
 *
 *  This is expected to become the interface used by model-checking
 *  algorithms to control the execution of the model-checked process
 *  and the exploration of the execution graph. Model-checking
 *  algorithms should be able to be written in high-level languages
 *  (e.g. Python) using bindings on this interface.
 */
class XBT_PUBLIC RemoteApp {
private:
  std::unique_ptr<ModelChecker> model_checker_;
  std::shared_ptr<simgrid::mc::Snapshot> initial_snapshot_;

  // No copy:
  RemoteApp(RemoteApp const&) = delete;
  RemoteApp& operator=(RemoteApp const&) = delete;

public:
  /** Create a new session by executing the provided code in a fork()
   *
   *  This sets up the environment for the model-checked process
   *  (environment variables, sockets, etc.).
   *
   *  The code is expected to `exec` the model-checked application.
   */
  explicit RemoteApp(const std::vector<char*>& args);

  ~RemoteApp();

  void restore_initial_state() const;

  /** Ask to the application to check for a deadlock. If so, do an error message and throw a DeadlockError. */
  void check_deadlock() const;

  /** Retrieve the max PID of the running actors */
  unsigned long get_maxpid() const;

  /* Get the list of actors that are ready to run at that step. Usually shorter than maxpid */
  void get_actors_status(std::map<aid_t, simgrid::mc::ActorState>& whereto) const;

  /* Get the remote process */
  RemoteProcess& get_remote_process() { return model_checker_->get_remote_process(); }
};
} // namespace simgrid::mc

#endif
