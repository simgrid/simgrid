/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_WAITSTRATEGY_HPP
#define SIMGRID_MC_WAITSTRATEGY_HPP

#include "src/mc/transition/Transition.hpp"

namespace simgrid::mc {

/** Wait MC guiding class that aims at minimizing the number of in-fly communication.
 *  When possible, it will try to take the wait transition. */
class WaitStrategy : public Strategy {
  int taken_wait_   = 0;
  bool taking_wait_ = false;

public:
  void operator=(const WaitStrategy& guide) { taken_wait_ = guide.taken_wait_; }

  bool is_transition_wait(Transition::Type type) const
  {
    return type == Transition::Type::WAITANY or type == Transition::Type::BARRIER_WAIT or
           type == Transition::Type::MUTEX_WAIT or type == Transition::Type::SEM_WAIT;
  }

  std::pair<aid_t, int> next_transition() const override
  {
    std::pair<aid_t, int> if_no_wait = std::make_pair(-1, 0);
    for (auto const& [aid, actor] : actors_to_run_) {
      if (not actor.is_todo() || not actor.is_enabled() || actor.is_done())
        continue;
      if (is_transition_wait(actor.get_transition(actor.get_times_considered())->type_))
        return std::make_pair(aid, -(taken_wait_ + 1));
      if_no_wait = std::make_pair(aid, -taken_wait_);
    }
    return if_no_wait;
  }

  /** If we are taking a wait transition, and last transition wasn't a wait, we need to increment the number
   *  of wait taken. On the opposite, if we took a wait before, and now we are taking another transition, we need
   *  to decrease the count. */
  void execute_next(aid_t aid, RemoteApp& app) override
  {
    auto const& actor = actors_to_run_.at(aid);
    if ((not taking_wait_) and is_transition_wait(actor.get_transition(actor.get_times_considered())->type_)) {
      taken_wait_++;
      taking_wait_ = true;
      return;
    }
    if (taking_wait_ and (not is_transition_wait(actor.get_transition(actor.get_times_considered())->type_))) {
      taken_wait_--;
      taking_wait_ = false;
      return;
    }
  }

  void consider_best() override
  {
    aid_t aid = next_transition().first;
    if (auto actor = actors_to_run_.find(aid); actor != actors_to_run_.end()) {
      actor->second.mark_todo();
      return;
    }
    for (auto& [_, actor] : actors_to_run_) {
      if (actor.is_todo())
        return;
      if (actor.is_enabled() and not actor.is_done()) {
        actor.mark_todo();
        return;
      }
    }
  }
};

} // namespace simgrid::mc

#endif
