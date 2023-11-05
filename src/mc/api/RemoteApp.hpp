/* Copyright (c) 2016-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_REMOTE_APP_HPP
#define SIMGRID_MC_REMOTE_APP_HPP

#include "simgrid/forward.h"
#include "src/mc/api/ActorState.hpp"
#include "src/mc/remote/CheckerSide.hpp"
#include "src/mc/remote/RemotePtr.hpp"

#include <functional>

namespace simgrid::mc {

/** High-level view of the verified application, from the model-checker POV
 *
 *  This is expected to become the interface used by model-checking algorithms to control the execution of
 *  the application process during the exploration of the execution graph.
 *
 *  One day, this will allow parallel exploration, ie, the handling of several application processes (each encapsulated
 * in a separate CheckerSide objects) that explore several parts of the exploration graph.
 */
class XBT_PUBLIC RemoteApp {
private:
  std::unique_ptr<CheckerSide> checker_side_;
  std::unique_ptr<CheckerSide> application_factory_; // when no meminfo, create checker_side_ by cloning this one
  int master_socket_ = -1;

  const std::vector<char*> app_args_;

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

  void restore_initial_state();
  void wait_for_requests();

  /** Ask to the application to check for a deadlock. If so, do an error message and throw a McError(DEADLOCK). */
  void check_deadlock() const;

  /** Ask the application to run post-mortem analysis, and maybe to stop ASAP */
  void finalize_app(bool terminate_asap = false);

  /** Retrieve the max PID of the running actors */
  unsigned long get_maxpid() const;

  /* Get the list of actors that are ready to run at that step. Usually shorter than maxpid */
  void get_actors_status(std::map<aid_t, simgrid::mc::ActorState>& whereto) const;

  /** Take a transition. A new Transition is created iff the last parameter is true */
  Transition* handle_simcall(aid_t aid, int times_considered, bool new_transition);
};
} // namespace simgrid::mc

#endif
