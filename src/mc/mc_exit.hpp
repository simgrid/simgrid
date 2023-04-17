/* Copyright (c) 2015-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_EXIT_HPP
#define SIMGRID_MC_EXIT_HPP
#include "xbt/base.h"
#include <exception>

namespace simgrid::mc {

enum class ExitStatus {
  SUCCESS         = 0,
  SAFETY          = 1,
  LIVENESS        = 2,
  DEADLOCK        = 3,
  NON_TERMINATION = 4,
  NON_DETERMINISM = 5,
  PROGRAM_CRASH   = 6,
  ERROR           = 63
};

struct McError : public std::exception {
  const ExitStatus value;
  explicit McError(ExitStatus v = ExitStatus::ERROR) : value(v) {}
};
} // namespace simgrid::mc

#endif
