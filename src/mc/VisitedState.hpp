/* Copyright (c) 2007-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_VISITED_STATE_HPP
#define SIMGRID_MC_VISITED_STATE_HPP

#include <cstddef>

#include <memory>

#include "src/mc/mc_snapshot.h"

namespace simgrid {
namespace mc {

struct XBT_PRIVATE VisitedState {
  std::shared_ptr<simgrid::mc::Snapshot> system_state = nullptr;
  std::size_t heap_bytes_used = 0;
  int nb_processes = 0;
  int num = 0;
  int other_num = 0; // dot_output for

  VisitedState();
  ~VisitedState();
};

class VisitedStates {
  std::vector<std::unique_ptr<simgrid::mc::VisitedState>> states_;
public:
  void clear() { states_.clear(); }
  std::unique_ptr<simgrid::mc::VisitedState> addVisitedState(simgrid::mc::State* graph_state, bool compare_snpashots);
private:
  void prune();
};

}
}

#endif
