/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_NOREDUCTION_HPP
#define SIMGRID_MC_NOREDUCTION_HPP

#include "simgrid/forward.h"
#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/explo/reduction/Reduction.hpp"
#include "src/mc/mc_config.hpp"
#include "xbt/asserts.h"

namespace simgrid::mc {

class NoReduction : public Reduction {

public:
  NoReduction()           = default;
  ~NoReduction() override = default;

  void races_computation(odpor::Execution E, stack_t* S, std::vector<std::shared_ptr<State>>* opened_states) override{};
  bool has_to_be_explored(odpor::Execution E, stack_t* S) override
  {
    S->back()->consider_all();
    return S->back()->count_todo_multiples() > 0;
  }
  aid_t next_to_explore(odpor::Execution E, stack_t* S) override { return S->back()->next_transition_guided().first; }
  void on_backtrack(State* s) override{};
};

} // namespace simgrid::mc

#endif
