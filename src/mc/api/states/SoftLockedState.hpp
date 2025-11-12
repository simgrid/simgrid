/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SOFTLOCKEDSTATE_HPP
#define SIMGRID_MC_SOFTLOCKEDSTATE_HPP

#include "src/mc/api/RemoteApp.hpp"
#include "src/mc/api/states/State.hpp"

namespace simgrid::mc {

class XBT_PRIVATE SoftLockedState : public State {
protected:
public:
  explicit SoftLockedState(RemoteApp& remote_app, StatePtr parent_state,
                           std::shared_ptr<Transition> incoming_transition)
      : State(remote_app, parent_state, incoming_transition)
  {
  }

  std::unordered_set<aid_t> get_enabled_actors() const override { return std::unordered_set<aid_t>(); }
};

} // namespace simgrid::mc

#endif
