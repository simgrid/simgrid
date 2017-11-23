/* Copyright (c) 2014-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** \file mc_record.hpp
 *
 *  This file contains the MC replay/record functionnality.
 *  A MC path may be recorded by using ``-cfg=model-check/record:1`'`.
 *  The path is written in the log output and an be replayed with MC disabled
 *  (even with an non-LC build) with `--cfg=model-check/replay:$replayPath`.
 *
 *  The same version of Simgrid should be used and the same arguments should be
 *  passed to the application (without the MC specific arguments).
 */

#ifndef SIMGRID_MC_RECORD_HPP
#define SIMGRID_MC_RECORD_HPP

#include "src/mc/Transition.hpp"
#include "xbt/base.h"

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
XBT_PRIVATE void replay(std::string trace);
}
}

/** Whether the MC record mode is enabled
 *
 *  The behaviour is not changed. The only real difference is that
 *  the path is writtent in the log when an interesting path is found.
 */
#define MC_record_is_active() _sg_do_model_check_record

// **** Data conversion

#endif
