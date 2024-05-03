/* Copyright (c) 2007-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/api/strategy/UniformStrategy.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "xbt/random.hpp"

namespace simgrid::mc {

UniformStrategy::UniformStrategy()
{
  for (unsigned long aid = 0; aid < Exploration::get_instance()->get_remote_app().get_maxpid(); aid++)
    valuation[aid] = xbt::random::uniform_int(0, MAX_RAND);
}
void UniformStrategy::copy_from(const StratLocalInfo* strategy)
{
  for (auto const& [aid, _] : actors_to_run_)
    valuation[aid] = xbt::random::uniform_int(0, MAX_RAND);
}

std::pair<aid_t, int> UniformStrategy::best_transition(bool must_be_todo) const
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
    chosen = xbt::random::uniform_int(0, possibilities - 1);

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

// FIXME: this is not using the strategy
void UniformStrategy::consider_best_among_set(std::unordered_set<aid_t> E)
{
  if (std::any_of(begin(E), end(E), [this](const auto& aid) { return actors_to_run_.at(aid).is_todo(); }))
    return;
  for (auto& [aid, actor] : actors_to_run_) {
    /* Only consider actors (1) marked as interleaving by the checker and (2) currently enabled in the application */
    if (E.count(aid) > 0) {
      xbt_assert(actor.is_enabled(), "Asked to consider one among a set containing the disabled actor %ld", aid);
      actor.mark_todo();
      return;
    }
  }
}

} // namespace simgrid::mc
