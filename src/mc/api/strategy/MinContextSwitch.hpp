/* Copyright (c) 2007-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_MINCONTEXTSWITCH_HPP
#define SIMGRID_MC_MINCONTEXTSWITCH_HPP

#include "src/mc/api/strategy/StratLocalInfo.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "xbt/random.hpp"
#include <algorithm>
#include <utility>

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

  void copy_from(const StratLocalInfo* strategy) override
  {
    const auto* cast_strategy = dynamic_cast<MinContextSwitch const*>(strategy);
    xbt_assert(cast_strategy != nullptr);
    current_actor_     = cast_strategy->current_actor_;
    nb_context_switch_ = cast_strategy->nb_context_switch_;
    // If we keep executing the same actor, no context switch
    if (cast_strategy->next_executed_actor_ == current_actor_)
      return;

    const auto action = cast_strategy->actors_to_run_.find(cast_strategy->current_actor_);
    // If the chosen actor has come to an end, we did not do a context switch, but
    // we have to record this actor has the new current actor
    if (action == cast_strategy->actors_to_run_.end()) {
      current_actor_ = cast_strategy->next_executed_actor_;
      return;
    }

    // If we executed something else because current actor is blocked for now, it's okay
    if (action->second.is_unknown())
      return;

    // In other cases, it means we did a context switch. Certainly because
    // we are exploring a different trace.
    nb_context_switch_++;
    current_actor_ = cast_strategy->next_executed_actor_;
  }

  std::pair<aid_t, int> best_transition(bool must_be_todo) const override
  {
    const auto action = actors_to_run_.find(current_actor_);
    if (action != actors_to_run_.end() and action->second.is_enabled() and (action->second.is_todo() or !must_be_todo))
      return std::make_pair(current_actor_, nb_context_switch_);

    std::map<aid_t, const ActorState> fireable;
    std::for_each(actors_to_run_.begin(), actors_to_run_.end(), [&](const auto& actor) {
      if (actor.second.is_enabled() and (actor.second.is_todo() or !must_be_todo))
        fireable.emplace(actor);
    });
    if (fireable.size() == 0)
      return std::make_pair(-1, 0);
    auto random_actor = std::next(std::begin(fireable), xbt::random::uniform_int(0, fireable.size() - 1));

    return std::make_pair(random_actor->second.get_aid(), nb_context_switch_);
  }

  void consider_best_among_set(std::unordered_set<aid_t> E) override
  {
    const auto action_in_E = E.find(current_actor_);
    const auto action      = actors_to_run_.find(current_actor_);
    if (action_in_E != E.end() and action != actors_to_run_.end() and action->second.is_enabled()) {
      action->second.mark_todo();
      return;
    }

    std::map<aid_t, const ActorState> fireable;
    std::for_each(actors_to_run_.begin(), actors_to_run_.end(), [&](const auto& actor) {
      if (actor.second.is_enabled() and E.count(actor.first) > 0)
        fireable.emplace(actor);
    });
    if (fireable.size() == 0)
      return;
    auto random_actor = std::next(std::begin(fireable), xbt::random::uniform_int(0, fireable.size() - 1));

    actors_to_run_.at(random_actor->second.get_aid()).mark_todo();
  }

  void execute_next(aid_t aid, RemoteApp& app) override { next_executed_actor_ = aid; }
};

} // namespace simgrid::mc

#endif
