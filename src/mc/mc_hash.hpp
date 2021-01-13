/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_HASH_HPP
#define SIMGRID_MC_HASH_HPP

#include "xbt/base.h"
#include "src/mc/mc_forward.hpp"

namespace simgrid {
namespace mc {

using hash_type = std::uint64_t;

XBT_PRIVATE hash_type hash(simgrid::mc::Snapshot const& snapshot);

}
}

#endif
