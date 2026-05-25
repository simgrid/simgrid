/* Copyright (c) 2016-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/api/Strategy.hpp"
#include "simgrid/forward.h"
#include "src/mc/mc_config.hpp"
#include "xbt/log.h"
#include "xbt/random.hpp"

#include <algorithm>
#include <utility>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_strategy, mc, "Generic exploration algorithm of the model-checker");

namespace simgrid::mc {

ExplorationStrategy::ExplorationStrategy() {}

std::pair<Aid, int> ExplorationStrategy::best_transition_in(const State* state, bool must_be_todo) const
{
  // If the state has not been initialized with the actor states
  if (not state->has_been_initialized()) {
    const auto& opened_transitions = state->get_opened_transitions();
    // Those are states created by the reduction that HAVE to be explored, but
    // do not yet contain informations about the application, so impossible to
    // add any form of TODO
    if (opened_transitions.size() == 0)
      return std::make_pair(0u, state->get_depth()); // we return an arbitrary actor != INVALID_AID
    if (_sg_mc_strategy == "none") {
      return std::make_pair(opened_transitions[0]->aid_, state->get_depth());
    } else {
      int chosen = xbt::random::uniform_int(0, opened_transitions.size() - 1);
      return std::make_pair(opened_transitions[chosen]->aid_, 0);
    }
  }

  // For now, we only have two way of considering local actors
  //   + either arbitrary in inserted ordre
  //   + uniform over the activated
  auto actors = state->get_actors_list();

  if (_sg_mc_strategy == "none") {
    // The initial case
    if (state->get_opened_transitions().size() == 0 and not must_be_todo) {
      auto actor_opt = std::find_if(actors.begin(), actors.end(), [](auto& actor) {
        return actor.has_value() and actor->is_enabled() and not actor->is_done();
      });
      if (actor_opt == actors.end())
        return std::make_pair(Aid::INVALID, state->get_depth());
      return std::make_pair(actor_opt->value().get_aid(), state->get_depth());
    }

    for (auto const& t : state->get_opened_transitions()) {
      /* Only consider actors (1) marked as interleaving by the checker and (2) currently enabled in the application */
      if (actors.size() <= t->aid_.value() or not actors[t->aid_.value()].has_value())
        continue;

      auto const actor = actors[t->aid_.value()].value();
      Aid aid          = actor.get_aid();
      if ((not actor.is_todo() && must_be_todo) || not actor.is_enabled() || actor.is_done()) {
        continue;
        XBT_DEBUG("Actor %d discarded as best possible transition", aid.c_val());
      }
      XBT_DEBUG("Actor %d is the best possible transition at state @ %lu", aid.c_val(), state->get_num());
      return std::make_pair(aid, state->get_depth());
    }
    return std::make_pair(Aid::INVALID, state->get_depth());
  } else {
    int possibilities = 0;

    // Consider only valid actors
    for (auto const& actor : actors) {
      if (actor.has_value())
        if ((actor.value().is_todo() || not must_be_todo) && (not actor.value().is_done()) &&
            actor.value().is_enabled())
          possibilities++;
    }

    int chosen;
    if (possibilities == 0)
      return std::make_pair(Aid::INVALID, 0);
    if (possibilities == 1)
      chosen = 0;
    else
      chosen = xbt::random::uniform_int(0, possibilities - 1);

    for (auto const& actor : actors) {
      if (not actor.has_value())
        continue;
      if (((not actor->is_todo()) && must_be_todo) || actor->is_done() || (not actor->is_enabled()))
        continue;
      if (chosen == 0) {
        return std::make_pair(actor->get_aid(), state->get_depth());
      }
      chosen--;
    }

    return std::make_pair(Aid::INVALID, 0);
  }
}

/** Return the associated value of the actor. For now it's not important, so we just return the depth. */
int ExplorationStrategy::get_actor_valuation_in(const State* state, Aid aid) const
{
  return state->get_depth();
}

/** Ensure at least one transition is marked as todo among the enabled ones not done in E.
 *  If required, it marks as todo the best transition according to the StratLocalInfo. */
Aid ExplorationStrategy::consider_best_among_set_in(State* state, std::unordered_set<Aid> E)
{

  auto actors = state->get_actors_list();

  if (std::any_of(begin(E), end(E), [=](const auto& aid) {
        return actors.size() > aid.value() && actors[aid.value()].has_value() && actors[aid.value()]->is_todo();
      }))
    return Aid::INVALID;
  if (_sg_mc_strategy == "none") {

    for (auto& actor : actors) {
      if (not actor.has_value())
        continue;
      Aid aid = actor->get_aid();
      if (E.count(aid) > 0) {
        xbt_assert(actor->is_enabled(), "Asked to consider one among a set containing the disabled actor %d",
                   aid.c_val());
        state->consider_one(aid);
        return aid;
      }
    }
  } else {
    int chosen = xbt::random::uniform_int(0, E.size() - 1);
    for (Aid aid : E) {
      if (chosen > 0) {
        chosen--;
        continue;
      }
      xbt_assert(state->is_actor_enabled(aid), "Asked to consider one among a set containing the disabled actor %d",
                 chosen);
      state->consider_one(aid);
      return aid;
    }
  }
  return Aid::INVALID;
}

Aid ExplorationStrategy::ensure_one_considered_among_set_in(State* state, std::unordered_set<Aid> E)
{
  for (auto& actor : state->get_actors_list()) {
    if (not actor.has_value())
      continue;
    // If we find an actor already satisfying condition E, we return
    if (E.count(actor->get_aid()) > 0 && (actor->is_done() or actor->is_todo()))
      return Aid::INVALID;
  }

  return consider_best_among_set_in(state, E);
}
} // namespace simgrid::mc
