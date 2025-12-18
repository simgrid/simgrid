/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_WUTSTATE_HPP
#define SIMGRID_MC_WUTSTATE_HPP

#include "src/mc/api/RemoteApp.hpp"
#include "src/mc/api/states/SleepSetState.hpp"
#include "src/mc/explo/odpor/WakeupTree.hpp"
#include "src/mc/explo/odpor/odpor_forward.hpp"
#include "src/mc/mc_forward.hpp"
#include <memory>
#include <mutex>
#include <vector>

namespace simgrid::mc {

class XBT_PRIVATE WutState : public SleepSetState {

  // A vector containing the sequence that couldn't be inserted for now
  //   this is used in parallel execution when this state is not owned by the Tree Handler
  std::vector<odpor::PartialExecution> to_be_inserted_;

  bool owned_by_the_explorers_ = false;

public:
  explicit WutState(RemoteApp& remote_app) : SleepSetState(remote_app) {}
  explicit WutState(RemoteApp& remote_app, StatePtr parent_state, TransitionPtr incoming_transition,
                    bool set_actor_status = true)
      : SleepSetState(remote_app, parent_state, incoming_transition, set_actor_status)
  {
  }

  /**
   * @brief
   */
  StatePtr insert_into_tree(odpor::PartialExecution&, RemoteApp&);

  unsigned int direct_children() const { return opened_.size() - closed_.size(); }

  bool has_more_to_be_explored() const override { return direct_children() > 0; }

  std::unordered_set<aid_t> get_sleeping_actors(aid_t after_actor) const override;

  void give_ownership_to_explorers() { owned_by_the_explorers_ = true; }

  void on_branch_completion() override;
};

} // namespace simgrid::mc

#endif
