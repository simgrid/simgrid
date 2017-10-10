/* Copyright (c) 2015-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_EXIT_HPP
#define SIMGRID_MC_EXIT_HPP
#include "xbt/base.h"

#define SIMGRID_MC_EXIT_SUCCESS 0
#define SIMGRID_MC_EXIT_SAFETY 1
#define SIMGRID_MC_EXIT_LIVENESS 2
#define SIMGRID_MC_EXIT_DEADLOCK 3
#define SIMGRID_MC_EXIT_NON_TERMINATION 4
#define SIMGRID_MC_EXIT_NON_DETERMINISM 5
#define SIMGRID_MC_EXIT_PROGRAM_CRASH 6

#define SIMGRID_MC_EXIT_ERROR 63

namespace simgrid {
namespace mc {
XBT_PUBLIC_CLASS DeadlockError{};
XBT_PUBLIC_CLASS TerminationError{};
XBT_PUBLIC_CLASS LivenessError{};
}
}

#endif
