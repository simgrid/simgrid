/* Copyright (c) 2016-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_CHECKER_HPP
#define SIMGRID_MC_CHECKER_HPP

#include "simgrid/forward.h"
#include "src/mc/api/RemoteApp.hpp"
#include "src/mc/mc_record.hpp"
#include <xbt/Extendable.hpp>

#include <memory>

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
  std::unique_ptr<RemoteApp> remote_app_;

  FILE* dot_output_ = nullptr;

public:
  explicit Exploration(const std::vector<char*>& args);
  virtual ~Exploration();

  // No copy:
  Exploration(Exploration const&) = delete;
  Exploration& operator=(Exploration const&) = delete;

  /** Main function of this algorithm */
  virtual void run() = 0;

  /** Kill the application and the model-checker (which exits with `status`)*/
  XBT_ATTRIB_NORETURN void system_exit(int status);

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

  RemoteApp& get_remote_app() { return *remote_app_.get(); }

  /** Print something to the dot output file*/
  void dot_output(const char* fmt, ...) XBT_ATTRIB_PRINTF(2, 3);
};

// External constructors so that the types (and the types of their content) remain hidden
XBT_PUBLIC Exploration* create_liveness_checker(const std::vector<char*>& args);
XBT_PUBLIC Exploration* create_dfs_exploration(const std::vector<char*>& args, bool with_dpor);
XBT_PUBLIC Exploration* create_communication_determinism_checker(const std::vector<char*>& args, bool with_dpor);
XBT_PUBLIC Exploration* create_udpor_checker(const std::vector<char*>& args);

} // namespace simgrid::mc

#endif
