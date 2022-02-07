/* Copyright (c) 2016-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SESSION_HPP
#define SIMGRID_MC_SESSION_HPP

#include "simgrid/forward.h"
#include "src/mc/ModelChecker.hpp"
#include "src/mc/remote/RemotePtr.hpp"

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
class XBT_PUBLIC Session {
private:
  std::unique_ptr<ModelChecker> model_checker_;
  std::shared_ptr<simgrid::mc::Snapshot> initial_snapshot_;

  // No copy:
  Session(Session const&) = delete;
  Session& operator=(Session const&) = delete;

public:
  /** Create a new session by executing the provided code in a fork()
   *
   *  This sets up the environment for the model-checked process
   *  (environment variables, sockets, etc.).
   *
   *  The code is expected to `exec` the model-checked application.
   */
  explicit Session(const std::function<void()>& code);

  ~Session();
  void close();

  void take_initial_snapshot();
  void log_state() const;

  void restore_initial_state() const;
  bool actor_is_enabled(aid_t pid) const;
};

// Temporary :)
extern simgrid::mc::Session* session_singleton;
}
}

#endif
