/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SAFETY_HPP
#define SIMGRID_MC_SAFETY_HPP

#include "xbt/base.h"

namespace simgrid {
namespace mc {

enum class ReductionMode {
  unset,
  none,
  dpor,
};

extern XBT_PRIVATE simgrid::mc::ReductionMode reduction_mode;
}
}

#endif
