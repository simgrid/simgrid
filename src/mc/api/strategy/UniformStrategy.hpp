/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UNIFORMSTRATEGY_HPP
#define SIMGRID_MC_UNIFORMSTRATEGY_HPP

#include "src/mc/transition/Transition.hpp"
#include "xbt/random.hpp"

namespace simgrid::mc {

/** Guiding strategy that valuate states randomly */
class UniformStrategy : public Strategy {
  static constexpr int MAX_RAND = 100000;

  std::map<aid_t, int> valuation;

public:
  UniformStrategy()
  {
    for (long aid = 0; aid < 10; aid++)
      valuation[aid] = xbt::random::uniform_int(0, MAX_RAND);
  }
  void copy_from(const Strategy* strategy) override
  {
    for (auto const& [aid, _] : actors_to_run_)
      valuation[aid] = xbt::random::uniform_int(0, MAX_RAND);
  }

  std::pair<aid_t, int> best_transition(bool must_be_todo) const override
  {
    int possibilities = 0;

    // Consider only valid actors
    for (auto const& [aid, actor] : actors_to_run_) {
      if ((actor.is_todo() || not must_be_todo) && (not actor.is_done()) && actor.is_enabled())
        possibilities++;
    }

    int chosen;
    if (possibilities == 0)
      return std::make_pair(-1, 0);
    if (possibilities == 1)
      chosen = 0;
    else
	chosen = xbt::random::uniform_int(0, possibilities-1);

    for (auto const& [aid, actor] : actors_to_run_) {
      if (((not actor.is_todo()) && must_be_todo) || actor.is_done() || (not actor.is_enabled()))
        continue;
      if (chosen == 0) {
        return std::make_pair(aid, valuation.at(aid));
      }
      chosen--;
    }

    return std::make_pair(-1, 0);
  }

  void execute_next(aid_t aid, RemoteApp& app) override {}
};

} // namespace simgrid::mc

#endif
