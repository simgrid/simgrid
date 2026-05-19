/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_STRATLOCALINFO_HPP
#define SIMGRID_MC_STRATLOCALINFO_HPP

#include "simgrid/forward.h"
#include "src/mc/api/states/State.hpp"

#include <map>
#include <utility>
#include <vector>

namespace simgrid::mc {

class ExplorationStrategy {

public:
  ExplorationStrategy();

  /** Returns the best transition among according to the StratLocalInfo for this state.
   *  The StratLocalInfo should consider only enabled transition not already done.
   *  Furthermore, if must_be_todo is set to true, only transitions marked as todo
   *  should be considered. */
  std::pair<Aid, int> best_transition_in(const State* state, bool must_be_todo) const;

  /** Returns the best transition among those that should be interleaved. */
  std::pair<Aid, int> next_transition_in(const State* state) const { return best_transition_in(state, true); }

  /** Return the associated value of the actor */
  int get_actor_valuation_in(const State* state, Aid aid) const;

  /** Ensure at least one transition is marked as todo among the enabled ones not done.
   *  If required, it marks as todo the best transition according to the StratLocalInfo. */
  void consider_best_in(State* state)
  {
    auto actors = state->get_actors_list();
    if (std::any_of(begin(actors), end(actors),
                    [](const auto& actor) { return actor.has_value() and actor->is_todo(); }))
      return;
    Aid best_aid = best_transition_in(state, false).first;
    if (best_aid.has_value())
      state->consider_one(best_aid);
  }

  /** Ensure at least one transition is marked as todo among the enabled ones not done in E.
   *  If required, it marks as todo the best transition according to the StratLocalInfo. */
  Aid consider_best_among_set_in(State* state, std::unordered_set<Aid> E);

  /** After the call to this function, at least one transition from set of process E will
   *  be explored at some point, ie. one transition must be marked todo, already been marked
   *  todo, or already been done */
  Aid ensure_one_considered_among_set_in(State* state, std::unordered_set<Aid> E);
};

} // namespace simgrid::mc

#endif
