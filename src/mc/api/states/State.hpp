/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_STATE_HPP
#define SIMGRID_MC_STATE_HPP

#include "src/mc/api/ActorState.hpp"
#include "src/mc/api/RemoteApp.hpp"
#include "src/mc/transition/Transition.hpp"
#include "src/mc/xbt_intrusiveptr.hpp"
#include "xbt/asserts.h"
#include <memory>
#include <vector>

namespace simgrid::mc {

/* A node in the exploration graph (kind-of) */
class XBT_PUBLIC State : public xbt::Extendable<State> {
  // Support for the StatePtr datatype, aka boost::intrusive_ptr<State>
  std::atomic_int_fast32_t refcount_{0};
  friend XBT_PUBLIC void intrusive_ptr_add_ref(State* activity);
  friend XBT_PUBLIC void intrusive_ptr_release(State* activity);

  static long expended_states_; /* Count total amount of states, for stats */

  static long in_memory_states_; // Count the number of states currently still in memory

  /** A forked application stationned in this state, to quickly recreate child states w/o replaying from the beginning
   */
  std::unique_ptr<CheckerSide> state_factory_ = nullptr;

  /** @brief The incoming transition is what led to this state, coming from its parent  */
  std::shared_ptr<Transition> incoming_transition_ = nullptr;

  /** @brief The outgoing transition is the last transition that we took to leave this state.  */
  std::shared_ptr<Transition> outgoing_transition_ = nullptr;

  /** Sequential state ID (used for debugging) */
  long num_ = 0;

  /** Depth of this state in the tree. Used for DFS-like strategy in BeFS algorithm */
  unsigned long depth_ = 0;

  /** Unique parent of this state */
  StatePtr parent_state_ = nullptr;

  /** @brief Wether this state can lead to a correct execution.
   *  Used by the critical transition algorithm */
  bool has_correct_descendent_ = false;

  /** @brief Add transition to the opened_ one in this state.
   *  If we find a placeholder for transition, replace it. Else simply add it. */
  void update_opened(std::shared_ptr<Transition> transition);

protected:
  /** State's exploration status by actor. All actors should be present, eventually disabled for now. */
  std::map<aid_t, ActorState> actors_to_run_;
  bool actor_status_set_ = false;

  ActorState get_actor_at(aid_t aid) { return actors_to_run_.at(aid); }

  std::vector<std::vector<StatePtr>> children_states_; // first key is aid, second time considered

  /** Store the aid that have been visited at least once. This is usefull both to know what not to
   *  revisit, but also to remember the order in which the children were visited. The latter information
   *  being important for the correction. */
  std::vector<std::shared_ptr<Transition>> opened_;

  size_t get_opened_size()
  {
    return std::count_if(opened_.begin(), opened_.end(), [](auto const& ptr_t) { return ptr_t != nullptr; });
  }

  /** Only leftmosts states of the tree can be closed. This is decided on creation based on parent
   *  value, and then updated when nearby states are closed. */
  bool is_leftmost_;

  /** Store the aid that have been closed. This is usefull to determine wether a given state is leftmost. */
  std::vector<aid_t> closed_;

public:
  explicit State(const RemoteApp& remote_app, bool set_actor_status = true);
  explicit State(const RemoteApp& remote_app, StatePtr parent_state, std::shared_ptr<Transition> incoming_transition,
                 bool set_actor_status = true);
  virtual ~State();

  bool has_been_initialized() const { return actor_status_set_; }
  void initialize(const RemoteApp& remote_app);
  void update_incoming_transition_with_remote_app(const RemoteApp& remote_app, aid_t aid, int times_considered);
  void update_incoming_transition_explicitly(std::shared_ptr<Transition> incoming_transition)
  {
    incoming_transition_ = incoming_transition;
  }

  int get_ref_count() { return refcount_; }
  /* Returns a positive number if there is another transition to pick, or -1 if not */
  aid_t next_transition() const; // this function should disapear as it is redundant with the next one

