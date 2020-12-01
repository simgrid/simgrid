/* Copyright (c) 2011-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/VisitedState.hpp"
#include "src/mc/mc_private.hpp"

#include <unistd.h>
#include <sys/wait.h>
#include <memory>
#include <boost/range/algorithm.hpp>
#include "src/mc/mc_api.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_VisitedState, mc, "Logging specific to state equality detection mechanisms");

using mcapi = simgrid::mc::mc_api;

namespace simgrid {
namespace mc {

/** @brief Save the current state */
VisitedState::VisitedState(unsigned long state_number) : num(state_number)
{  
  this->heap_bytes_used = mcapi::get().get_remote_heap_bytes();
  this->actors_count = mcapi::get().get_actors_size();
  this->system_state = std::make_shared<simgrid::mc::Snapshot>(state_number);
}

void VisitedStates::prune()
{
  while (states_.size() > (std::size_t)_sg_mc_max_visited_states) {
    XBT_DEBUG("Try to remove visited state (maximum number of stored states reached)");
    auto min_element = boost::range::min_element(
        states_, [](const std::unique_ptr<simgrid::mc::VisitedState>& a,
                    const std::unique_ptr<simgrid::mc::VisitedState>& b) { return a->num < b->num; });
    xbt_assert(min_element != states_.end());
    // and drop it:
    states_.erase(min_element);
    XBT_DEBUG("Remove visited state (maximum number of stored states reached)");
  }
}

/** @brief Checks whether a given state has already been visited by the algorithm. */
std::unique_ptr<simgrid::mc::VisitedState>
VisitedStates::addVisitedState(unsigned long state_number, simgrid::mc::State* graph_state, bool compare_snapshots)
{
  auto new_state             = std::make_unique<simgrid::mc::VisitedState>(state_number);
  graph_state->system_state_ = new_state->system_state;
  XBT_DEBUG("Snapshot %p of visited state %d (exploration stack state %d)", new_state->system_state.get(),
            new_state->num, graph_state->num_);

  auto range =
      boost::range::equal_range(states_, new_state.get(), simgrid::mc::DerefAndCompareByActorsCountAndUsedHeap());

  if (compare_snapshots)
    for (auto i = range.first; i != range.second; ++i) {
      auto& visited_state = *i;
      if (mcapi::get().snapshot_equal(visited_state->system_state.get(), new_state->system_state.get())) {
        // The state has been visited:

        std::unique_ptr<simgrid::mc::VisitedState> old_state =
          std::move(visited_state);

        if (old_state->original_num == -1) // I'm the copy of an original process
          new_state->original_num = old_state->num;
        else // I'm the copy of a copy
          new_state->original_num = old_state->original_num;

        if (dot_output == nullptr)
          XBT_DEBUG("State %d already visited ! (equal to state %d)",
                    new_state->num, old_state->num);
        else
          XBT_DEBUG("State %d already visited ! (equal to state %d (state %d in dot_output))",
                    new_state->num, old_state->num, new_state->original_num);

        /* Replace the old state with the new one (with a bigger num)
           (when the max number of visited states is reached,  the oldest
           one is removed according to its number (= with the min number) */
        XBT_DEBUG("Replace visited state %d with the new visited state %d",
          old_state->num, new_state->num);

        visited_state = std::move(new_state);
        return old_state;
      }
    }

  XBT_DEBUG("Insert new visited state %d (total : %lu)", new_state->num, (unsigned long) states_.size());
  states_.insert(range.first, std::move(new_state));
  this->prune();
  return nullptr;
}

}
}
