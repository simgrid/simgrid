/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_STATE_HPP
#define SIMGRID_MC_STATE_HPP

#include "src/mc/api/ActorState.hpp"
#include "src/mc/api/ClockVector.hpp"
#include "src/mc/api/RemoteApp.hpp"
#include "src/mc/api/strategy/Strategy.hpp"
#include "src/mc/explo/odpor/WakeupTree.hpp"
#include "src/mc/transition/Transition.hpp"

namespace simgrid::mc {

/* A node in the exploration graph (kind-of) */
class XBT_PRIVATE State : public xbt::Extendable<State> {
  static long expended_states_; /* Count total amount of states, for stats */

  /** @brief The outgoing transition is the last transition that we took to leave this state.  */
  std::shared_ptr<Transition> outgoing_transition_ = nullptr;

  /** @brief The incoming transition is what led to this state, coming from its parent  */
  std::shared_ptr<Transition> incoming_transition_ = nullptr;

  /** Sequential state ID (used for debugging) */
  long num_ = 0;

  /** Unique parent of this state. Required both for sleep set computation
      and for guided model-checking */
  std::shared_ptr<State> parent_state_ = nullptr;

  std::shared_ptr<Strategy> strategy_;

  /* Sleep sets are composed of the actor and the corresponding transition that made it being added to the sleep
   * set. With this information, it is check whether it should be removed from it or not when exploring a new
   * transition */
  std::map<aid_t, std::shared_ptr<Transition>> sleep_set_;

  /**
   * The wakeup tree with respect to the execution represented
   * by the totality of all states before and including this one
   * and with respect to this state's sleep set
   */
  odpor::WakeupTree wakeup_tree_;
  bool has_initialized_wakeup_tree = false;

public:
  explicit State(RemoteApp& remote_app);
  explicit State(RemoteApp& remote_app, std::shared_ptr<State> parent_state);
  /* Returns a positive number if there is another transition to pick, or -1 if not */
  aid_t next_transition() const; // this function should disapear as it is redundant with the next one

  /* Same as next_transition(), but choice is now guided, and an integer corresponding to the
   internal cost of the transition is returned */
  std::pair<aid_t, int> next_transition_guided() const;

  /**
   * Same as next_transition(), but the choice is not based off the ODPOR
   * wakeup tree associated with this state
   */
  aid_t next_odpor_transition() const;

  /**
   * @brief Explore a new path on the remote app; the parameter 'next' must be the result of a previous call to
   * next_transition()
   */
  std::shared_ptr<Transition> execute_next(aid_t next, RemoteApp& app);

  long get_num() const { return num_; }
  std::size_t count_todo() const;
  std::size_t count_todo_multiples() const;

  /* Marking as TODO some actor in this state:
   *  + consider_one mark aid actor (and assert it is possible)
   *  + consider_best ensure one actor is marked by eventually marking the best regarding its guiding method
   *  + consider_all mark all enabled actor that are not done yet */
  void consider_one(aid_t aid) const { strategy_->consider_one(aid); }
  void consider_best() const { strategy_->consider_best(); }
  unsigned long consider_all() const { return strategy_->consider_all(); }

  bool is_actor_done(aid_t actor) const { return strategy_->actors_to_run_.at(actor).is_done(); }
  std::shared_ptr<Transition> get_transition_out() const { return outgoing_transition_; }
  std::shared_ptr<Transition> get_transition_in() const { return incoming_transition_; }
  std::shared_ptr<State> get_parent_state() const { return parent_state_; }

  std::map<aid_t, ActorState> const& get_actors_list() const { return strategy_->actors_to_run_; }

  unsigned long get_actor_count() const { return strategy_->actors_to_run_.size(); }
  bool is_actor_enabled(aid_t actor) const { return strategy_->actors_to_run_.at(actor).is_enabled(); }

  /**
   * @brief Computes the backtrack set for this state
   * according to its definition in SimGrid.
   *
   * The backtrack set as it appears in DPOR, SDPOR, and ODPOR
   * in SimGrid consists of those actors marked as `todo`
   * (i.e. those that have yet to be explored) as well as those
   * marked `done` (i.e. those that have already been explored)
   * since the pseudocode in none of the above algorithms explicitly
   * removes elements from the backtrack set. DPOR makes use
   * explicitly of the `done` set, but we again note that the
   * backtrack set still contains processes added to the done set.
   */
  std::unordered_set<aid_t> get_backtrack_set() const;
  std::unordered_set<aid_t> get_sleeping_actors() const;
  std::unordered_set<aid_t> get_enabled_actors() const;
  std::map<aid_t, std::shared_ptr<Transition>> const& get_sleep_set() const { return sleep_set_; }
  void add_sleep_set(std::shared_ptr<Transition> t) { sleep_set_.insert_or_assign(t->aid_, std::move(t)); }
  bool is_actor_sleeping(aid_t actor) const
  {
    return std::find_if(sleep_set_.begin(), sleep_set_.end(), [=](const auto& pair) { return pair.first == actor; }) !=
           sleep_set_.end();
  }

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

  /* Returns the total amount of states created so far (for statistics) */
  static long get_expanded_states() { return expended_states_; }
};
} // namespace simgrid::mc

#endif
