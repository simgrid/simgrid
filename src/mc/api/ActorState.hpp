/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_PATTERN_H
#define SIMGRID_MC_PATTERN_H

#include "src/kernel/activity/CommImpl.hpp"
#include "src/mc/remote/RemotePtr.hpp"

namespace simgrid::mc {

/* On every state, each actor has an entry of the following type.
 * This represents both the actor and its transition because
 *   an actor cannot have more than one enabled transition at a given time.
 */
class ActorState {
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
  InterleavingType state_ = InterleavingType::disabled;

  /** The ID of that actor */
  const aid_t aid_;

  /** Number of times that the actor was considered to be executed in previous explorations of the state space */
  unsigned int times_considered_ = 0;
  /** Maximal amount of times that the actor can be considered for execution in this state.
   * If times_considered==max_consider, we fully explored that part of the state space */
  unsigned int max_consider_ = 0;

  /** Whether that actor is initially enabled in this state */
  bool enabled_;

public:
  ActorState(aid_t aid, bool enabled, unsigned int max_consider)
      : aid_(aid), max_consider_(max_consider), enabled_(enabled)
  {
  }

  unsigned int do_consider()
  {
    if (max_consider_ <= times_considered_ + 1)
      set_done();
    return times_considered_++;
  }
  unsigned int get_times_considered() const { return times_considered_; }
  aid_t get_aid() const { return aid_; }

  /* returns whether the actor is marked as enabled in the application side */
  bool is_enabled() const { return enabled_; }
  /* returns whether the actor is marked as disabled by the exploration algorithm */
  bool is_disabled() const { return this->state_ == InterleavingType::disabled; }
  bool is_done() const { return this->state_ == InterleavingType::done; }
  bool is_todo() const { return this->state_ == InterleavingType::todo; }
  /** Mark that we should try executing this process at some point in the future of the checker algorithm */
  void mark_todo()
  {
    this->state_            = InterleavingType::todo;
    this->times_considered_ = 0;
  }
  void set_done() { this->state_ = InterleavingType::done; }
};

} // namespace simgrid::mc

#endif
