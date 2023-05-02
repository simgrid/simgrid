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
#include <simgrid/forward.h>

namespace simgrid::mc::odpor {

class Event;
class Execution;
class ExecutionSequence;
class ExecutionView;
class WakeupTree;

} // namespace simgrid::mc::odpor

#endif
