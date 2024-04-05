/* Copyright (c) 2007-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/api/states/BFSWutState.hpp"
#include "src/mc/api/RemoteApp.hpp"
#include "src/mc/api/states/SleepSetState.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/explo/odpor/WakeupTree.hpp"
#include "src/mc/transition/Transition.hpp"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_bfswutstate, mc_state,
                                "Model-checker state dedicated to the BFS version of ODPOR algorithm");

namespace simgrid::mc {

BFSWutState::BFSWutState(RemoteApp& remote_app, std::shared_ptr<BFSWutState> parent_state)
    : WutState(remote_app, parent_state)
{
}

std::shared_ptr<Transition> BFSWutState::execute_next(aid_t next, RemoteApp& app)
{
  if (children_states_.find(next) != children_states_.end()) {
    outgoing_transition_ = get_actors_list().at(next).get_transition();
    return outgoing_transition_;
  }
  return State::execute_next(next, app);
}

std::unordered_set<aid_t> BFSWutState::get_sleeping_actors(aid_t after_actor) const
{
  std::unordered_set<aid_t> actors;
  for (const auto& [aid, _] : get_sleep_set()) {
    actors.insert(aid);
  }
  for (const auto& aid : done_) {
    if (aid == after_actor)
      break;
    actors.insert(aid);
  }
  return actors;
}

} // namespace simgrid::mc
