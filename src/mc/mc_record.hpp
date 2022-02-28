/* Copyright (c) 2014-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** \file mc_record.hpp
 *
 *  This file contains the MC replay/record functionality.
 *  The recorded path is written in the log output and can be replayed with MC disabled
 *  (even with a non-MC build) using `--cfg=model-check/replay:$replayPath`.
 *
 *  The same version of Simgrid should be used and the same arguments should be
 *  passed to the application (without the MC specific arguments).
 */

#ifndef SIMGRID_MC_RECORD_HPP
#define SIMGRID_MC_RECORD_HPP

#include "src/mc/mc_forward.hpp"
#include "xbt/base.h"

#include <string>
#include <vector>

namespace simgrid {
namespace mc {

class RecordTrace {
  std::vector<Transition*> transitions_;

public:
  RecordTrace() = default;

  /** Build a trace that can be replayed from a string representation */
  explicit RecordTrace(const char* data);
  /** Make a string representation that can later be used to create a new trace */
  std::string to_string() const;

  void push_back(Transition* t) { transitions_.push_back(t); }

  /** Replay all transitions of a trace */
  void replay() const;

  /** Parse and replay a string representation */
  static void replay(const std::string& trace);
};

}
}

#endif
