/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_BASICSTRATEGY_HPP
#define SIMGRID_MC_BASICSTRATEGY_HPP

#include "src/mc/transition/Transition.hpp"

namespace simgrid::mc {

/** Basic MC guiding class which corresponds to no guide at all (random choice) */
class BasicStrategy : public Strategy {
public:
  void operator=(const BasicStrategy& other) { this->penalties_ = other.penalties_; }

  std::pair<aid_t, double> best_transition(bool need_to_be_todo) const
  {
    auto min = std::make_pair(-1, 10000);
    for (auto const& [aid, actor] : actors_to_run_) {
      /* Only consider actors (1) marked as interleaving by the checker and (2) currently enabled in the application */
      if ((not actor.is_todo() and need_to_be_todo) || not actor.is_enabled() || actor.is_done()) {
        continue;
      }
      if (actor.get_transition(actor.get_times_considered())->type_ != Transition::Type::TESTANY)
        return std::make_pair(aid, 0.0);

      double dist;
      auto iterator = penalties_.find(aid);
      if (iterator != penalties_.end())
        dist = (*iterator).second;
      else
        dist = 0;
      if (dist < min.second)
        min = std::make_pair(aid, dist);
    }
    if (min.first == -1)
      return std::make_pair(-1, -1000);
    return min;
  }

  std::pair<aid_t, double> next_transition() const override { return best_transition(true); }

  void execute_next(aid_t aid, RemoteApp& app) override
  {
    auto actor = actors_to_run_.at(aid);
    if (actor.get_transition(actor.get_times_considered())->type_ == Transition::Type::TESTANY)
      penalties_[aid] = penalties_[aid] + 1.0;
    return;
  }

  void consider_best() override
  {
    const auto& [aid, _] = this->best_transition(false);
    auto actor           = actors_to_run_.find(aid);
    if (actor != actors_to_run_.end()) {
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
