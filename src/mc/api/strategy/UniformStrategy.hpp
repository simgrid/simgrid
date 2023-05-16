/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UNIFORMSTRATEGY_HPP
#define SIMGRID_MC_UNIFORMSTRATEGY_HPP

#include "src/mc/transition/Transition.hpp"
#include "src/plugins/cfg/CFGMap.hpp"

namespace simgrid::mc {

/** Guiding strategy that valuate states randomly */
class UniformStrategy : public Strategy {
  std::map<aid_t, int> valuation;

public:
  UniformStrategy()
  {
    for (long aid = 0; aid < 10; aid++)
      valuation[aid] = rand() % 1000;
  }
  void copy_from(const Strategy* strategy) override
  {
    for (auto& [aid, _] : actors_to_run_)
      valuation[aid] = rand() % 1000;
  }

  std::pair<aid_t, int> next_transition() const override
  {
    int possibilities = 0;

    // Consider only valid actors
    for (auto const& [aid, actor] : actors_to_run_) {
      if (actor.is_todo() and (not actor.is_done()) and actor.is_enabled())
        possibilities++;
    }

    int chosen;
    if (possibilities == 0)
      return std::make_pair(-1, 100000);
    if (possibilities == 1)
      chosen = 0;
    else
      chosen = rand() % possibilities;

    for (auto const& [aid, actor] : actors_to_run_) {
      if ((not actor.is_todo()) or actor.is_done() or (not actor.is_enabled()))
        continue;
      if (chosen == 0) {
        return std::make_pair(aid, valuation.at(aid));
      }
      chosen--;
    }

    return std::make_pair(-1, 100000);
  }

  void execute_next(aid_t aid, RemoteApp& app) override {}

  void consider_best() override
  {

    int possibilities = 0;
    // Consider only valid actors
    // If some actor are already considered as todo, skip
    for (auto const& [aid, actor] : actors_to_run_) {
      if (valuation.count(aid) == 0)
        for (auto& [aid, _] : actors_to_run_)
          valuation[aid] = rand() % 1000;
      if (actor.is_todo())
        return;
      if (actor.is_enabled() and not actor.is_done())
        possibilities++;
    }

    int chosen;
    if (possibilities == 0)
      return;
    if (possibilities == 1)
      chosen = 0;
    else
      chosen = rand() % possibilities;

    for (auto& [aid, actor] : actors_to_run_) {
      if (not actor.is_enabled() or actor.is_done())
        continue;
      if (chosen == 0) {
        actor.mark_todo();
        return;
      }
      chosen--;
    }
    THROW_IMPOSSIBLE; // One actor should be marked as todo before
  }
};

} // namespace simgrid::mc

#endif
