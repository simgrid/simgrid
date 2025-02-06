/* Copyright (c) 2016-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/api/Strategy.hpp"
#include "src/mc/mc_config.hpp"
#include "xbt/log.h"
#include "xbt/random.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_strategy, mc, "Generic exploration algorithm of the model-checker");

namespace simgrid::mc {

ExplorationStrategy::ExplorationStrategy() {}

std::pair<aid_t, int> ExplorationStrategy::best_transition_in(const State* state, bool must_be_todo) const
{
  // For now, we only have to way of considering local actors
  //   + either arbitrary in aid ordre
  //   + uniform over the activated
  auto actors = state->get_actors_list();
  if (_sg_mc_strategy == "none") {

    for (auto const& [aid, actor] : actors) {
      /* Only consider actors (1) marked as interleaving by the checker and (2) currently enabled in the application */
      if ((not actor.is_todo() && must_be_todo) || not actor.is_enabled() || actor.is_done()) {
        continue;
        XBT_DEBUG("Actor %ld discarded as best possible transition", aid);
      }
      XBT_DEBUG("Actor %ld is the best possible transition at state @ %ld", aid, state->get_num());
      return std::make_pair(aid, state->get_depth());
    }
    return std::make_pair(-1, state->get_depth());
  } else {
    int possibilities = 0;

    // Consider only valid actors
    for (auto const& [aid, actor] : actors) {
      if ((actor.is_todo() || not must_be_todo) && (not actor.is_done()) && actor.is_enabled())
        possibilities++;
    }

    int chosen;
    if (possibilities == 0)
      return std::make_pair(-1, 0);
    if (possibilities == 1)
      chosen = 0;
    else
      chosen = xbt::random::uniform_int(0, possibilities - 1);

    for (auto const& [aid, actor] : actors) {
      if (((not actor.is_todo()) && must_be_todo) || actor.is_done() || (not actor.is_enabled()))
        continue;
      if (chosen == 0) {
        return std::make_pair(aid, state->get_depth());
      }
      chosen--;
    }

    return std::make_pair(-1, 0);
  }
}

/** Return the associated value of the actor. For now it's not important, so we just return the depth. */
int ExplorationStrategy::get_actor_valuation_in(const State* state, aid_t aid) const
{
  return state->get_depth();
}

/** Ensure at least one transition is marked as todo among the enabled ones not done in E.
 *  If required, it marks as todo the best transition according to the StratLocalInfo. */
void ExplorationStrategy::consider_best_among_set_in(State* state, std::unordered_set<aid_t> E)
{

  if (std::any_of(begin(E), end(E), [=](const auto& aid) { return state->get_actors_list().at(aid).is_todo(); }))
    return;

  auto actors = state->get_actors_list();
  if (_sg_mc_strategy == "none") {

    for (auto& [aid, actor] : actors) {
      if (E.count(aid) > 0) {
        xbt_assert(actor.is_enabled(), "Asked to consider one among a set containing the disabled actor %ld", aid);
        state->consider_one(aid);
        return;
      }
    }
  } else {
    aid_t chosen = xbt::random::uniform_int(0, E.size() - 1);
    for (aid_t aid : E) {
      if (chosen > 0) {
        chosen--;
        continue;
      }
      xbt_assert(state->is_actor_enabled(aid), "Asked to consider one among a set containing the disabled actor %ld",
                 chosen);
      state->consider_one(aid);
      return;
    }
  }
}

} // namespace simgrid::mc
