/* Copyright (c) 2016-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/DFSExplorer.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/mc_record.hpp"
#include "src/mc/transition/Transition.hpp"

#if SIMGRID_HAVE_STATEFUL_MC
#include "src/mc/VisitedState.hpp"
#endif

#include "src/xbt/mmalloc/mmprivate.h"
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

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_dfs, mc, "DFS exploration algorithm of the model-checker");

namespace simgrid::mc {

xbt::signal<void(RemoteApp&)> DFSExplorer::on_exploration_start_signal;
xbt::signal<void(RemoteApp&)> DFSExplorer::on_backtracking_signal;

xbt::signal<void(State*, RemoteApp&)> DFSExplorer::on_state_creation_signal;

xbt::signal<void(State*, RemoteApp&)> DFSExplorer::on_restore_system_state_signal;
xbt::signal<void(RemoteApp&)> DFSExplorer::on_restore_initial_state_signal;
xbt::signal<void(Transition*, RemoteApp&)> DFSExplorer::on_transition_replay_signal;
xbt::signal<void(Transition*, RemoteApp&)> DFSExplorer::on_transition_execute_signal;

xbt::signal<void(RemoteApp&)> DFSExplorer::on_log_state_signal;

void DFSExplorer::check_non_termination(const State* current_state)
{
#if SIMGRID_HAVE_STATEFUL_MC
  for (auto const& state : stack_) {
    if (state->get_system_state()->equals_to(*current_state->get_system_state(),
                                             *get_remote_app().get_remote_process_memory())) {
      XBT_INFO("Non-progressive cycle: state %ld -> state %ld", state->get_num(), current_state->get_num());
      XBT_INFO("******************************************");
      XBT_INFO("*** NON-PROGRESSIVE CYCLE DETECTED ***");
      XBT_INFO("******************************************");
      XBT_INFO("Counter-example execution trace:");
      for (auto const& s : get_textual_trace())
        XBT_INFO("  %s", s.c_str());
      XBT_INFO("You can debug the problem (and see the whole details) by rerunning out of simgrid-mc with "
               "--cfg=model-check/replay:'%s'",
               get_record_trace().to_string().c_str());
      log_state();

      throw McError(ExitStatus::NON_TERMINATION);
    }
  }
#endif
}

RecordTrace DFSExplorer::get_record_trace() // override
{
  RecordTrace res;

  if (const auto trans = stack_.back()->get_transition_out(); trans != nullptr)
    res.push_back(trans.get());
  for (const auto* state = stack_.back().get(); state != nullptr; state = state->get_parent_state().get())
    if (state->get_transition_in() != nullptr)
      res.push_front(state->get_transition_in().get());

  return res;
}

void DFSExplorer::restore_stack(std::shared_ptr<State> state)
{
  stack_.clear();
  auto current_state = state;
  stack_.emplace_front(current_state);
  // condition corresponds to reaching initial state
  while (current_state->get_parent_state() != nullptr) {
    current_state = current_state->get_parent_state();
    stack_.emplace_front(current_state);
  }
  XBT_DEBUG("Replaced stack by %s", get_record_trace().to_string().c_str());

  if (reduction_mode_ == ReductionMode::sdpor) {
    execution_seq_ = sdpor::Execution();

    // NOTE: The outgoing transition for the top-most
    // state of the  stack refers to that which was taken
    // as part of the last trace explored by the algorithm.
    // Thus, only the sequence of transitions leading up to,
    // but not including, the last state must be included
    // when reconstructing the Exploration for SDPOR.
    for (auto iter = stack_.begin(); iter != stack_.end() - 1 and iter != stack_.end(); ++iter) {
      const auto& state = *(iter);
      execution_seq_.push_transition(state->get_transition_out().get());
    }
  }
  XBT_DEBUG("Additionally replaced corresponding SDPOR execution stack");
}

void DFSExplorer::log_state() // override
{
  on_log_state_signal(get_remote_app());
  XBT_INFO("DFS exploration ended. %ld unique states visited; %lu backtracks (%lu transition replays, %lu states "
           "visited overall)",
           State::get_expanded_states(), backtrack_count_, visited_states_count_,
           Transition::get_replayed_transitions());
  Exploration::log_state();
}

void DFSExplorer::run()
{
  on_exploration_start_signal(get_remote_app());
  /* This function runs the DFS algorithm the state space.
   * We do so iteratively instead of recursively, dealing with the call stack manually.
   * This allows one to explore the call stack at will. */

  while (not stack_.empty()) {
    /* Get current state */
    auto state = stack_.back();

    XBT_DEBUG("**************************************************");
    XBT_DEBUG("Exploration depth=%zu (state:#%ld; %zu interleaves todo)", stack_.size(), state->get_num(),
              state->count_todo());

    visited_states_count_++;

    // Backtrack if we reached the maximum depth
    if (stack_.size() > (std::size_t)_sg_mc_max_depth) {
      if (reduction_mode_ == ReductionMode::dpor) {
        XBT_ERROR("/!\\ Max depth of %d reached! THIS WILL PROBABLY BREAK the dpor reduction /!\\",
                  _sg_mc_max_depth.get());
        XBT_ERROR("/!\\ If bad things happen, disable dpor with --cfg=model-check/reduction:none /!\\");
      } else
        XBT_WARN("/!\\ Max depth reached ! /!\\ ");
      this->backtrack();
      continue;
    }

#if SIMGRID_HAVE_STATEFUL_MC
    // Backtrack if we are revisiting a state we saw previously while applying state-equality reduction
    if (visited_state_ != nullptr) {
      XBT_DEBUG("State already visited (equal to state %ld), exploration stopped on this path.",
                visited_state_->original_num_ == -1 ? visited_state_->num_ : visited_state_->original_num_);

      visited_state_ = nullptr;
      this->backtrack();
      continue;
    }
#endif

    // Search for the next transition
    // next_transition returns a pair<aid_t, int> in case we want to consider multiple state (eg. during backtrack)
    auto [next, _] = state->next_transition_guided();

    if (next < 0) { // If there is no more transition in the current state, backtrack.
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

    if (_sg_mc_sleep_set && XBT_LOG_ISENABLED(mc_dfs, xbt_log_priority_verbose)) {
      XBT_VERB("Sleep set actually containing:");
      for (auto& [aid, transition] : state->get_sleep_set())
        XBT_VERB("  <%ld,%s>", aid, transition.to_string().c_str());
    }

    /* Actually answer the request: let's execute the selected request (MCed does one step) */
    const auto executed_transition = state->execute_next(next, get_remote_app());
    on_transition_execute_signal(state->get_transition_out().get(), get_remote_app());

    // If there are processes to interleave and the maximum depth has not been
    // reached then perform one step of the exploration algorithm.
    XBT_VERB("Execute %ld: %.60s (stack depth: %zu, state: %ld, %zu interleaves)", state->get_transition_out()->aid_,
             state->get_transition_out()->to_string().c_str(), stack_.size(), state->get_num(), state->count_todo());

    /* Create the new expanded state (copy the state of MCed into our MCer data) */
    auto next_state = std::make_shared<State>(get_remote_app(), state);
    on_state_creation_signal(next_state.get(), get_remote_app());

    /* Sleep set procedure:
     * adding the taken transition to the sleep set of the original state.
     * <!> Since the parent sleep set is used to compute the child sleep set, this need to be
     * done after next_state creation */
    XBT_DEBUG("Marking Transition >>%s<< of process %ld done and adding it to the sleep set",
              state->get_transition_out()->to_string().c_str(), state->get_transition_out()->aid_);
    state->add_sleep_set(state->get_transition_out()); // Actors are marked done when they are considerd in ActorState

    /* DPOR persistent set procedure:
     * for each new transition considered, check if it depends on any other previous transition executed before it
     * on another process. If there exists one, find the more recent, and add its process to the interleave set.
     * If the process is not enabled at this  point, then add every enabled process to the interleave */
    if (reduction_mode_ == ReductionMode::dpor) {
      aid_t issuer_id   = state->get_transition_out()->aid_;
      stack_t tmp_stack = stack_;
      while (not tmp_stack.empty()) {
        if (const State* prev_state = tmp_stack.back().get();
            state->get_transition_out()->aid_ == prev_state->get_transition_out()->aid_) {
          XBT_DEBUG("Simcall >>%s<< and >>%s<< with same issuer %ld", state->get_transition_out()->to_string().c_str(),
                    prev_state->get_transition_out()->to_string().c_str(), issuer_id);
          tmp_stack.pop_back();
          continue;
        } else if (prev_state->get_transition_out()->depends(state->get_transition_out().get())) {
          XBT_VERB("Dependent Transitions:");
          XBT_VERB("  %s (state=%ld)", prev_state->get_transition_out()->to_string().c_str(), prev_state->get_num());
          XBT_VERB("  %s (state=%ld)", state->get_transition_out()->to_string().c_str(), state->get_num());

          if (prev_state->is_actor_enabled(issuer_id)) {
            if (not prev_state->is_actor_done(issuer_id)) {
              prev_state->consider_one(issuer_id);
              opened_states_.emplace_back(tmp_stack.back());
            } else
              XBT_DEBUG("Actor %ld is already in done set: no need to explore it again", issuer_id);
          } else {
            XBT_DEBUG("Actor %ld is not enabled: DPOR may be failing. To stay sound, we are marking every enabled "
                      "transition as todo",
                      issuer_id);
            // If we ended up marking at least a transition, explore it at some point
            if (prev_state->consider_all() > 0)
              opened_states_.emplace_back(tmp_stack.back());
          }
          break;
        } else {
          XBT_VERB("INDEPENDENT Transitions:");
          XBT_VERB("  %s (state=%ld)", prev_state->get_transition_out()->to_string().c_str(), prev_state->get_num());
          XBT_VERB("  %s (state=%ld)", state->get_transition_out()->to_string().c_str(), state->get_num());
        }
        tmp_stack.pop_back();
      }
    } else if (reduction_mode_ == ReductionMode::sdpor) {
      /**
       * SDPOR Source Set Procedure:
       *
       * Find "reversible races" in the current execution `E` with respect
       * to the latest action `p`. For each such race, determine one thread
       * not contained in the backtrack set at the "race point" `r` which
       * "represents" the trace formed by first executing everything after
       * `r` that doesn't depend on it (`v := notdep(r, E)`) and then `p` to
       * flip the race.
       *
       * The intuition is that some subsequence of `v` may enable `p`, so
       * we want to be sure that search "in that direction"
       */
      execution_seq_.push_transition(executed_transition.get());

      xbt_assert(execution_seq_.get_latest_event_handle().has_value(),
                 "No events are contained in the SDPOR/OPDPOR execution "
                 "even though one was just added");
      const aid_t p       = executed_transition->aid_;
      const auto next_E_p = execution_seq_.get_latest_event_handle().value();

      for (const auto racing_event_handle : execution_seq_.get_racing_events_of(next_E_p)) {
        // To determine if the race is reversible, we have to ensure
        // that actor `p` running `next_E_p` (viz. the event such that
        // `racing_event -> (E_p) next_E_p` and no other event
        // "happens-between" the two) is enabled in any equivalent
        // execution where `racing_event` happens before `next_E_p`.
        //
        // Importantly, it is equivalent to checking if in ANY
        // such equivalent execution sequence where `racing_event`
        // happens-before `next_E_p` that `p` is enabled in `pre(racing_event, E.p)`.
        // Thus it suffices to check THIS execution
        //
        // If the actor `p` is not enabled at s_[E'], it is not a *reversible* race
        const std::shared_ptr<State> prev_state = stack_[racing_event_handle];
        if (prev_state->is_actor_enabled(p)) {
          // NOTE: To incorporate the idea of attempting to select the "best"
          // backtrack point into SDPOR, instead of selecting the `first` initial,
          // we should instead compute all choices and decide which is best
          const std::optional<aid_t> q =
              execution_seq_.get_first_sdpor_initial_from(racing_event_handle, prev_state->get_backtrack_set());
          if (q.has_value()) {
            prev_state->consider_one(q.value());
            opened_states_.emplace_back(std::move(prev_state));
          }
        }
      }
    }

    // Before leaving that state, if the transition we just took can be taken multiple times, we
    // need to give it to the opened states
    if (stack_.back()->count_todo_multiples() > 0)
      opened_states_.emplace_back(stack_.back());

    if (_sg_mc_termination)
      this->check_non_termination(next_state.get());

#if SIMGRID_HAVE_STATEFUL_MC
    /* Check whether we already explored next_state in the past (but only if interested in state-equality reduction)
     */
    if (_sg_mc_max_visited_states > 0)
      visited_state_ = visited_states_.addVisitedState(next_state->get_num(), next_state.get(), get_remote_app());
#endif

    stack_.emplace_back(std::move(next_state));

    /* If this is a new state (or if we don't care about state-equality reduction) */
    if (visited_state_ == nullptr) {
      /* Get an enabled process and insert it in the interleave set of the next state */
      if (reduction_mode_ == ReductionMode::dpor)
        stack_.back()->consider_best(); // Take only one transition if DPOR: others may be considered later if required
      else {
        stack_.back()->consider_all();
      }

      dot_output("\"%ld\" -> \"%ld\" [%s];\n", state->get_num(), stack_.back()->get_num(),
                 state->get_transition_out()->dot_string().c_str());
#if SIMGRID_HAVE_STATEFUL_MC
    } else {
      dot_output("\"%ld\" -> \"%ld\" [%s];\n", state->get_num(),
                 visited_state_->original_num_ == -1 ? visited_state_->num_ : visited_state_->original_num_,
                 state->get_transition_out()->dot_string().c_str());
#endif
    }
  }
  log_state();
}

