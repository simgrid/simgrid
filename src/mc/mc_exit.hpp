/* Copyright (c) 2015-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_EXIT_HPP
#define SIMGRID_MC_EXIT_HPP
#include "simgrid/forward.h"
#include "src/kernel/activity/MemoryImpl.hpp"
#include "src/mc/explo/odpor/Execution.hpp"
#include "xbt/base.h"
#include <exception>

namespace simgrid::mc {

enum class ExitStatus {
  SUCCESS         = 0,
  SAFETY          = 1,
  DEADLOCK        = 2,
  NON_DETERMINISM = 3,
  PROGRAM_CRASH   = 4,
  DATA_RACE       = 5,
  ERROR           = 63
};

struct McError : public std::exception {
  const ExitStatus value;
  explicit McError(ExitStatus v = ExitStatus::ERROR) : value(v) {}
};

struct McDataRace : public std::exception {
  const ExitStatus value;
  const odpor::epoch first_mem_op_;
  const odpor::epoch second_mem_op_;
  MemOpType second_mem_type_;
  void* location_;
  explicit McDataRace(odpor::epoch first_mem_op, odpor::epoch second_mem_op, void* location, MemOpType second_mem_type)
      : value(ExitStatus::DATA_RACE)
      , first_mem_op_(first_mem_op)
      , second_mem_op_(second_mem_op)
      , second_mem_type_(second_mem_type)
      , location_(location)
  {
  }
};

struct McWarning : public std::exception {
  const ExitStatus value;
  explicit McWarning(ExitStatus v = ExitStatus::ERROR) : value(v) {}
};

} // namespace simgrid::mc

#endif
