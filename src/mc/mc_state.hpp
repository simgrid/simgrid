/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_STATE_HPP
#define SIMGRID_MC_STATE_HPP

#include "src/mc/Transition.hpp"
#include "src/mc/sosp/Snapshot.hpp"
#include "src/mc/mc_comm_pattern.hpp"

namespace simgrid {
namespace mc {

/* A node in the exploration graph (kind-of) */
class XBT_PRIVATE State {
  static long expended_states_; /* Count total amount of states, for stats */

  /* Outgoing transition: what was the last transition that we took to leave this state? Useful for replay */
  std::unique_ptr<Transition> transition_;

public:
  explicit State();

  /** Sequential state number (used for debugging) */
  long num_ = 0;

  /** State's exploration status by process */
  std::vector<ActorState> actor_states_;

  /** The simcall which was executed, going out of that state */
  s_smx_simcall executed_req_;

  /** Snapshot of system state (if needed) */
  std::shared_ptr<simgrid::mc::Snapshot> system_state_;

  // For CommunicationDeterminismChecker
  std::vector<std::vector<simgrid::mc::PatternCommunication>> incomplete_comm_pattern_;
  std::vector<unsigned> communication_indices_;


  /* Returns a positive number if there is another transition to pick, or -1 if not */
  int next_transition() const;

  /* Explore a new path */
  Transition* execute_next(int next);

  std::size_t count_todo() const;
  void mark_todo(aid_t actor) { this->actor_states_[actor].mark_todo(); }
  Transition* get_transition() const;
  void set_transition(Transition* t) { transition_.reset(t); }

  /* Returns the total amount of states created so far (for statistics) */
  static long get_expanded_states() { return expended_states_; }

private:
  void copy_incomplete_comm_pattern();
  void copy_index_comm_pattern();
};
}
}

#endif
