/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_BASICSTRATEGY_HPP
#define SIMGRID_MC_BASICSTRATEGY_HPP

#include "Strategy.hpp"

namespace simgrid::mc {

/** Basic MC guiding class which corresponds to no guide. When asked for different states
 *  it will follow a depth first search politics to minize the number of opened states. */
class BasicStrategy : public Strategy {
    int depth_ = 100000; // Arbitrary starting point. next_transition must return a positiv value to work with threshold in DFSExplorer

public:
  void copy_from(const Strategy* strategy) override
  {
    const BasicStrategy* cast_strategy = static_cast<BasicStrategy const*>(strategy);
    xbt_assert(cast_strategy != nullptr);
    depth_ = cast_strategy->depth_ - 1;
    xbt_assert(depth_ > 0, "The exploration reached a depth greater than 100000. We will stop here to prevent weird interaction with DFSExplorer.");
  }
  BasicStrategy()                     = default;
  ~BasicStrategy() override           = default;

  std::pair<aid_t, int> next_transition() const override
  {
    for (auto const& [aid, actor] : actors_to_run_) {
      /* Only consider actors (1) marked as interleaving by the checker and (2) currently enabled in the application */
      if (not actor.is_todo() || not actor.is_enabled() || actor.is_done()) {
        continue;
      }

      return std::make_pair(aid, depth_);
    }
    return std::make_pair(-1, depth_);
  }
  void execute_next(aid_t aid, RemoteApp& app) override { return; }

  void consider_best() override
  {
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
