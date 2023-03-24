/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_STATE_HPP
#define SIMGRID_MC_STATE_HPP

#include "src/mc/api/ActorState.hpp"
#include "src/mc/api/RemoteApp.hpp"
#include "src/mc/api/guide/GuidedState.hpp"
#include "src/mc/sosp/Snapshot.hpp"
#include "src/mc/transition/Transition.hpp"

namespace simgrid::mc {

/* A node in the exploration graph (kind-of) */
class XBT_PRIVATE State : public xbt::Extendable<State> {
  static long expended_states_; /* Count total amount of states, for stats */

  /**
   * @brief An empty transition that leads to this state by default
   */
  const std::unique_ptr<Transition> default_transition_ = std::make_unique<Transition>();

  /**
   * @brief The outgoing transition: what was the last transition that
   * we took to leave this state?
   *
   * The owner of the transition is the `ActorState` instance which exists in this state,
   * or a reference to the internal default transition `Transition()` if no transition has been
   * set
   */
  Transition* transition_ = default_transition_.get();

  /** Sequential state ID (used for debugging) */
  long num_ = 0;

  /** Snapshot of system state (if needed) */
  std::shared_ptr<Snapshot> system_state_;

  /** Unique parent of this state. Required both for sleep set computation
      and for guided model-checking */
  const State* parent_state_;

  std::shared_ptr<GuidedState> guide_;

  /* Sleep sets are composed of the actor and the corresponding transition that made it being added to the sleep
   * set. With this information, it is check whether it should be removed from it or not when exploring a new
   * transition */
  std::map<aid_t, Transition> sleep_set_;

public:
  explicit State(RemoteApp& remote_app);
  explicit State(RemoteApp& remote_app, const State* parent_state);
  explicit State(const State& other);
  /* Returns a positive number if there is another transition to pick, or -1 if not */
  aid_t next_transition() const; // this function should disapear as it is redundant with the next one

  /* Same as next_transition, but choice is now guided, and a double corresponding to the
   internal cost of the transition is returned */
  std::pair<aid_t, double> next_transition_guided() const;

  /* Explore a new path on the remote app; the parameter 'next' must be the result of a previous call to
   * next_transition() */
  void execute_next(aid_t next, RemoteApp& app);

  long get_num() const { return num_; }
  std::size_t count_todo() const;

  /* Marking as TODO some actor in this state:
   *  + consider_one mark aid actor (and assert it is possible)
   *  + consider_best ensure one actor is marked by eventually marking the best regarding its guiding methode
   *  + conside_all mark all enabled actor that are not done yet */
  void consider_one(aid_t aid) { guide_->consider_one(aid); }
  void consider_best() { guide_->consider_best(); }
  void consider_all() { guide_->consider_all(); }

  bool is_actor_done(aid_t actor) const { return guide_->actors_to_run_.at(actor).is_done(); }
  Transition* get_transition() const;
  void set_transition(Transition* t) { transition_ = t; }
  std::map<aid_t, ActorState> const& get_actors_list() const { return guide_->actors_to_run_; }

  unsigned long get_actor_count() const { return guide_->actors_to_run_.size(); }
  bool is_actor_enabled(aid_t actor) { return guide_->actors_to_run_.at(actor).is_enabled(); }

  Snapshot* get_system_state() const { return system_state_.get(); }
  void set_system_state(std::shared_ptr<Snapshot> state) { system_state_ = std::move(state); }

  std::map<aid_t, Transition> const& get_sleep_set() const { return sleep_set_; }
  void add_sleep_set(const Transition* t)
  {
    sleep_set_.insert_or_assign(t->aid_, Transition(t->type_, t->aid_, t->times_considered_));
  }

  /* Returns the total amount of states created so far (for statistics) */
  static long get_expanded_states() { return expended_states_; }
};
} // namespace simgrid::mc

#endif
