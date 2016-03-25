/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** \file
 *  This file contains the MC replay/record functionnality.
 *  A MC path may be recorded by using ``-cfg=model-check/record:1`'`.
 *  The path is written in the log output and an be replayed with MC disabled
 *  (even with an non-LC build) with `--cfg=model-check/replay:$replayPath`.
 *
 *  The same version of Simgrid should be used and the same arguments should be
 *  passed to the application (without the MC specific arguments).
 */

#ifndef SIMGRID_MC_RECORD_H
#define SIMGRID_MC_RECORD_H

#include <string>
#include <vector>

#include <xbt/base.h>
#include <xbt/dynar.h>
#include <xbt/fifo.h>

namespace simgrid {
namespace mc {

/** An element in the recorded path
 *
 *  At each decision point, we need to record which process transition
 *  is trigerred and potentially which value is associated with this
 *  transition. The value is used to find which communication is triggerred
 *  in things like waitany and for associating a given value of MC_random()
 *  calls.
 */
struct RecordTraceElement {
  int pid = 0;
  int value = 0;
  RecordTraceElement() {}
  RecordTraceElement(int pid, int value) : pid(pid), value(value) {}
};

typedef std::vector<RecordTraceElement> RecordTrace;

/** Convert a string representation of the path into a array of `simgrid::mc::RecordTraceElement`
 */
XBT_PRIVATE RecordTrace parseRecordTrace(const char* data);
XBT_PRIVATE std::string traceToString(simgrid::mc::RecordTrace const& trace);
XBT_PRIVATE void dumpRecordPath();

XBT_PRIVATE void replay(RecordTrace const& trace);
XBT_PRIVATE void replay(const char* trace);

}
}

SG_BEGIN_DECL()

/** Whether the MC record mode is enabled
 *
 *  The behaviour is not changed. The only real difference is that
 *  the path is writtent in the log when an interesting path is found.
 */
#define MC_record_is_active() _sg_do_model_check_record

// **** Data conversion

/** Generate a string representation
*
* The current format is a ";"-delimited list of pairs:
* "pid0,value0;pid2,value2;pid3,value3". The value can be
* omitted is it is null.
*/
XBT_PRIVATE char* MC_record_stack_to_string(xbt_fifo_t stack);

SG_END_DECL()

#endif
