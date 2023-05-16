/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/asserts.h"

#ifndef SIMGRID_MC_STRATEGY_HPP
#define SIMGRID_MC_STRATEGY_HPP

namespace simgrid::mc {

class Strategy {
protected:
  /** State's exploration status by actor. All actors should be present, eventually disabled for now. */
  std::map<aid_t, ActorState> actors_to_run_;

public:
  virtual void copy_from(const Strategy*) = 0;
  Strategy()                              = default;
  virtual ~Strategy()                     = default;

  virtual std::pair<aid_t, int> next_transition() const = 0;
  virtual void execute_next(aid_t aid, RemoteApp& app)  = 0;

  // Mark the first enabled and not yet done transition as todo
  // If there's already a transition marked as todo, does nothing
  virtual void consider_best() = 0;

  // Mark aid as todo. If it makes no sense, ie. if it is already done or not enabled,
  // raise an error
  void consider_one(aid_t aid)
  {
    xbt_assert(actors_to_run_.at(aid).is_enabled() and not actors_to_run_.at(aid).is_done(),
               "Tried to mark as TODO actor %ld but it is either not enabled or already done", aid);
    actors_to_run_.at(aid).mark_todo();
  }
  // Mark as todo all actors enabled that are not done yet and return the number
  // of marked actors
  unsigned long consider_all()
  {
    unsigned long count = 0;
    for (auto& [_, actor] : actors_to_run_)
      if (actor.is_enabled() and not actor.is_done()) {
        actor.mark_todo();
        count++;
      }
    return count;
  }

  friend class State;
};

} // namespace simgrid::mc

#endif
