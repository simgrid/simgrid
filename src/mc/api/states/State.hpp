/* Copyright (c) 2007-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_STATE_HPP
#define SIMGRID_MC_STATE_HPP

#include "src/mc/api/ActorState.hpp"
#include "src/mc/api/ClockVector.hpp"
#include "src/mc/api/RemoteApp.hpp"
#include "src/mc/api/strategy/StratLocalInfo.hpp"
#include "src/mc/transition/Transition.hpp"

namespace simgrid::mc {

/* A node in the exploration graph (kind-of) */
class XBT_PRIVATE State : public xbt::Extendable<State> {
  // Support for the StatePtr datatype, aka boost::intrusive_ptr<State>
  std::atomic_int_fast32_t refcount_{0};
  friend XBT_PUBLIC void intrusive_ptr_add_ref(State* activity);
  friend XBT_PUBLIC void intrusive_ptr_release(State* activity);

  static long expended_states_; /* Count total amount of states, for stats */

  /** A forked application stationned in this state, to quickly recreate child states w/o replaying from the beginning
   */
  std::unique_ptr<CheckerSide> state_factory_ = nullptr;

  /** @brief The incoming transition is what led to this state, coming from its parent  */
  std::shared_ptr<Transition> incoming_transition_ = nullptr;

  /** Sequential state ID (used for debugging) */
  long num_ = 0;

  /** Unique parent of this state */
  StatePtr parent_state_ = nullptr;

  /** @brief Wether this state can lead to a correct execution.
   *  Used by the critical transisition algorithm */
  bool has_correct_descendent_ = false;

protected:
  std::shared_ptr<StratLocalInfo> strategy_;

  /** @brief The outgoing transition is the last transition that we took to leave this state.  */
  std::shared_ptr<Transition> outgoing_transition_ = nullptr;

public:
  explicit State(const RemoteApp& remote_app);
  explicit State(const RemoteApp& remote_app, StatePtr parent_state);
  virtual ~State() = default;
  /* Returns a positive number if there is another transition to pick, or -1 if not */
  aid_t next_transition() const; // this function should disapear as it is redundant with the next one

  /* Same as next_transition(), but choice is now guided, and an integer corresponding to the
   internal cost of the transition is returned */
  virtual std::pair<aid_t, int> next_transition_guided() const;

  int get_actor_strategy_valuation(aid_t actor) const { return strategy_->get_actor_valuation(actor); }

  /**
   * @brief Explore a new path on the remote app; the parameter 'next' must be the result of a previous call to
   * next_transition()
   *
   * Some algorithm may sometimes require to not really explore a new state; hence the real behavior is dependent
   * of the currently used state and strategy
   */
  virtual std::shared_ptr<Transition> execute_next(aid_t next, RemoteApp& app);

  long get_num() const { return num_; }
  std::size_t count_todo() const;

  virtual bool has_more_to_be_explored() const;

  /* Marking as TODO some actor in this state:
   *  + consider_one mark aid actor (and assert it is possible)
   *  + consider_best ensure one actor is marked by eventually marking the best regarding its guiding method
   *  + consider_all mark all enabled actor that are not done yet */
  void consider_one(aid_t aid) const { strategy_->consider_one(aid); }
  void consider_best() const { strategy_->consider_best(); }
  void ensure_one_considered_among_set(std::unordered_set<aid_t> E) { strategy_->ensure_one_considered_among_set(E); }
  unsigned long consider_all() const { return strategy_->consider_all(); }

  bool is_actor_done(aid_t actor) const { return strategy_->actors_to_run_.at(actor).is_done(); }
  std::shared_ptr<Transition> get_transition_out() const { return outgoing_transition_; }
  std::shared_ptr<Transition> get_transition_in() const { return incoming_transition_; }
  State* get_parent_state() const { return parent_state_.get(); }

  std::map<aid_t, ActorState> const& get_actors_list() const { return strategy_->actors_to_run_; }

  unsigned long get_actor_count() const { return strategy_->actors_to_run_.size(); }
  bool is_actor_enabled(aid_t actor) const { return strategy_->actors_to_run_.at(actor).is_enabled(); }

  /** Returns whether this state has a state factory.
   *
   *  The idea is to sometimes fork the application before taking a transition so that we can restore this state very
   *  quickly in the future.
   *
   *  Of course, that's very memory hungry but this is meant to be a rare event, and it's subject to future
   *  optimizations (to remove some forks when they become useless).
   */
  bool has_state_factory() { return state_factory_ != nullptr; }
  void set_state_factory(std::unique_ptr<simgrid::mc::CheckerSide> checkerside)
  {
    state_factory_ = std::move(checkerside);
  }
  simgrid::mc::CheckerSide* get_state_factory() { return state_factory_.get(); }

  /**
   * @brief Computes the backtrack set for this state according to its definition in SimGrid.
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
  std::unordered_set<aid_t> get_enabled_actors() const;
  std::vector<aid_t> get_batrack_minus_done() const;

  /* Returns the total amount of states created so far (for statistics) */
  static long get_expanded_states() { return expended_states_; }

  /**
   * @brief Register this state as leading to a correct execution and
   * does the same for all its predecessors
   */
  void register_as_correct();

  bool has_correct_execution() { return has_correct_descendent_; }
};
} // namespace simgrid::mc

#endif
