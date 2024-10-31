/* Copyright (c) 2007-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_BASICSTRATEGY_HPP
#define SIMGRID_MC_BASICSTRATEGY_HPP

#include "src/mc/api/strategy/StratLocalInfo.hpp"
#include "src/mc/explo/Exploration.hpp"

namespace simgrid::mc {

/** Basic MC guiding class which corresponds to no guide. When asked for different states
 *  it will follow a depth first search politics to minize the number of opened states. */
class BasicStrategy : public StratLocalInfo {
  int depth_ = _sg_mc_max_depth; // Arbitrary starting point. next_transition must return a positive value to work with
                                 // threshold in BeFSExplorer

public:
  void copy_from(const StratLocalInfo* strategy) override;
  BasicStrategy()           = default;
  ~BasicStrategy() override = default;

  int get_actor_valuation(aid_t aid) const override { return depth_; }

  std::pair<aid_t, int> best_transition(bool must_be_todo) const override;

  void consider_best_among_set(std::unordered_set<aid_t> E) override;

  void execute_next(aid_t aid, RemoteApp& app) override { return; }
};

} // namespace simgrid::mc

#endif
