/* Copyright (c) 2007-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_REDUCTION_HPP
#define SIMGRID_MC_REDUCTION_HPP

#include "src/mc/explo/odpor/Execution.hpp"

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
  Reduction()          = default;
  virtual ~Reduction() = default;

  // Eventually changes values in the stack S so that the races discovered while
  // visiting E will be taken care of at some point
  virtual void races_computation(odpor::Execution& E, stack_t* S, std::vector<StatePtr>* opened_states = nullptr) = 0;
  // Return the next aid to be explored from the E. If -1 is returned, then the
  // reduction assumes no more traces need to be explored from E.
  virtual aid_t next_to_explore(odpor::Execution& E, stack_t* S) = 0;
  // Update the state s field according to the reduction.
  // The base case is to only do the Sleep-Set procedure since most of the
  // algorithm are based on sleep sets anyway.
  virtual StatePtr state_create(RemoteApp& remote_app, StatePtr parent_state = nullptr);

  // Update the state s fields assuming we just ended the exploration of the subtree
  // rooted in s.
  // base case simply add the incoming transition to the sleep set of the parent
  virtual void on_backtrack(State* s);
};

} // namespace simgrid::mc

#endif
