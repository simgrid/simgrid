/* Copyright (c) 2016-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_CHECKER_HPP
#define SIMGRID_MC_CHECKER_HPP

#include "src/mc/api.hpp"

namespace simgrid {
namespace mc {

/** A model-checking algorithm
 *
 *  This is an abstract base class used to group the data, state, configuration
 *  of a model-checking algorithm.
 *
 *  Implementing this interface will probably not be really mandatory,
 *  you might be able to write your model-checking algorithm as plain
 *  imperative code instead.
 *
 *  It is expected to interact with the model-checking core through the
 * `Session` interface (but currently the `Session` interface does not
 *  have all the necessary features). */
// abstract
class Checker {
public:
  inline explicit Checker() { Api::get().set_checker(this); }

  // No copy:
  Checker(Checker const&) = delete;
  Checker& operator=(Checker const&) = delete;

  virtual ~Checker() = default;

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
  virtual void log_state() = 0;
};

// External constructors so that the types (and the types of their content) remain hidden
XBT_PUBLIC Checker* createLivenessChecker();
XBT_PUBLIC Checker* createSafetyChecker();
XBT_PUBLIC Checker* createCommunicationDeterminismChecker();
XBT_PUBLIC Checker* createUdporChecker();

} // namespace mc
} // namespace simgrid

#endif
