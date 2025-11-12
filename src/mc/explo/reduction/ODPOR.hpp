/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_ODPOR_HPP
#define SIMGRID_MC_ODPOR_HPP

#include "simgrid/forward.h"
#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/explo/odpor/odpor_forward.hpp"
#include "src/mc/explo/reduction/Reduction.hpp"

namespace simgrid::mc {

class ODPOR : public Reduction {

public:
  ODPOR()           = default;
  ~ODPOR() override = default;

  class RaceUpdate : public Reduction::RaceUpdate {
    std::vector<std::pair<StatePtr, odpor::PartialExecution>> state_and_seq_;

  public:
    RaceUpdate() = default;
    void add_element(StatePtr state, odpor::PartialExecution v) { state_and_seq_.emplace_back(state, v); }
    std::vector<std::pair<StatePtr, odpor::PartialExecution>> get_value() { return state_and_seq_; }
  };
  Reduction::RaceUpdate* empty_race_update() override { return new RaceUpdate(); }
  void delete_race_update(Reduction::RaceUpdate* race_update) override { delete (RaceUpdate*)race_update; }
  Reduction::RaceUpdate* races_computation(odpor::Execution& E, stack_t* S,
                                           std::vector<StatePtr>* opened_states) override;
  unsigned long apply_race_update(RemoteApp& remote_app, Reduction::RaceUpdate* updates,
                                  std::vector<StatePtr>* opened_states = nullptr) override;
  aid_t next_to_explore(odpor::Execution& E, stack_t* S) override;
  StatePtr state_create(RemoteApp& remote_app, StatePtr parent_state,
                        std::shared_ptr<Transition> incoming_transition) override;
  void on_backtrack(State* s) override;
  void consider_best(StatePtr state) override;
};

} // namespace simgrid::mc

#endif
