/* Copyright (c) 2008-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SAFETY_CHECKER_HPP
#define SIMGRID_MC_SAFETY_CHECKER_HPP

#include "src/mc/VisitedState.hpp"
#include "src/mc/checker/Checker.hpp"
#include "src/mc/mc_safety.hpp"

#include <list>
#include <memory>
#include <string>
#include <vector>

namespace simgrid {
namespace mc {

class XBT_PRIVATE SafetyChecker : public Checker {
  ReductionMode reductionMode_ = ReductionMode::unset;
  long backtrack_count_        = 0;

public:
  explicit SafetyChecker(Session* session);
  void run() override;
  RecordTrace get_record_trace() override;
  std::vector<std::string> get_textual_trace() override;
  void log_state() override;

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
