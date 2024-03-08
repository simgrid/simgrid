/* Copyright (c) 2016-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/OutOfOrderExplorer.hpp"
#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/mc/mc_forward.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/mc_record.hpp"
#include "src/mc/remote/mc_protocol.h"
#include "src/mc/transition/Transition.hpp"

#include "src/mc/explo/reduction/DPOR.hpp"
#include "src/mc/explo/reduction/NoReduction.hpp"
#include "src/mc/explo/reduction/Reduction.hpp"
#include "src/mc/explo/reduction/SDPOR.hpp"

#include "xbt/asserts.h"
#include "xbt/log.h"
#include "xbt/string.hpp"
#include "xbt/sysdep.h"

#include <cassert>
#include <cstdio>

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_ooo, mc, "Out-of-Order exploration algorithm of the model-checker");

namespace simgrid::mc {

xbt::signal<void(RemoteApp&)> OutOfOrderExplorer::on_exploration_start_signal;

xbt::signal<void(State*, RemoteApp&)> OutOfOrderExplorer::on_state_creation_signal;

xbt::signal<void(Transition*, RemoteApp&)> OutOfOrderExplorer::on_transition_execute_signal;

xbt::signal<void(RemoteApp&)> OutOfOrderExplorer::on_log_state_signal;

RecordTrace OutOfOrderExplorer::get_record_trace() // override
{
  RecordTrace res;

  if (const auto trans = stack_.back()->get_transition_out(); trans != nullptr)
    res.push_back(trans.get());
  for (const auto* state = stack_.back().get(); state != nullptr; state = state->get_parent_state().get())
    if (state->get_transition_in() != nullptr)
      res.push_front(state->get_transition_in().get());

  return res;
}

void OutOfOrderExplorer::restore_stack(std::shared_ptr<State> state)
{
  stack_.clear();
  execution_seq_     = odpor::Execution();
  auto current_state = state;
  stack_.emplace_front(current_state);
  // condition corresponds to reaching initial state
  while (current_state->get_parent_state() != nullptr) {
    current_state = current_state->get_parent_state();
    stack_.emplace_front(current_state);
  }
  XBT_DEBUG("Replaced stack by %s", get_record_trace().to_string().c_str());
  // NOTE: The outgoing transition for the top-most state of the  stack refers to that which was taken
  // as part of the last trace explored by the algorithm. Thus, only the sequence of transitions leading up to,
  // but not including, the last state must be included when reconstructing the Exploration for SDPOR.
  for (auto iter = std::next(stack_.begin()); iter != stack_.end(); ++iter) {
    execution_seq_.push_transition((*iter)->get_transition_in());
  }
}

void OutOfOrderExplorer::log_state() // override
{
  on_log_state_signal(get_remote_app());
  XBT_INFO(
      "Out-of-Order exploration ended. %ld unique states visited; %lu backtracks (%lu transition replays, %lu states "
      "visited overall)",
      State::get_expanded_states(), backtrack_count_, Transition::get_replayed_transitions(), visited_states_count_);
  Exploration::log_state();
}

void OutOfOrderExplorer::run()
{
  on_exploration_start_signal(get_remote_app());
  /* This function runs the Out-Of-Order algorithm the state space.
   * We do so iteratively instead of recursively, dealing with the call stack manually.
   * This allows one to explore the call stack at will. */

  while (not stack_.empty()) {
    /* Get current state */
    auto state = stack_.back();

    XBT_DEBUG("**************************************************");
    XBT_DEBUG("Exploration depth=%zu (state:#%ld; %zu interleaves todo)", stack_.size(), state->get_num(),
              state->count_todo());

    // Backtrack if we reached the maximum depth
    if (stack_.size() > (std::size_t)_sg_mc_max_depth) {
      if (reduction_mode_ == ReductionMode::dpor) {
        XBT_ERROR("/!\\ Max depth of %d reached! THIS WILL PROBABLY BREAK the dpor reduction /!\\",
                  _sg_mc_max_depth.get());
        XBT_ERROR("/!\\ If bad things happen, disable dpor with --cfg=model-check/reduction:none /!\\");
      } else if (reduction_mode_ == ReductionMode::sdpor || reduction_mode_ == ReductionMode::odpor) {
        XBT_ERROR("/!\\ Max depth of %d reached! THIS **WILL** BREAK the reduction, which is not sound "
                  "when stopping at a fixed depth /!\\",
                  _sg_mc_max_depth.get());
        XBT_ERROR("/!\\ If bad things happen, disable the reduction with --cfg=model-check/reduction:none /!\\");
      } else {
        XBT_WARN("/!\\ Max depth reached ! /!\\ ");
      }
      this->backtrack();
      continue;
    }

    // Search for the next transition
    // next_transition returns a pair<aid_t, int>
    // in case we want to consider multiple states (eg. during backtrack)
    const aid_t next = reduction_algo_->next_to_explore(execution_seq_, &stack_);

    if (next < 0) {

      // If there is no more transition in the current state (or if ODPOR picked an actor that is not enabled --
      // ReversibleRace is an overapproximation), backtrace
      XBT_VERB("%lu actors remain, but none of them need to be interleaved (depth %zu).", state->get_actor_count(),
               stack_.size() + 1);

      if (state->get_actor_count() == 0) {
        get_remote_app().finalize_app();
        XBT_VERB("Execution came to an end at %s (state: %ld, depth: %zu)", get_record_trace().to_string().c_str(),
                 state->get_num(), stack_.size());
      }

      this->backtrack();
      continue;
    }

    xbt_assert(state->is_actor_enabled(next));

    // If we use a state containing a sleep state, display it during debug
    if (XBT_LOG_ISENABLED(mc_ooo, xbt_log_priority_verbose)) {
      std::shared_ptr<SleepSetState> sleep_state = std::static_pointer_cast<SleepSetState>(state);
      if (sleep_state != nullptr and not sleep_state->get_sleep_set().empty()) {
        XBT_VERB("Sleep set actually containing:");

        for (const auto& [aid, transition] : sleep_state->get_sleep_set())
          XBT_VERB("  <%ld,%s>", aid, transition->to_string().c_str());
      }
    }

    auto todo = state->get_actors_list().at(next).get_transition();
    XBT_DEBUG("wanna execute %ld: %.60s", next, todo->to_string().c_str());

    /* Actually answer the request: let's execute the selected request (MCed does one step) */
    auto executed_transition = state->execute_next(next, get_remote_app());
    on_transition_execute_signal(state->get_transition_out().get(), get_remote_app());

    // If there are processes to interleave and the maximum depth has not been
    // reached then perform one step of the exploration algorithm.
    XBT_VERB("Executed %ld: %.60s (stack depth: %zu, state: %ld, %zu interleaves)", state->get_transition_out()->aid_,
             state->get_transition_out()->to_string().c_str(), stack_.size(), state->get_num(), state->count_todo());

    /* Create the new expanded state (copy the state of MCed into our MCer data) */
    auto next_state = reduction_algo_->state_create(get_remote_app(), state);
    on_state_creation_signal(next_state.get(), get_remote_app());

    visited_states_count_++;

    // Before leaving that state, if the transition we just took can be taken multiple times, we
    // need to give it to the opened states
    if (stack_.back()->count_todo_multiples() > 0)
      opened_states_.emplace_back(stack_.back());

    reduction_algo_->on_backtrack(state.get());

    stack_.emplace_back(std::move(next_state));
    execution_seq_.push_transition(std::move(executed_transition));

    reduction_algo_->races_computation(execution_seq_, &stack_, &opened_states_);

    dot_output("\"%ld\" -> \"%ld\" [%s];\n", state->get_num(), stack_.back()->get_num(),
               state->get_transition_out()->dot_string().c_str());
  }
  log_state();
}

