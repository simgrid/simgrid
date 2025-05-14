/* Copyright (c) 2008-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/reduction/Reduction.hpp"
#include "src/mc/api/states/SleepSetState.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/mc_forward.hpp"
#include "xbt/asserts.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_reduction, mc, "Logging specific to the reduction algorithms");

namespace simgrid::mc {

StatePtr Reduction::state_create(RemoteApp& remote_app, StatePtr parent_state,
                                 std::shared_ptr<Transition> incoming_transition)
{
  if (parent_state == nullptr)
    return StatePtr(new SleepSetState(remote_app), true);
  else {
    auto new_state = StatePtr(new SleepSetState(remote_app, parent_state, incoming_transition), true);
    parent_state->record_child_state(new_state);

    return new_state;
  }
}

void Reduction::on_backtrack(State* s)
{
  
}

void Reduction::consider_best(StatePtr state)
{
  Exploration::get_strategy()->consider_best_in(state.get());
}

} // namespace simgrid::mc
