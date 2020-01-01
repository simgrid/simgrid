/* Copyright (c) 2015-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_EXIT_HPP
#define SIMGRID_MC_EXIT_HPP
#include "xbt/base.h"
#include <exception>

constexpr int SIMGRID_MC_EXIT_SUCCESS         = 0;
constexpr int SIMGRID_MC_EXIT_SAFETY          = 1;
constexpr int SIMGRID_MC_EXIT_LIVENESS        = 2;
constexpr int SIMGRID_MC_EXIT_DEADLOCK        = 3;
constexpr int SIMGRID_MC_EXIT_NON_TERMINATION = 4;
constexpr int SIMGRID_MC_EXIT_NON_DETERMINISM = 5;
constexpr int SIMGRID_MC_EXIT_PROGRAM_CRASH   = 6;

constexpr int SIMGRID_MC_EXIT_ERROR           = 63;

namespace simgrid {
namespace mc {
class XBT_PUBLIC DeadlockError : public std::exception {
};
class XBT_PUBLIC TerminationError : public std::exception {
};
class XBT_PUBLIC LivenessError : public std::exception {
};
}
}

#endif
