/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/api/states/BeFSWutState.hpp"
#include "src/mc/api/RemoteApp.hpp"
#include "src/mc/api/Strategy.hpp"
#include "src/mc/api/states/WutState.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/explo/odpor/WakeupTree.hpp"
#include "src/mc/explo/odpor/odpor_forward.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_forward.hpp"
#include "src/mc/transition/Transition.hpp"
#include "xbt/asserts.h"
#include "xbt/log.h"
#include <algorithm>
#include <cassert>
#include <limits>
#include <memory>
#include <numeric>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_befswutstate, mc_state,
                                "Model-checker state dedicated to the BeFS version of ODPOR algorithm");

namespace simgrid::mc {

BeFSWutState::BeFSWutState(RemoteApp& remote_app) : WutState(remote_app) {}

BeFSWutState::BeFSWutState(RemoteApp& remote_app, StatePtr parent_state,
                           std::shared_ptr<Transition> incoming_transition)
    : WutState(remote_app, parent_state, incoming_transition, false)
{
  auto parent = static_cast<BeFSWutState*>(parent_state.get());
  for (const auto& transition : parent->opened_) {
    if (not get_transition_in()->depends(transition.get()))
      sleep_add_and_mark(transition);
  }
}

std::unordered_set<aid_t> BeFSWutState::get_sleeping_actors(aid_t after_actor) const
{
  std::unordered_set<aid_t> actors;
  for (const auto& [aid, _] : get_sleep_set()) {
    actors.insert(aid);
  }
  for (const auto& t : opened_) {
    if (t->aid_ == after_actor)
      break;
    actors.insert(t->aid_);
  }
  return actors;
}

} // namespace simgrid::mc
