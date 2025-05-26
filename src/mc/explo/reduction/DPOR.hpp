/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_DPOR_HPP
#define SIMGRID_MC_DPOR_HPP

#include "simgrid/forward.h"
#include "src/mc/api/states/SleepSetState.hpp"
#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/explo/reduction/Reduction.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_forward.hpp"
#include "xbt/asserts.h"
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

namespace simgrid::mc {

class DPOR : public Reduction {

  /** Compute the eventual i of Godefroid algorithm, line 4
   *  Note that with persistency, we do not consider every p in "advance" but only the lastly taken p */
  std::optional<EventHandle> max_dependent_dpor(const odpor::Execution& S, const State* s, aid_t p);

  std::unordered_set<aid_t> compute_ancestors(const odpor::Execution& S, stack_t* state_stack, aid_t p, EventHandle i);

public:
  DPOR()           = default;
  ~DPOR() override = default;

  class RaceUpdate : public Reduction::RaceUpdate {
    std::vector<std::pair<StatePtr, std::unordered_set<aid_t>>> state_and_ancestors_ =
        std::vector<std::pair<StatePtr, std::unordered_set<aid_t>>>();

  public:
    RaceUpdate() = default;

    void add_element(StatePtr state, std::unordered_set<aid_t> ancestors)
    {
      state_and_ancestors_.emplace_back(state, ancestors);
    }
    std::vector<std::pair<StatePtr, std::unordered_set<aid_t>>>& get_value() { return state_and_ancestors_; }
  };

  Reduction::RaceUpdate* empty_race_update() override { return new RaceUpdate(); }

  void delete_race_update(Reduction::RaceUpdate* race_update) override { delete (RaceUpdate*)race_update; }

  Reduction::RaceUpdate* races_computation(odpor::Execution& E, stack_t* S,
                                           std::vector<StatePtr>* opened_states) override;

  unsigned long apply_race_update(RemoteApp&, Reduction::RaceUpdate* updates,
                                  std::vector<StatePtr>* opened_states = nullptr) override;

  StatePtr state_create(RemoteApp& remote_app, StatePtr parent_state,
                        std::shared_ptr<Transition> incoming_transition) override;

  aid_t next_to_explore(odpor::Execution& E, stack_t* S) override;
};

} // namespace simgrid::mc

#endif
