/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/reduction/Reduction.hpp"
#include "src/mc/api/State.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_reduction, mc, "Logging specific to the reduction algorithms");

namespace simgrid::mc {

void Reduction::on_state_creation(State* s)
{
  for (const auto& [aid, transition] : s->parent_state_->get_sleep_set()) {
    if (not s->incoming_transition_->depends(transition.get())) {
      s->sleep_set_.try_emplace(aid, transition);
      if (s->strategy_->actors_to_run_.count(aid) != 0)
        s->strategy_->actors_to_run_.at(aid).mark_done();
    }
  }
}

void Reduction::on_backtrack(State* s)
{
  s->add_sleep_set(s->get_transition_out());
}
} // namespace simgrid::mc
