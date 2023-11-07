/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** @file udpor_forward.hpp
 *
 *  Forward definitions for MC types specific to UDPOR
 */

#ifndef SIMGRID_MC_UDPOR_FORWARD_HPP
#define SIMGRID_MC_UDPOR_FORWARD_HPP

#include "src/mc/mc_forward.hpp"
#include <simgrid/forward.h>

namespace simgrid::mc::udpor {

class Comb;
struct ExtensionSetCalculator;
class EventSet;
class Configuration;
class History;
class Unfolding;
class UnfoldingEvent;
struct maximal_subsets_iterator;

} // namespace simgrid::mc::udpor

#endif
