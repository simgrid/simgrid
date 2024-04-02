/* Copyright (c) 2007-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_STRATLOCALINFO_HPP
#define SIMGRID_MC_STRATLOCALINFO_HPP

#include "simgrid/forward.h"
#include "src/mc/api/RemoteApp.hpp"
#include "xbt/asserts.h"
#include <map>
#include <utility>

namespace simgrid::mc {

class StratLocalInfo {
protected:
  /** State's exploration status by actor. All actors should be present, eventually disabled for now. */
  std::map<aid_t, ActorState> actors_to_run_;

public:
  /** Strategies can have values shared from parent to children state.
   *  This method should copy all value the StratLocalInfo needs from its parent. */
  virtual void copy_from(const StratLocalInfo*) = 0;

  StratLocalInfo()          = default;
  virtual ~StratLocalInfo() = default;

  /** Returns the best transition among according to the StratLocalInfo for this state.
   *  The StratLocalInfo should consider only enabled transition not already done.
   *  Furthermore, if must_be_todo is set to true, only transitions marked as todo
   *  should be considered. */
  virtual std::pair<aid_t, int> best_transition(bool must_be_todo) const = 0;

  /** Returns the best transition among those that should be interleaved. */
  std::pair<aid_t, int> next_transition() const { return best_transition(true); }

  /** Allows for the StratLocalInfo to update its fields knowing that the actor aid will
   *  be executed and a children StratLocalInfo will then be created. */
  virtual void execute_next(aid_t aid, RemoteApp& app) = 0;

  /** Return the associated value of the actor */
  virtual int get_actor_valuation(aid_t aid) const = 0;

  /** Ensure at least one transition is marked as todo among the enabled ones not done.
   *  If required, it marks as todo the best transition according to the StratLocalInfo. */
  void consider_best()
  {
    if (std::any_of(begin(actors_to_run_), end(actors_to_run_),
                    [](const auto& actor) { return actor.second.is_todo(); }))
      return;
    aid_t best_aid = best_transition(false).first;
    if (best_aid != -1)
      actors_to_run_.at(best_aid).mark_todo();
  }

  /** Ensure at least one transition is marked as todo among the enabled ones not done in E.
   *  If required, it marks as todo the best transition according to the StratLocalInfo. */
  virtual void consider_best_among_set(std::unordered_set<aid_t> E) = 0;

  // Mark aid as todo. If it makes no sense, ie. if it is already done or not enabled,
  // else raise an error
  void consider_one(aid_t aid)
  {
    xbt_assert(actors_to_run_.at(aid).is_enabled() && not actors_to_run_.at(aid).is_done(),
               "Tried to mark as TODO actor %ld but it is either not enabled or already done", aid);
    actors_to_run_.at(aid).mark_todo();
  }
  // Mark as todo all actors enabled that are not done yet and return the number
  // of marked actors
  unsigned long consider_all()
  {
    unsigned long count = 0;
    for (auto& [_, actor] : actors_to_run_)
      if (actor.is_enabled() && not actor.is_done()) {
        actor.mark_todo();
        count++;
      }
    return count;
  }
  /** After the call to this function, at least one transition from set of process E will
   *  be explored at some point, ie. one transition must be marked todo, already been marked
   *  todo, or already been done */
  void ensure_one_considered_among_set(std::unordered_set<aid_t> E)
  {
    for (auto& [p, actor] : actors_to_run_) {
      // If we find an actor already satisfying condition E, we return
      if (E.count(p) > 0 && (actor.is_done() or actor.is_todo()))
        return;
    }

    consider_best_among_set(E);
  }

  friend class State;
  friend class SleepSetState;
  friend class Reduction;
};

} // namespace simgrid::mc

#endif
