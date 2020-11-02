/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_STATE_DEV_HPP
#define SIMGRID_MC_STATE_DEV_HPP

#include "src/kernel/activity/CommImpl.hpp"
#include "src/mc/Transition.hpp"
#include "src/mc/sosp/Snapshot.hpp"

namespace simgrid {
namespace mc {

/* On every state, each actor has an entry of the following type.
 * This represents both the actor and its transition because
 *   an actor cannot have more than one enabled transition at a given time.
 */
class ActorState_Dev {
  /* Possible exploration status of an actor transition in a state.
   * Either the checker did not consider the transition, or it was considered and still to do, or considered and done.
   */
  enum class InterleavingType {
    /** This actor transition is not considered by the checker (yet?) */
    disabled = 0,
    /** The checker algorithm decided that this actor transitions should be done at some point */
    todo,
    /** The checker algorithm decided that this should be done, but it was done in the meanwhile */
    done,
  };

  /** Exploration control information */
  InterleavingType state = InterleavingType::disabled;

public:
  /** Number of times that the process was considered to be executed */
  // TODO, make this private
  unsigned int times_considered = 0;

  bool is_disabled() const { return this->state == InterleavingType::disabled; }
  bool is_done() const { return this->state == InterleavingType::done; }
  bool is_todo() const { return this->state == InterleavingType::todo; }
  /** Mark that we should try executing this process at some point in the future of the checker algorithm */
  void consider()
  {
    this->state            = InterleavingType::todo;
    this->times_considered = 0;
  }
  void set_done() { this->state = InterleavingType::done; }
};

/* A node in the exploration graph (kind-of) */
class XBT_PRIVATE State_Dev {
public:
  /** Sequential state number (used for debugging) */
  int num_ = 0;

  /** State's exploration status by process */
  std::vector<ActorState_Dev> actor_states_;

  /** The simcall which was executed, going out of that state */
  s_smx_simcall executed_req_;

  /* Internal translation of the executed_req simcall
   *
   * SIMCALL_COMM_TESTANY is translated to a SIMCALL_COMM_TEST
   * and SIMCALL_COMM_WAITANY to a SIMCALL_COMM_WAIT.
   */
  s_smx_simcall internal_req_;

  explicit State_Dev(unsigned long state_number);

  std::size_t interleave_size() const;
  
  void add_interleaving_set(const simgrid::kernel::actor::ActorImpl* actor)
  {
    this->actor_states_[actor->get_pid()].consider();
  }
  
  Transition transition_;
};
}
}

XBT_PRIVATE smx_simcall_t MC_state_choose_request_dev(simgrid::mc::State_Dev* state);

#endif
