/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_NOREDUCTION_HPP
#define SIMGRID_MC_NOREDUCTION_HPP

#include "simgrid/forward.h"
#include "src/mc/api/states/State.hpp"
#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/explo/reduction/Reduction.hpp"
#include "src/mc/mc_config.hpp"
#include "xbt/asserts.h"
#include <memory>

namespace simgrid::mc {

class NoReduction : public Reduction {

public:
  NoReduction()           = default;
  ~NoReduction() override = default;

  Reduction::RaceUpdate* races_computation(odpor::Execution& E, stack_t* S,
                                           std::vector<StatePtr>* opened_states) override
  {
    return new RaceUpdate();
  };

  Reduction::RaceUpdate* empty_race_update() override { return new RaceUpdate(); }
  void delete_race_update(RaceUpdate* race_update) override { delete (RaceUpdate*)race_update; }

  unsigned long apply_race_update(RemoteApp&, RaceUpdate* updates,
                                  std::vector<StatePtr>* opened_states = nullptr) override
  {
    return 0;
  }

  StatePtr state_create(RemoteApp& remote_app, StatePtr parent_state,
                        std::shared_ptr<Transition> incoming_transition) override
  {
    StatePtr state;
    if (parent_state == nullptr)
      state = new State(remote_app);
    else
      state = new State(remote_app, parent_state, incoming_transition);

    state->consider_all();

    return state;
  }

  aid_t next_to_explore(odpor::Execution& E, stack_t* S) override { return S->back()->next_transition_guided().first; }
  void on_backtrack(State* s) override{};
};

} // namespace simgrid::mc

#endif
