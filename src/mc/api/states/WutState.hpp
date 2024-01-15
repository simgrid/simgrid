/* Copyright (c) 2007-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_WUTSTATE_HPP
#define SIMGRID_MC_WUTSTATE_HPP

#include "src/mc/api/RemoteApp.hpp"
#include "src/mc/api/states/SleepSetState.hpp"
#include "src/mc/explo/odpor/WakeupTree.hpp"
#include "xbt/log.h"

namespace simgrid::mc {

class XBT_PRIVATE WutState : public SleepSetState {

  /**
   * The wakeup tree with respect to the execution represented
   * by the totality of all states before and including this one
   * and with respect to this state's sleep set
   */
  odpor::WakeupTree wakeup_tree_;
  bool has_initialized_wakeup_tree = false;

  /** Unique parent of this state. Required both for sleep set computation
      and for guided model-checking */
  std::shared_ptr<WutState> parent_state_ = nullptr;

  void initialize_if_empty_wut();

public:
  explicit WutState(RemoteApp& remote_app);
  explicit WutState(RemoteApp& remote_app, std::shared_ptr<WutState> parent_state);

  /**
   * Same as next_transition(), but the choice is not based off the ODPOR
   * wakeup tree associated with this state
   */
  aid_t next_odpor_transition() const;

  /**
   * @brief Inserts an arbitrary enabled actor into the wakeup tree
   * associated with this state, if such an actor exists and if
   * the wakeup tree is already not empty
   *
   * @param prior The sequence of steps leading up to this state
   * with respec to which the tree associated with this state should be
   * a wakeup tree (wakeup trees are defined relative to an execution)
   *
   * @invariant: You should not manipulate a wakeup tree with respect
   * to more than one execution; doing so will almost certainly lead to
   * unexpected results as wakeup trees are defined relative to a single
   * execution
   */
  void seed_wakeup_tree_if_needed(const odpor::Execution& prior);

  /**
   * @brief Initializes the wakeup_tree_ instance by taking the subtree rooted at the
   * single-process node `N` running actor `p := "actor taken by parent to form this state"`
   * of the *parent's* wakeup tree
   */
  void sprout_tree_from_parent_state();

  /**
   * @brief Removes the subtree rooted at the single-process node
   * `N` running actor `p` of this state's wakeup tree
   */
  void remove_subtree_using_current_out_transition();
  void remove_subtree_at_aid(aid_t proc);
  bool has_empty_tree() const { return this->wakeup_tree_.empty(); }
  std::string string_of_wut() const { return this->wakeup_tree_.string_of_whole_tree(); }

  /**
   * @brief
   */
  odpor::WakeupTree::InsertionResult insert_into_wakeup_tree(const odpor::PartialExecution&, const odpor::Execution&);

  /** @brief Prepares the state for re-exploration following
   * another after having followed ODPOR from this state with
   * the current out transition
   *
   * After ODPOR has completed searching a maximal trace, it
   * finds the first point in the execution with a nonempty wakeup
   * tree. This method corresponds to lines 20 and 21 in the ODPOR
   * pseudocode
   */
  void do_odpor_unwind();
};

} // namespace simgrid::mc

#endif
