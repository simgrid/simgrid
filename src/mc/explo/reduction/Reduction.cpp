/* Copyright (c) 2008-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/reduction/Reduction.hpp"
#include "src/mc/api/states/SleepSetState.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/mc_forward.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_reduction, mc, "Logging specific to the reduction algorithms");

namespace simgrid::mc {

StatePtr Reduction::state_create(RemoteApp& remote_app, StatePtr parent_state)
{
  if (parent_state == nullptr)
    return StatePtr(new SleepSetState(remote_app), true);
  else
    return StatePtr(new SleepSetState(remote_app, parent_state), true);
}

void Reduction::on_backtrack(State* s)
{
  StatePtr parent = s->get_parent_state();
  if (parent == nullptr) // this is the root
    return;              // Backtracking from the root means we end exploration, nothing to do

  SleepSetState* sleep_parent = static_cast<SleepSetState*>(parent.get());
  sleep_parent->add_sleep_set(s->get_transition_in());
}

void Reduction::consider_best(StatePtr state)
{
  Exploration::get_strategy()->consider_best_in(state.get());
}

} // namespace simgrid::mc
