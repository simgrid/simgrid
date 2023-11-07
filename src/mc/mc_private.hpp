/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_PRIVATE_HPP
#define SIMGRID_MC_PRIVATE_HPP

#include "src/mc/mc.h"

#include "src/mc/mc_forward.hpp"
#include "src/xbt/memory_map.hpp"

/********************************** Miscellaneous **********************************/
namespace simgrid::mc {

XBT_PRIVATE void find_object_address(std::vector<simgrid::xbt::VmMap> const& maps,
                                     simgrid::mc::ObjectInformation* result);

} // namespace simgrid::mc

#endif