std::shared_ptr<State> DFSExplorer::best_opened_state()
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
    if (best == end(opened_states_) || prio > best_prio) {
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

void DFSExplorer::backtrack()
{
  XBT_VERB("Backtracking from %s", get_record_trace().to_string().c_str());
  XBT_DEBUG("%lu alternatives are yet to be explored:", opened_states_.size());

  on_backtracking_signal(get_remote_app());
  get_remote_app().check_deadlock();

  // Take the point with smallest distance
  auto backtracking_point = best_opened_state();

  // if no backtracking point, then set the stack_ to empty so we can end the exploration
  if (not backtracking_point) {
    XBT_DEBUG("No more opened point of exploration, the search will end");
    stack_.clear();
    return;
  }

  // We found a backtracking point, let's go to it
  backtrack_count_++;
  XBT_DEBUG("Backtracking to state#%ld", backtracking_point->get_num());

#if SIMGRID_HAVE_STATEFUL_MC
  /* If asked to rollback on a state that has a snapshot, restore it */
  if (const auto* system_state = backtracking_point->get_system_state()) {
    system_state->restore(*get_remote_app().get_remote_process_memory());
    on_restore_system_state_signal(backtracking_point.get(), get_remote_app());
    this->restore_stack(backtracking_point);
    return;
  }
#endif

  // Search how to restore the backtracking point
  State* init_state = nullptr;
  std::deque<Transition*> replay_recipe;
  for (auto* s = backtracking_point.get(); s != nullptr; s = s->get_parent_state().get()) {
#if SIMGRID_HAVE_STATEFUL_MC
    if (s->get_system_state() != nullptr) { // Found a state that I can restore
      init_state = s;
      break;
    }
#endif
    if (s->get_transition_in() != nullptr) // The root has no transition_in
      replay_recipe.push_front(s->get_transition_in().get());
  }

  // Restore the init_state, if any
  if (init_state != nullptr) {
#if SIMGRID_HAVE_STATEFUL_MC
    const auto* system_state = init_state->get_system_state();
    system_state->restore(*get_remote_app().get_remote_process_memory());
    on_restore_system_state_signal(init_state, get_remote_app());
#endif
  } else { // Restore the initial state if no intermediate state was found
    get_remote_app().restore_initial_state();
    on_restore_initial_state_signal(get_remote_app());
  }

  /* if no snapshot, we need to restore the initial state and replay the transitions */
  /* Traverse the stack from the state at position start and re-execute the transitions */
  for (auto& transition : replay_recipe) {
    transition->replay(get_remote_app());
    on_transition_replay_signal(transition, get_remote_app());
    visited_states_count_++;
  }
  this->restore_stack(backtracking_point);
}

DFSExplorer::DFSExplorer(const std::vector<char*>& args, ReductionMode mode, bool need_memory_info)
    : Exploration(args, need_memory_info || _sg_mc_termination
#if SIMGRID_HAVE_STATEFUL_MC
                            || _sg_mc_checkpoint > 0
#endif
                  )
    , reduction_mode_(mode)
{
  if (_sg_mc_termination) {
    if (mode != ReductionMode::none) {
      XBT_INFO("Check non progressive cycles (turning DPOR off)");
      reduction_mode_ = ReductionMode::none;
    } else {
      XBT_INFO("Check non progressive cycles");
    }
  } else
    XBT_INFO("Start a DFS exploration. Reduction is: %s.", to_c_str(reduction_mode_));

  auto initial_state = std::make_shared<State>(get_remote_app());

  XBT_DEBUG("**************************************************");

  stack_.emplace_back(std::move(initial_state));

  /* Get an enabled actor and insert it in the interleave set of the initial state */
  XBT_DEBUG("Initial state. %lu actors to consider", stack_.back()->get_actor_count());
  if (reduction_mode_ == ReductionMode::dpor)
    stack_.back()->consider_best();
  else {
    stack_.back()->consider_all();
  }
  if (stack_.back()->count_todo_multiples() > 1)
    opened_states_.emplace_back(stack_.back());
}

Exploration* create_dfs_exploration(const std::vector<char*>& args, ReductionMode mode)
{
  return new DFSExplorer(args, mode);
}

} // namespace simgrid::mc
