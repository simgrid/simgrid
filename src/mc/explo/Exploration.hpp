/* Copyright (c) 2016-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_CHECKER_HPP
#define SIMGRID_MC_CHECKER_HPP

#include "src/mc/api.hpp"

namespace simgrid::mc {

/** A model-checking exploration algorithm
 *
 *  This is an abstract base class used to group the data, state, configuration
 *  of a model-checking algorithm.
 *
 *  Implementing this interface will probably not be really mandatory,
 *  you might be able to write your model-checking algorithm as plain
 *  imperative code instead.
 *
 *  It is expected to interact with the model-checked application through the
 * `RemoteApp` interface (that is currently not perfectly sufficient to that extend). */
// abstract
class Exploration : public xbt::Extendable<Exploration> {
  RemoteApp& remote_app_;

public:
  explicit Exploration(RemoteApp& remote_app) : remote_app_(remote_app) {}

  // No copy:
  Exploration(Exploration const&) = delete;
  Exploration& operator=(Exploration const&) = delete;

  virtual ~Exploration() = default;

  /** Main function of this algorithm */
  virtual void run() = 0;

  /* These methods are callbacks called by the model-checking engine
   * to get and display information about the current state of the
   * model-checking algorithm: */

  /** Show the current trace/stack
   *
   *  Could this be handled in the Session/ModelChecker instead? */
  virtual RecordTrace get_record_trace() = 0;

  /** Generate a textual execution trace of the simulated application */
  virtual std::vector<std::string> get_textual_trace() = 0;

  /** Log additional information about the state of the model-checker */
  virtual void log_state();

  RemoteApp& get_remote_app() { return remote_app_; }
};

// External constructors so that the types (and the types of their content) remain hidden
XBT_PUBLIC Exploration* create_liveness_checker(RemoteApp& remote_app);
XBT_PUBLIC Exploration* create_dfs_exploration(RemoteApp& remote_app);
XBT_PUBLIC Exploration* create_communication_determinism_checker(RemoteApp& remote_app);
XBT_PUBLIC Exploration* create_udpor_checker(RemoteApp& remote_app);

} // namespace simgrid::mc

#endif
