/* Copyright (c) 2011-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/VisitedState.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_private.hpp"

#include <unistd.h>
#include <sys/wait.h>
#include <memory>
#include <boost/range/algorithm.hpp>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_VisitedState, mc, "Logging specific to state equality detection mechanisms");

namespace simgrid::mc {

/** @brief Save the current state */
VisitedState::VisitedState(unsigned long state_number, unsigned int actor_count, RemoteApp& remote_app)
    : heap_bytes_used_(remote_app.get_remote_process_memory()->get_remote_heap_bytes())
    , actor_count_(actor_count)
    , num_(state_number)
{
  this->system_state_ = std::make_shared<simgrid::mc::Snapshot>(state_number, remote_app.get_page_store(),
                                                                *remote_app.get_remote_process_memory());
}

void VisitedStates::prune()
{
  while (states_.size() > (std::size_t)_sg_mc_max_visited_states) {
    XBT_DEBUG("Try to remove visited state (maximum number of stored states reached)");
    auto min_element = boost::range::min_element(
        states_, [](const std::unique_ptr<simgrid::mc::VisitedState>& a,
                    const std::unique_ptr<simgrid::mc::VisitedState>& b) { return a->num_ < b->num_; });
    xbt_assert(min_element != states_.end());
    // and drop it:
    states_.erase(min_element);
    XBT_DEBUG("Remove visited state (maximum number of stored states reached)");
  }
}

/** @brief Checks whether a given state has already been visited by the algorithm. */
std::unique_ptr<simgrid::mc::VisitedState>
VisitedStates::addVisitedState(unsigned long state_number, simgrid::mc::State* graph_state, RemoteApp& remote_app)
{
  auto new_state =
      std::make_unique<simgrid::mc::VisitedState>(state_number, graph_state->get_actor_count(), remote_app);

  graph_state->set_system_state(new_state->system_state_);
  XBT_DEBUG("Snapshot %p of visited state %ld (exploration stack state %ld)", new_state->system_state_.get(),
            new_state->num_, graph_state->get_num());

  auto [range_begin, range_end] = boost::range::equal_range(states_, new_state.get(), [](auto const& a, auto const& b) {
    return std::make_pair(a->actor_count_, a->heap_bytes_used_) < std::make_pair(b->actor_count_, b->heap_bytes_used_);
  });

  for (auto i = range_begin; i != range_end; ++i) {
    auto& visited_state = *i;
    if (visited_state->system_state_->equals_to(*new_state->system_state_.get(),
                                                *remote_app.get_remote_process_memory())) {
      // The state has been visited:

      std::unique_ptr<simgrid::mc::VisitedState> old_state = std::move(visited_state);

      if (old_state->original_num_ == -1) // I'm the copy of an original process
        new_state->original_num_ = old_state->num_;
      else // I'm the copy of a copy
        new_state->original_num_ = old_state->original_num_;

      XBT_DEBUG("State %ld already visited ! (equal to state %ld (state %ld in dot_output))", new_state->num_,
                old_state->num_, new_state->original_num_);

      /* Replace the old state with the new one (with a bigger num)
          (when the max number of visited states is reached,  the oldest
          one is removed according to its number (= with the min number) */
      XBT_DEBUG("Replace visited state %ld with the new visited state %ld", old_state->num_, new_state->num_);

      visited_state = std::move(new_state);
      return old_state;
    }
  }

  XBT_DEBUG("Insert new visited state %ld (total : %lu)", new_state->num_, (unsigned long)states_.size());
  states_.insert(range_begin, std::move(new_state));
  this->prune();
  return nullptr;
}

} // namespace simgrid::mc
