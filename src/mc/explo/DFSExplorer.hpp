/* Copyright (c) 2008-2024. The SimGrid Team. All rights reserved.          */

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
  static xbt::signal<void(RemoteApp&)> on_exploration_start_signal;

  static xbt::signal<void(State*, RemoteApp&)> on_state_creation_signal;

  static xbt::signal<void(Transition*, RemoteApp&)> on_transition_execute_signal;

  static xbt::signal<void(RemoteApp&)> on_log_state_signal;

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

  /** Called once when the exploration starts */
  static void on_exploration_start(std::function<void(RemoteApp& remote_app)> const& f)
  {
    on_exploration_start_signal.connect(f);
  }
  /** Called each time that a new state is create */
  static void on_state_creation(std::function<void(State*, RemoteApp& remote_app)> const& f)
  {
    on_state_creation_signal.connect(f);
  }
  /** Called when executing a new transition */
  static void on_transition_execute(std::function<void(Transition*, RemoteApp& remote_app)> const& f)
  {
    on_transition_execute_signal.connect(f);
  }
  /** Called when displaying the statistics at the end of the exploration */
  static void on_log_state(std::function<void(RemoteApp&)> const& f) { on_log_state_signal.connect(f); }

protected:
  void explore(odpor::Execution& S, stack_t& state_stack);
  void simgrid_wrapper_explore(odpor::Execution& S, aid_t next_actor, stack_t& state_stack);
};

} // namespace simgrid::mc

#endif
