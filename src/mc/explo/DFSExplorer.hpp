/* Copyright (c) 2008-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SAFETY_CHECKER_HPP
#define SIMGRID_MC_SAFETY_CHECKER_HPP

#include "src/mc/api/ClockVector.hpp"
#include "src/mc/api/states/State.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/explo/reduction/DPOR.hpp"
#include "src/mc/mc_config.hpp"

#include <deque>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace simgrid::mc {

using EventHandle = uint32_t;

class XBT_PRIVATE DFSExplorer : public Exploration {
protected:
  std::unique_ptr<Reduction> reduction_algo_;
  ReductionMode reduction_mode_;
  stack_t* stack_;
  bool is_execution_descending = true;

private:
  // For statistics. Starts at one because we only track the act of starting a new trace
  unsigned long explored_traces_ = 0;

public:
  // Used for the critical transition explorer
  explicit DFSExplorer(std::unique_ptr<RemoteApp> remote_app, ReductionMode mode);

  explicit DFSExplorer(const std::vector<char*>& args, ReductionMode mode);
  void run() override;
  RecordTrace get_record_trace() override;
  void log_state() override;
  stack_t get_stack() override { return *stack_; }

protected:
  /// Recursively explore the transtions provided by the reduction
  void explore(odpor::Execution& S, stack_t& state_stack);
  /// Do one step in the exploration: execute a transition, create a state and deal with failures
  void step_exploration(odpor::Execution& S, aid_t next_actor, stack_t& state_stack);
};

} // namespace simgrid::mc

#endif