std::shared_ptr<State> OutOfOrderExplorer::best_opened_state()
{
  int best_prio = 0; // cache the value for the best priority found so far (initialized to silence gcc)
  auto best     = end(opened_states_);   // iterator to the state to explore having the best priority
  auto valid    = begin(opened_states_); // iterator marking the limit between states still to explore, and already
                                         // explored ones

  // Keep only still non-explored states (aid != -1), and record the one with the best (greater) priority.
  for (auto current = begin(opened_states_); current != end(opened_states_); ++current) {
    auto [aid, prio] = (*current)->next_transition_guided();
    if (aid == -1)
      continue;
    if (valid != current)
      *valid = std::move(*current);
    if (best == end(opened_states_) || prio <= best_prio) {
      best_prio = prio;
      best      = valid;
    }
    ++valid;
  }

  std::shared_ptr<State> best_state;
  if (best < valid) {
    // There are non-explored states, and one of them has the best priority.  Remove it from opened_states_ before
    // returning.
    best_state = std::move(*best);
    --valid;
    if (best != valid)
      *best = std::move(*valid);
  }
  opened_states_.erase(valid, end(opened_states_));

  return best_state;
}

void OutOfOrderExplorer::backtrack()
{
  XBT_VERB("Backtracking from %s", get_record_trace().to_string().c_str());
  XBT_DEBUG("%lu alternatives are yet to be explored:", opened_states_.size());

  Exploration::check_deadlock();

  // Take the point with smallest distance
  auto backtracking_point = best_opened_state();

  // if no backtracking point, then set the stack_ to empty so we can end the exploration
  if (not backtracking_point) {
    XBT_DEBUG("No more opened point of exploration, the search will end");
    stack_.clear();
    return;
  }
  // We found a backtracking point, let's go to it
  backtrack_to_state(backtracking_point.get());
  this->restore_stack(backtracking_point);
}

OutOfOrderExplorer::OutOfOrderExplorer(const std::vector<char*>& args, ReductionMode mode)
    : Exploration(args), reduction_mode_(mode)
{

  if (reduction_mode_ == ReductionMode::dpor)
    reduction_algo_ = std::make_unique<DPOR>();
  else if (reduction_mode_ == ReductionMode::sdpor)
    reduction_algo_ = std::make_unique<SDPOR>();
  else {
    xbt_assert(reduction_mode_ == ReductionMode::none, "Reduction mode %s not supported yet by DFS explorer",
               to_c_str(reduction_mode_));
    reduction_algo_ = std::make_unique<NoReduction>();
  }

  XBT_INFO("Start an Out-of-Order exploration. Reduction is: %s.", to_c_str(reduction_mode_));

  auto initial_state = reduction_algo_->state_create(get_remote_app());

  XBT_DEBUG("**************************************************");

  stack_.emplace_back(std::move(initial_state));
  visited_states_count_++;

  /* Get an enabled actor and insert it in the interleave set of the initial state */
  XBT_DEBUG("Initial state. %lu actors to consider", stack_.back()->get_actor_count());
  if (stack_.back()->count_todo_multiples() > 1)
    opened_states_.emplace_back(stack_.back());
}

Exploration* create_out_of_order_exploration(const std::vector<char*>& args, ReductionMode mode)
{
  return new OutOfOrderExplorer(args, mode);
}

} // namespace simgrid::mc