  /* Same as next_transition(), but choice is now guided, and an integer corresponding to the
   internal cost of the transition is returned */
  virtual std::pair<aid_t, int> next_transition_guided() const;

  /**
   * Explore a new path on the remote app; the parameter 'next' must be the result of a previous call to
   * next_transition()
   */
  std::shared_ptr<Transition> execute_next(aid_t next, RemoteApp& app);

  long get_num() const { return num_; }
  unsigned long get_depth() const { return depth_; }
  std::size_t count_todo() const;

  virtual bool has_more_to_be_explored() const;

  /* Marking as TODO some actor in this state:
   *  + consider_one mark aid actor (and assert it is possible)
   *  + consider_best ensure one actor is marked by eventually marking the best regarding its guiding method
   *  + consider_all mark all enabled actor that are not done yet */
  void consider_one(aid_t aid)
  {
    xbt_assert(actors_to_run_.find(aid) != actors_to_run_.end(),
               "Actor %ld does not exist in state #%ld, yet one was asked", aid, get_num());
    xbt_assert(actors_to_run_.at(aid).is_enabled(),
               "Tried to mark as TODO actor %ld in state #%ld but it is not enabled", aid, get_num());
    xbt_assert(not actors_to_run_.at(aid).is_done(),
               "Tried to mark as TODO actor %ld in state #%ld but it is already done", aid, get_num());
    actors_to_run_.at(aid).mark_todo();
    opened_.emplace_back(
        std::make_shared<Transition>(Transition::Type::UNKNOWN, aid, actors_to_run_.at(aid).get_times_considered()));
  }

  unsigned long consider_all()
  {
    unsigned long count = 0;
    for (auto& [aid, actor] : actors_to_run_)
      if (actor.is_enabled() && not actor.is_done()) {
        actor.mark_todo();
        count++;
        opened_.emplace_back(
            std::make_shared<Transition>(Transition::Type::UNKNOWN, aid, actor.get_times_considered()));
      }
    return count;
  }

  bool is_actor_done(aid_t actor) const { return actors_to_run_.at(actor).is_done(); }
  std::shared_ptr<Transition> get_transition_out() const { return outgoing_transition_; }
  std::shared_ptr<Transition> get_transition_in() const { return incoming_transition_; }
  State* get_parent_state() const { return parent_state_.get(); }
  void reset_parent_state();

  std::map<aid_t, ActorState> const& get_actors_list() const { return actors_to_run_; }

  unsigned long get_actor_count() const { return actors_to_run_.size(); }
  bool is_actor_enabled(aid_t actor) const { return actors_to_run_.at(actor).is_enabled(); }

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
  virtual std::unordered_set<aid_t> get_enabled_actors() const;
  std::vector<aid_t> get_batrack_minus_done() const;

  /* Returns the total amount of states created so far (for statistics) */
  static long get_expanded_states() { return expended_states_; }

  /* Returns the total amount of states created so far (for statistics) */
  static long get_in_memory_states() { return in_memory_states_; }

  /**
   * @brief Register this state as leading to a correct execution and
   * does the same for all its predecessors
   */
  void register_as_correct();

  bool has_correct_execution() { return has_correct_descendent_; }
  /**
   * @brief To be called when this state is backtracked in the exploration.
   */
  void signal_on_backtrack();

  StatePtr get_children_state_of_aid(aid_t next, int times_considered)
  {
    if (next >= 0 && times_considered >= 0 && children_states_.size() > static_cast<long unsigned>(next) &&
        children_states_[next].size() > static_cast<long unsigned>(times_considered))
      return children_states_[next][times_considered];
    return nullptr;
  }

  void record_child_state(StatePtr child);

  const std::vector<std::shared_ptr<Transition>> get_opened_transitions() const { return opened_; }

  xbt::reference_holder<State> reference_holder_;
};
} // namespace simgrid::mc

#endif
