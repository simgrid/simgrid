/* Copyright (c) 2007-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_BFSWUTSTATE_HPP
#define SIMGRID_MC_BFSWUTSTATE_HPP

#include "src/mc/api/RemoteApp.hpp"
#include "src/mc/api/states/WutState.hpp"
#include "src/mc/explo/odpor/WakeupTree.hpp"
#include "src/mc/explo/odpor/odpor_forward.hpp"
#include "xbt/log.h"
#include <map>
#include <memory>
#include <unordered_set>
#include <vector>

namespace simgrid::mc {

class XBT_PRIVATE BFSWutState : public WutState {
  /** Unique parent of this state. Required both for sleep set computation
      and for guided model-checking */
  std::shared_ptr<BFSWutState> parent_state_ = nullptr;

  std::map<aid_t, std::shared_ptr<BFSWutState>> children_states_;

  /** This wakeup tree is used to store the whole tree explored during the process,
      it is only growing. There could be an optimization where we store only one wakeup
      tree and color the subtree that correponds to the current WuT. */
  odpor::WakeupTree final_wakeup_tree_;

  /** Store the aid that have been visited at least once. This is usefull both to know what not to
   *  revisit, but also to remember the order in which the children were visited. The latter information
   *  being important for the correction. */
  std::vector<aid_t> done_;

public:
  explicit BFSWutState(RemoteApp& remote_app);
  explicit BFSWutState(RemoteApp& remote_app, std::shared_ptr<BFSWutState> parent_state);

  void record_child_state(std::shared_ptr<BFSWutState> child);

  aid_t next_transition() const;

  odpor::PartialExecution insert_into_final_wakeup_tree(const odpor::PartialExecution&);

  std::pair<aid_t, int> next_transition_guided() const override;

  /**
   * @brief Explore a new path on the remote app; the parameter 'next' must be the result of a previous call to
   * next_to_explore()
   *
   * In practice, if the child resulting of the execution of next has already been visited, we do not create a new
   * transition object, nor ask the remote app.
   */
  std::shared_ptr<Transition> execute_next(aid_t next, RemoteApp& app) override;

  std::shared_ptr<BFSWutState> get_children_state_of_aid(aid_t next)
  {
    if (auto children_state = children_states_.find(next); children_state != children_states_.end())
      return children_state->second;
    return nullptr;
  }

  /**
   * @brief Take the Wakeup tree from the parent and extract the correponding subtree in this state.
   */
  void unwind_wakeup_tree_from_parent();

  std::unordered_set<aid_t> get_sleeping_actors(aid_t after_actor) const override;
  std::string get_string_of_final_wut() const { return final_wakeup_tree_.string_of_whole_tree(); }
};

} // namespace simgrid::mc

#endif
