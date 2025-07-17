/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_REDUCTION_HPP
#define SIMGRID_MC_REDUCTION_HPP

#include "src/mc/explo/odpor/Execution.hpp"
#include <memory>

namespace simgrid::mc {

class State;

using EventHandle = uint32_t;

/**
 * @brief The reduction algorithm methods
 *
 * This is an attempt at finding a global overview of what a reduction algorithm should
 * behave like. For now the exploration is described as following the given simple pseudo-code:
 * - Compute potential races found by the model
 * - Determine if the current exploration point need to be extended
 * - Determine the set of exploration points to be considered
 */

class Reduction {
public:
  class RaceUpdate {
  public:
    RaceUpdate() = default;
    State* last_explored_state_ = nullptr; // This is mandatory if we want the // algo to handle state destruction
    State* get_last_explored_state() const { return last_explored_state_; }
  };

  Reduction()          = default;
  virtual ~Reduction() = default;

  // Eventually changes values in the stack S so that the races discovered while
  // visiting E will be taken care of at some point
  virtual RaceUpdate* races_computation(odpor::Execution& E, stack_t* S,
                                        std::vector<StatePtr>* opened_states = nullptr) = 0;
  // Create an empty race update of the right type
  virtual RaceUpdate* empty_race_update() = 0;
  // Delete a raceupdate in memory. This is required since the pointer must be of the corresponding type for a call
  // to delete.
  virtual void delete_race_update(RaceUpdate*) = 0;
  // Update the states saved in RaceUpdate accordingly to the saved informations
  // Splitting the update in two steps is mandatory for a future parallelization of the
  // race_computation() operation
  virtual unsigned long apply_race_update(RemoteApp&, RaceUpdate* updates,
                                          std::vector<StatePtr>* opened_states = nullptr) = 0;
  // Return the next aid to be explored from the E. If -1 is returned, then the
  // reduction assumes no more traces need to be explored from E.
  virtual aid_t next_to_explore(odpor::Execution& E, stack_t* S) = 0;
  // Update the state s field according to the reduction.
  // The base case is to only do the Sleep-Set procedure since most of the
  // algorithm are based on sleep sets anyway.
  virtual StatePtr state_create(RemoteApp& remote_app, StatePtr parent_state = nullptr,
                                std::shared_ptr<Transition> incoming_transition = nullptr);
  // Update the state s fields assuming we just ended the exploration of the subtree
  // rooted in s.
  // base case simply add the incoming transition to the sleep set of the parent
  virtual void on_backtrack(State* s);
  // Ask the reduction to consider one action from a given state
  //   this is required to handle so called soft-locked states
  virtual void consider_best(StatePtr state);
};

} // namespace simgrid::mc

#endif
