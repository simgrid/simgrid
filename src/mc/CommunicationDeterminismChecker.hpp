/* Copyright (c) 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <list>

#include "src/mc/mc_forward.hpp"
#include "src/mc/Checker.hpp"
#include "src/mc/VisitedState.hpp"

#ifndef SIMGRID_MC_COMMUNICATION_DETERMINISM_CHECKER_HPP
#define SIMGRID_MC_COMMUNICATION_DETERMINISM_CHECKER_HPP

namespace simgrid {
namespace mc {

class CommunicationDeterminismChecker : public Checker {
public:
  CommunicationDeterminismChecker(Session& session);
  ~CommunicationDeterminismChecker();
  int run() override;
  RecordTrace getRecordTrace() override;
  std::vector<std::string> getTextualTrace() override;
private:
  void prepare();
  int main();
private:
  /** Stack representing the position in the exploration graph */
  std::list<simgrid::mc::State*> stack_;
  simgrid::mc::VisitedStates visitedStates_;
};

#endif

}
}
