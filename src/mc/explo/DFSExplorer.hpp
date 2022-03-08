/* Copyright (c) 2008-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SAFETY_CHECKER_HPP
#define SIMGRID_MC_SAFETY_CHECKER_HPP

#include "src/mc/VisitedState.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/mc_safety.hpp"

#include <list>
#include <memory>
#include <string>
#include <vector>

namespace simgrid {
namespace mc {

class XBT_PRIVATE DFSExplorer : public Exploration {
  ReductionMode reductionMode_ = ReductionMode::unset;
  long backtrack_count_        = 0;

  static xbt::signal<void()> on_exploration_start_signal;
  static xbt::signal<void()> on_backtracking_signal;

  static xbt::signal<void(State*)> on_state_creation_signal;

  static xbt::signal<void(State*)> on_restore_system_state_signal;
  static xbt::signal<void()> on_restore_initial_state_signal;
  static xbt::signal<void(Transition*)> on_transition_replay_signal;
  static xbt::signal<void(Transition*)> on_transition_execute_signal;

  static xbt::signal<void()> on_log_state_signal;

public:
  explicit DFSExplorer(Session* session);
  void run() override;
  RecordTrace get_record_trace() override;
  std::vector<std::string> get_textual_trace() override;
  void log_state() override;

  /** Called once when the exploration starts */
  static void on_exploration_start(std::function<void()> const& f) { on_exploration_start_signal.connect(f); }
  /** Called each time that the exploration backtracks from a exploration end */
  static void on_backtracking(std::function<void()> const& f) { on_backtracking_signal.connect(f); }
  /** Called each time that a new state is create */
  static void on_state_creation(std::function<void(State*)> const& f) { on_state_creation_signal.connect(f); }
  /** Called when rollbacking directly onto a previously checkpointed state */
  static void on_restore_system_state(std::function<void(State*)> const& f)
  {
    on_restore_system_state_signal.connect(f);
  }
  /** Called when the state to which we backtrack was not checkpointed state, forcing us to restore the initial state
   * before replaying some transitions */
  static void on_restore_initial_state(std::function<void()> const& f) { on_restore_initial_state_signal.connect(f); }
  /** Called when replaying a transition that was previously executed, to reach a backtracked state */
  static void on_transition_replay(std::function<void(Transition*)> const& f)
  {
    on_transition_replay_signal.connect(f);
  }
  /** Called when executing a new transition */
  static void on_transition_execute(std::function<void(Transition*)> const& f)
  {
    on_transition_execute_signal.connect(f);
  }
  /** Called when displaying the statistics at the end of the exploration */
  static void on_log_state(std::function<void()> const& f) { on_log_state_signal.connect(f); }

private:
  void check_non_termination(const State* current_state);
  void backtrack();
  void restore_state();

  /** Stack representing the position in the exploration graph */
  std::list<std::unique_ptr<State>> stack_;
  VisitedStates visited_states_;
  std::unique_ptr<VisitedState> visited_state_;
};

} // namespace mc
} // namespace simgrid

#endif
