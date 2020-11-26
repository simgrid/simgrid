/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_STATE_HPP
#define SIMGRID_MC_STATE_HPP

#include "src/mc/Transition.hpp"
#include "src/mc/sosp/Snapshot.hpp"
#include "src/mc/mc_pattern.hpp"

namespace simgrid {
namespace mc {

/* A node in the exploration graph (kind-of) */
class XBT_PRIVATE State {
public:
  /** Sequential state number (used for debugging) */
  int num_ = 0;

  /** State's exploration status by process */
  std::vector<ActorState> actor_states_;

  Transition transition_;

  /** The simcall which was executed, going out of that state */
  s_smx_simcall executed_req_;

  /* Internal translation of the executed_req simcall
   *
   * Simcall::COMM_TESTANY is translated to a Simcall::COMM_TEST
   * and Simcall::COMM_WAITANY to a Simcall::COMM_WAIT.
   */
  s_smx_simcall internal_req_;

  /* Can be used as a copy of the remote synchro object */
  simgrid::mc::Remote<simgrid::kernel::activity::CommImpl> internal_comm_;

  /** Snapshot of system state (if needed) */
  std::shared_ptr<simgrid::mc::Snapshot> system_state_;

  // For CommunicationDeterminismChecker
  std::vector<std::vector<simgrid::mc::PatternCommunication>> incomplete_comm_pattern_;
  std::vector<unsigned> communication_indices_;

  explicit State(unsigned long state_number);

  std::size_t interleave_size() const;
  void add_interleaving_set(const simgrid::kernel::actor::ActorImpl* actor)
  {
    this->actor_states_[actor->get_pid()].consider();
  }
  Transition get_transition() const;
};
}
}

#endif
