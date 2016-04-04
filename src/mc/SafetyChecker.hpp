/* Copyright (c) 2008-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SAFETY_CHECKER_HPP
#define SIMGRID_MC_SAFETY_CHECKER_HPP

#include <list>

#include "src/mc/mc_forward.hpp"
#include "src/mc/Checker.hpp"

namespace simgrid {
namespace mc {

class SafetyChecker : public Checker {
  simgrid::mc::ReductionMode reductionMode_ = simgrid::mc::ReductionMode::unset;
public:
  SafetyChecker(Session& session);
  ~SafetyChecker();
  int run() override;
  RecordTrace getRecordTrace() override;
  std::vector<std::string> getTextualTrace() override;
private:
  // Temp
  void init();
  bool checkNonDeterminism(simgrid::mc::State* current_state);
private:
  /** Stack representing the position in the exploration graph */
  std::list<simgrid::mc::State*> stack_;
};

}
}

#endif
