/* Copyright (c) 2014-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** \file mc_record.hpp
 *
 *  This file contains the MC replay/record functionality.
 *  The recorded path is written in the log output and can be replayed with MC disabled
 *  (even with an non-MC build) using `--cfg=model-check/replay:$replayPath`.
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

typedef std::vector<Transition> RecordTrace;

/** Convert a string representation of the path into a array of `simgrid::mc::Transition`
 */
XBT_PRIVATE RecordTrace parseRecordTrace(const char* data);
XBT_PRIVATE std::string traceToString(simgrid::mc::RecordTrace const& trace);
XBT_PRIVATE void dumpRecordPath();

XBT_PRIVATE void replay(RecordTrace const& trace);
XBT_PRIVATE void replay(const std::string& trace);
}
}

// **** Data conversion

#endif
