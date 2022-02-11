/* Copyright (c) 2015-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_TRANSITION_HPP
#define SIMGRID_MC_TRANSITION_HPP

#include "simgrid/forward.h" // aid_t

#include <string>

namespace simgrid {
namespace mc {

/** An element in the recorded path
 *
 *  At each decision point, we need to record which process transition
 *  is triggered and potentially which value is associated with this
 *  transition. The value is used to find which communication is triggered
 *  in things like waitany and for associating a given value of MC_random()
 *  calls.
 */
class Transition {
  /* Textual representation of the transition, to display backtraces */
  static unsigned long executed_transitions_;
  static unsigned long replayed_transitions_;

  friend State; // FIXME remove this once we have a proper class to handle the statistics

protected:
  std::string textual_ = "";

public:
  aid_t aid_ = 0;

  /* Which transition was executed for this simcall
   *
   * Some simcalls can lead to different transitions:
   *
   * * waitany/testany can trigger on different messages;
   *
   * * random can produce different values.
   */
  int times_considered_ = 0;

  Transition() = default;
  virtual ~Transition();
  Transition(aid_t issuer, int times_considered) : aid_(issuer), times_considered_(times_considered) {}

  void init(aid_t aid, int times_considered);

  virtual std::string to_string(bool verbose = false);
  const char* to_cstring(bool verbose = false);

  /* Moves the application toward a path that was already explored, but don't change the current transition */
  void replay() const;

  virtual bool depends(const Transition* other) const { return true; }

  /* Returns the total amount of transitions executed so far (for statistics) */
  static unsigned long get_executed_transitions() { return executed_transitions_; }
  /* Returns the total amount of transitions replayed so far while backtracing (for statistics) */
  static unsigned long get_replayed_transitions() { return replayed_transitions_; }
};

} // namespace mc
} // namespace simgrid

#endif
