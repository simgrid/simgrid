/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_REDUCTION_HPP
#define SIMGRID_MC_REDUCTION_HPP

#include "simgrid/forward.h"
#include "src/mc/api/State.hpp"
#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/mc_config.hpp"
#include "xbt/asserts.h"

namespace simgrid::mc {

using stack_t     = std::vector<std::shared_ptr<State>>;
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
  virtual void races_computation(odpor::Execution E, stack_t* S) = 0;
  // Determine wheter the current sequence E has to be explored further. The decision
  // can be based on any information saved inside the states of S. Furthermore, the
  // reduction may do some adjustments on S to be ready to extent the exploration.
  virtual bool has_to_be_explored(odpor::Execution E, stack_t* S) = 0;
  // Return the next aid to be explored from the E. If -1 is returned, then the
  // reduction assumes no more traces need to be explored from E.
  virtual aid_t next_to_explore(odpor::Execution E, stack_t* S) = 0;
};

} // namespace simgrid::mc

#endif
