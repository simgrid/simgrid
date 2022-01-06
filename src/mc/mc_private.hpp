/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_PRIVATE_HPP
#define SIMGRID_MC_PRIVATE_HPP

#include "mc/mc.h"
#include "xbt/automaton.h"

#include "src/mc/mc_forward.hpp"
#include "src/xbt/memory_map.hpp"

/********************************* MC Global **********************************/

XBT_PRIVATE void MC_init_dot_output();

XBT_PRIVATE extern FILE* dot_output;

/********************************** Miscellaneous **********************************/
namespace simgrid {
namespace mc {

XBT_PRIVATE void find_object_address(std::vector<simgrid::xbt::VmMap> const& maps,
                                     simgrid::mc::ObjectInformation* result);

XBT_PRIVATE
bool snapshot_equal(const Snapshot* s1, const Snapshot* s2);

// Move is somewhere else (in the LivenessChecker class, in the Session class?):
extern XBT_PRIVATE xbt_automaton_t property_automaton;
}
}

#endif
