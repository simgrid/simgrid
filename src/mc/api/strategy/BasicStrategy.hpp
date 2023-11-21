/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_BASICSTRATEGY_HPP
#define SIMGRID_MC_BASICSTRATEGY_HPP

#include "Strategy.hpp"
#include "src/mc/explo/Exploration.hpp"

XBT_LOG_EXTERNAL_CATEGORY(mc_dfs);

namespace simgrid::mc {

/** Basic MC guiding class which corresponds to no guide. When asked for different states
 *  it will follow a depth first search politics to minize the number of opened states. */
class BasicStrategy : public Strategy {
  int depth_ = _sg_mc_max_depth; // Arbitrary starting point. next_transition must return a positive value to work with
                                 // threshold in DFSExplorer

public:
  void copy_from(const Strategy* strategy) override
  {
    const auto* cast_strategy = dynamic_cast<BasicStrategy const*>(strategy);
    xbt_assert(cast_strategy != nullptr);
    depth_ = cast_strategy->depth_ - 1;
    if (depth_ <= 0) {
      XBT_CERROR(mc_dfs,
                 "The exploration reached a depth greater than %d. Change the depth limit with "
                 "--cfg=model-check/max-depth. Here are the 100 first trace elements",
                 _sg_mc_max_depth.get());
      auto trace = Exploration::get_instance()->get_textual_trace(100);
      for (auto const& elm : trace)
        XBT_CERROR(mc_dfs, "  %s", elm.c_str());
      xbt_die("Aborting now.");
    }
  }
  BasicStrategy()                     = default;
  ~BasicStrategy() override           = default;

  std::pair<aid_t, int> best_transition(bool must_be_todo) const override {
    for (auto const& [aid, actor] : actors_to_run_) {
      /* Only consider actors (1) marked as interleaving by the checker and (2) currently enabled in the application */
      if ((not actor.is_todo() && must_be_todo) || not actor.is_enabled() || actor.is_done())
        continue;

      return std::make_pair(aid, depth_);
    }
    return std::make_pair(-1, depth_);
  }

  void execute_next(aid_t aid, RemoteApp& app) override { return; }
};

} // namespace simgrid::mc

#endif
