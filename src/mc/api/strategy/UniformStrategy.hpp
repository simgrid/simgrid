/* Copyright (c) 2007-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UNIFORMSTRATEGY_HPP
#define SIMGRID_MC_UNIFORMSTRATEGY_HPP

#include "src/mc/api/strategy/StratLocalInfo.hpp"

namespace simgrid::mc {

/** Guiding strategy that valuate states randomly */
class UniformStrategy : public StratLocalInfo {
  static constexpr int MAX_RAND = 100000;

  std::map<aid_t, int> valuation;

public:
  UniformStrategy();
  void copy_from(const StratLocalInfo* strategy) override;

  int get_actor_valuation(aid_t aid) const override { return valuation.at(aid); }

  std::pair<aid_t, int> best_transition(bool must_be_todo) const override;

  // FIXME: this is not using the strategy
  void consider_best_among_set(std::unordered_set<aid_t> E) override;

  void execute_next(aid_t aid, RemoteApp& app) override {}
};

} // namespace simgrid::mc

#endif
