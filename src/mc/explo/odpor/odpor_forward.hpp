/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** @file odpor_forward.hpp
 *
 *  Forward definitions for MC types specific to ODPOR
 */

#ifndef SIMGRID_MC_ODPOR_FORWARD_HPP
#define SIMGRID_MC_ODPOR_FORWARD_HPP

#include "src/mc/mc_forward.hpp"
#include <list>
#include <memory>
#include <simgrid/forward.h>

namespace simgrid::mc::odpor {

using PartialExecution = std::list<std::shared_ptr<Transition>>;

class Event;
class Execution;
class ReversibleRaceCalculator;
class WakeupTree;
class WakeupTreeNode;
class WakeupTreeIterator;

} // namespace simgrid::mc::odpor

namespace simgrid::mc {

// Permit ODPOR or SDPOR to be used as namespaces
// Many of the structures overlap, so it doesn't
// make sense to some in one and not the other.
// Having one for each algorithm makes the corresponding
// code easier to read
namespace sdpor = simgrid::mc::odpor;

} // namespace simgrid::mc

#endif
