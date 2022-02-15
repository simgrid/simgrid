/* Copyright (c) 2016-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/VisitedState.hpp"
#include "src/mc/checker/Checker.hpp"

#include <string>
#include <vector>

#ifndef SIMGRID_MC_COMMUNICATION_DETERMINISM_CHECKER_HPP
#define SIMGRID_MC_COMMUNICATION_DETERMINISM_CHECKER_HPP

namespace simgrid {
namespace mc {

class XBT_PRIVATE CommunicationDeterminismChecker : public Checker {
public:
  explicit CommunicationDeterminismChecker(Session* session);
  ~CommunicationDeterminismChecker() override;
  void run() override;
  RecordTrace get_record_trace() override;
  std::vector<std::string> get_textual_trace() override;

private:
  void prepare();
  void real_run();
  void log_state() override;
  void restoreState();

  /** Stack representing the position in the exploration graph */
  std::list<std::unique_ptr<State>> stack_;
  VisitedStates visited_states_;
};
} // namespace mc
} // namespace simgrid

#endif

