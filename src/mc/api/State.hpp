/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_STATE_HPP
#define SIMGRID_MC_STATE_HPP

#include "src/mc/mc_pattern.hpp"
#include "src/mc/sosp/Snapshot.hpp"
#include "src/mc/transition/Transition.hpp"

namespace simgrid {
namespace mc {

/* A node in the exploration graph (kind-of) */
class XBT_PRIVATE State : public xbt::Extendable<State> {
  static long expended_states_; /* Count total amount of states, for stats */

  /* Outgoing transition: what was the last transition that we took to leave this state? */
  std::unique_ptr<Transition> transition_;

  /** Sequential state number (used for debugging) */
  long num_ = 0;

  /** State's exploration status by process */
  std::vector<ActorState> actor_states_;

  /** Snapshot of system state (if needed) */
  std::shared_ptr<Snapshot> system_state_;

public:
  explicit State();

  /* Returns a positive number if there is another transition to pick, or -1 if not */
  int next_transition() const;

  /* Explore a new path; the parameter must be the result of a previous call to next_transition() */
  void execute_next(int next);

  long get_num() const { return num_; }
  std::size_t count_todo() const;
  void mark_todo(aid_t actor) { this->actor_states_[actor].mark_todo(); }
  bool is_done(aid_t actor) const { return this->actor_states_[actor].is_done(); }
  Transition* get_transition() const;
  void set_transition(Transition* t) { transition_.reset(t); }

  Snapshot* get_system_state() const { return system_state_.get(); }
  void set_system_state(std::shared_ptr<Snapshot> state) { system_state_ = std::move(state); }

  /* Returns the total amount of states created so far (for statistics) */
  static long get_expanded_states() { return expended_states_; }
};
} // namespace mc
} // namespace simgrid

#endif
