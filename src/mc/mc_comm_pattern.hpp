/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_COMM_PATTERN_H
#define SIMGRID_MC_COMM_PATTERN_H

#include <vector>

#include "smpi/smpi.h"
#include "src/mc/mc_state.hpp"

namespace simgrid {
namespace mc {

enum class CallType {
  NONE,
  SEND,
  RECV,
  WAIT,
  WAITANY,
};

enum class CommPatternDifference {
  NONE,
  TYPE,
  RDV,
  TAG,
  SRC_PROC,
  DST_PROC,
  DATA_SIZE,
  DATA,
};

struct PatternCommunicationList {
  unsigned int index_comm = 0;
  std::vector<std::unique_ptr<simgrid::mc::PatternCommunication>> list;
};
} // namespace mc
} // namespace simgrid

extern XBT_PRIVATE std::vector<simgrid::mc::PatternCommunicationList> initial_communications_pattern;
extern XBT_PRIVATE std::vector<std::vector<simgrid::mc::PatternCommunication*>> incomplete_communications_pattern;

static inline simgrid::mc::CallType MC_get_call_type(const s_smx_simcall* req)
{
  using simgrid::mc::CallType;
  using simgrid::simix::Simcall;
  switch (req->call_) {
    case Simcall::COMM_ISEND:
      return CallType::SEND;
    case Simcall::COMM_IRECV:
      return CallType::RECV;
    case Simcall::COMM_WAIT:
      return CallType::WAIT;
    case Simcall::COMM_WAITANY:
      return CallType::WAITANY;
    default:
      return CallType::NONE;
  }
}

XBT_PRIVATE void MC_restore_communications_pattern(simgrid::mc::State* state);

#endif
