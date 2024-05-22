/* Copyright (c) 2007-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_MINCONTEXTSWITCH_HPP
#define SIMGRID_MC_MINCONTEXTSWITCH_HPP

#include "src/mc/api/strategy/StratLocalInfo.hpp"
#include "src/mc/explo/Exploration.hpp"

namespace simgrid::mc {

/** Guiding strategy that explores in priority traces with the least number
 *  of real context switch. Real context switches occur when the original actor
 *  is still enabled but not executed. */
class MinContextSwitch : public StratLocalInfo {

  aid_t current_actor_       = 1;
  aid_t next_executed_actor_ = -1;
  int nb_context_switch_     = 0;

public:
  MinContextSwitch() {}

  int get_actor_valuation(aid_t aid) const override { return nb_context_switch_; }

  void copy_from(const StratLocalInfo* strategy) override;

  std::pair<aid_t, int> best_transition(bool must_be_todo) const override;

  void consider_best_among_set(std::unordered_set<aid_t> E) override;

  void execute_next(aid_t aid, RemoteApp& app) override { next_executed_actor_ = aid; }
};

} // namespace simgrid::mc

#endif
