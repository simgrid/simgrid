/* Copyright (c) 2015-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_TRANSITION_HPP
#define SIMGRID_MC_TRANSITION_HPP

#include "simgrid/forward.h" // aid_t
#include "src/mc/remote/RemotePtr.hpp"
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
  std::string textual_ = "";
  static unsigned long executed_transitions_;

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

  void init(aid_t aid, int times_considered);

  std::string to_string() const;

  /* Moves the application toward a path that was already explored, but don't change the current transition */
  RemotePtr<simgrid::kernel::actor::SimcallObserver> replay() const;

  /* Returns the total amount of transitions executed so far (for statistics) */
  static unsigned long get_executed_transitions() { return executed_transitions_; }
};

} // namespace mc
} // namespace simgrid

#endif
