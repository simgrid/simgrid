/* Copyright (c) 2007-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/api/strategy/BasicStrategy.hpp"
#include "xbt/log.h"

XBT_LOG_EXTERNAL_CATEGORY(mc_dfs);

namespace simgrid::mc {

void BasicStrategy::copy_from(const StratLocalInfo* strategy)
{
  const auto* cast_strategy = dynamic_cast<BasicStrategy const*>(strategy);
  xbt_assert(cast_strategy != nullptr);
  depth_ = cast_strategy->depth_ - 1;
  if (depth_ <= 0) {
    XBT_CERROR(mc_dfs,
               "The exploration reached a depth greater than %d. Change the depth limit with "
               "--cfg=model-check/max-depth. Here are the 100 first and 100 last trace elements",
               _sg_mc_max_depth.get());
    auto trace = Exploration::get_instance()->get_textual_trace();
    for (unsigned i = 0; i < trace.size(); i++)
      if (i < 100 || i > (trace.size() - 100))
        XBT_CERROR(mc_dfs, "  %s", trace[i].c_str());
      else if (i == 100)
        XBT_CERROR(mc_dfs, " ... (omitted trace elements)");
    xbt_die("Aborting now.");
  }
}

std::pair<aid_t, int> BasicStrategy::best_transition(bool must_be_todo) const
{
  for (auto const& [aid, actor] : actors_to_run_) {
    /* Only consider actors (1) marked as interleaving by the checker and (2) currently enabled in the application */
    if ((not actor.is_todo() && must_be_todo) || not actor.is_enabled() || actor.is_done())
      continue;

    return std::make_pair(aid, depth_);
  }
  return std::make_pair(-1, depth_);
}

void BasicStrategy::consider_best_among_set(std::unordered_set<aid_t> E)
{
  if (std::any_of(begin(E), end(E), [this](const auto& aid) {
        // FIXME: I fail to undestand why E contains actors that are not to run yet. It appeared when I added the
        // ActorCreate transition, and I have the feeling that E contains an actor that is yet to be created, but I
        // don't understand SDPOR enough to be definitive here. -- Mt So, the following lines survive the fact that E
        // contains invalid AIDs. It used to simply be ``actors_to_run_.at(aid)``
        auto search = actors_to_run_.find(aid);
        if (search == actors_to_run_.end())
          return false;
        return search->second.is_todo();
      }))
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
