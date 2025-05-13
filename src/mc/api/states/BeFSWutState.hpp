/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_BeFSWUTSTATE_HPP
#define SIMGRID_MC_BeFSWUTSTATE_HPP

#include "src/mc/api/RemoteApp.hpp"
#include "src/mc/api/states/WutState.hpp"
#include "src/mc/explo/odpor/WakeupTree.hpp"
#include "src/mc/explo/odpor/odpor_forward.hpp"
#include "src/mc/mc_forward.hpp"
#include <algorithm>
#include <unordered_set>
#include <vector>

namespace simgrid::mc {

class XBT_PRIVATE BeFSWutState : public WutState {

public:
  explicit BeFSWutState(RemoteApp& remote_app);
  explicit BeFSWutState(RemoteApp& remote_app, StatePtr parent_state, std::shared_ptr<Transition> incoming_transition);
  std::unordered_set<aid_t> get_sleeping_actors(aid_t after_actor) const override;
};

} // namespace simgrid::mc

#endif
