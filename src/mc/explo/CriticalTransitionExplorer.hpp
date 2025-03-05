/* Copyright (c) 2008-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_CRITICAL_FINDER_HPP
#define SIMGRID_MC_CRITICAL_FINDER_HPP

#include "src/mc/api/ClockVector.hpp"
#include "src/mc/api/states/State.hpp"
#include "src/mc/explo/DFSExplorer.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/explo/reduction/DPOR.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_forward.hpp"

#include <deque>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace simgrid::mc {

using EventHandle = uint32_t;

class XBT_PRIVATE CriticalTransitionExplorer : public DFSExplorer {
private:
  // The first non-correct execution found. We must record the state (in order to re-explore them later) AND
  // the corresponding out-transition (since this can change during later explorations).
  std::deque<std::pair<StatePtr, std::shared_ptr<Transition>>> initial_bugged_stack = {};

  // Display the initial bugged stacked and update information to track where the current critical transition might be
  void log_stack();

  // Display information about the exploration after it ended
  void log_end_exploration();

public:
  explicit CriticalTransitionExplorer(std::unique_ptr<RemoteApp> remote_app, ReductionMode mode, stack_t* stack);
  void run() override;
};

} // namespace simgrid::mc

#endif
