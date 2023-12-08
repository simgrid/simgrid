/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/reduction/Reduction.hpp"
#include "src/mc/api/states/SleepSetState.hpp"
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_reduction, mc, "Logging specific to the reduction algorithms");

namespace simgrid::mc {

std::shared_ptr<State> Reduction::state_create(RemoteApp& remote_app, std::shared_ptr<State> parent_state)
{
  if (parent_state == nullptr)
    return std::make_shared<SleepSetState>(remote_app);
  else {
    std::shared_ptr<SleepSetState> sleep_state = std::static_pointer_cast<SleepSetState>(parent_state);
    xbt_assert(sleep_state != nullptr, "Wrong kind of state for this reduction. This shouldn't happen, fix me");
    return std::make_shared<SleepSetState>(remote_app, sleep_state);
  }
}

void Reduction::on_backtrack(State* s)
{
  std::shared_ptr<State> parent = s->get_parent_state();
  if (parent == nullptr) // this is the root
    return;              // Backtracking from the root means we end exploration, nothing to do

  std::shared_ptr<SleepSetState> sleep_parent = std::static_pointer_cast<SleepSetState>(parent);
  xbt_assert(sleep_parent != nullptr, "Wrong kind of state for this reduction. This shouldn't happen, fix me");
  sleep_parent->add_sleep_set(s->get_transition_in());
}
} // namespace simgrid::mc
