/* Copyright (c) 2016-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/DFSExplorer.hpp"
#include "src/mc/VisitedState.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/mc_record.hpp"
#include "src/mc/transition/Transition.hpp"

#include "src/xbt/mmalloc/mmprivate.h"
#include "xbt/log.h"
#include "xbt/string.hpp"
#include "xbt/sysdep.h"

#include <cassert>
#include <cstdio>

#include <memory>
#include <string>
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

      throw TerminationError();
    }
  }
}

RecordTrace DFSExplorer::get_record_trace() // override
{
  RecordTrace res;
  for (auto const& state : stack_)
    res.push_back(state->get_transition());
  return res;
}

std::vector<std::string> DFSExplorer::get_textual_trace() // override
{
  std::vector<std::string> trace;
  for (auto const& state : stack_) {
    const auto* t = state->get_transition();
    trace.push_back(xbt::string_printf("%ld: %s", t->aid_, t->to_string().c_str()));
  }
  return trace;
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
    State* state = stack_.back().get();

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

    // Backtrack if we are revisiting a state we saw previously while applying state-equality reduction
    if (visited_state_ != nullptr) {
      XBT_DEBUG("State already visited (equal to state %ld), exploration stopped on this path.",
                visited_state_->original_num_ == -1 ? visited_state_->num_ : visited_state_->original_num_);

      visited_state_ = nullptr;
      this->backtrack();
      continue;
    }

    // Search for the next transition
    // next_transition returns a pair<aid_t, double> in case we want to consider multiple state
    auto [next, _] = state->next_transition_guided();

    if (next < 0) { // If there is no more transition in the current state, backtrack.
      XBT_DEBUG("There remains %lu actors, but none to interleave (depth %zu).", state->get_actor_count(),
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
    state->execute_next(next, get_remote_app());
    on_transition_execute_signal(state->get_transition(), get_remote_app());

    // If there are processes to interleave and the maximum depth has not been
    // reached then perform one step of the exploration algorithm.
    XBT_VERB("Execute %ld: %.60s (stack depth: %zu, state: %ld, %zu interleaves)", state->get_transition()->aid_,
             state->get_transition()->to_string().c_str(), stack_.size(), state->get_num(), state->count_todo());

    /* Create the new expanded state (copy the state of MCed into our MCer data) */
    std::unique_ptr<State> next_state;

    next_state = std::make_unique<State>(get_remote_app(), state);
    on_state_creation_signal(next_state.get(), get_remote_app());

    /* Sleep set procedure:
     * adding the taken transition to the sleep set of the original state.
     * <!> Since the parent sleep set is used to compute the child sleep set, this need to be
     * done after next_state creation */
    XBT_DEBUG("Marking Transition >>%s<< of process %ld done and adding it to the sleep set",
              state->get_transition()->to_string().c_str(), state->get_transition()->aid_);
    state->add_sleep_set(state->get_transition()); // Actors are marked done when they are considerd in ActorState
    
    /* DPOR persistent set procedure:
     * for each new transition considered, check if it depends on any other previous transition executed before it
     * on another process. If there exists one, find the more recent, and add its process to the interleave set.
     * If the process is not enabled at this  point, then add every enabled process to the interleave */
    if (reduction_mode_ == ReductionMode::dpor) {
      aid_t issuer_id = state->get_transition()->aid_;
      for (auto i = stack_.rbegin(); i != stack_.rend(); ++i) {
        State* prev_state = i->get();
        if (state->get_transition()->aid_ == prev_state->get_transition()->aid_) {
          XBT_DEBUG("Simcall >>%s<< and >>%s<< with same issuer %ld", state->get_transition()->to_string().c_str(),
                    prev_state->get_transition()->to_string().c_str(), issuer_id);
          continue;
        } else if (prev_state->get_transition()->depends(state->get_transition())) {
          XBT_VERB("Dependent Transitions:");
          XBT_VERB("  %s (state=%ld)", prev_state->get_transition()->to_string().c_str(), prev_state->get_num());
          XBT_VERB("  %s (state=%ld)", state->get_transition()->to_string().c_str(), state->get_num());

          if (prev_state->is_actor_enabled(issuer_id)) {
            if (not prev_state->is_actor_done(issuer_id))
              prev_state->consider_one(issuer_id);
            else
              XBT_DEBUG("Actor %ld is already in done set: no need to explore it again", issuer_id);
          } else {
            XBT_DEBUG("Actor %ld is not enabled: DPOR may be failing. To stay sound, we are marking every enabled "
                      "transition as todo",
                      issuer_id);
            prev_state->consider_all();
          }
          break;
        } else {
          XBT_VERB("INDEPENDENT Transitions:");
          XBT_VERB("  %s (state=%ld)", prev_state->get_transition()->to_string().c_str(), prev_state->get_num());
          XBT_VERB("  %s (state=%ld)", state->get_transition()->to_string().c_str(), state->get_num());
        }
      }
    }

    if (_sg_mc_termination)
      this->check_non_termination(next_state.get());

    /* Check whether we already explored next_state in the past (but only if interested in state-equality reduction) */
    if (_sg_mc_max_visited_states > 0)
      visited_state_ = visited_states_.addVisitedState(next_state->get_num(), next_state.get(), get_remote_app());

    /* If this is a new state (or if we don't care about state-equality reduction) */
    if (visited_state_ == nullptr) {
      /* Get an enabled process and insert it in the interleave set of the next state */
      if (reduction_mode_ == ReductionMode::dpor)
        next_state->consider_best(); // Take only one transition if DPOR: others may be considered later if required
      else
        next_state->consider_all();

      dot_output("\"%ld\" -> \"%ld\" [%s];\n", state->get_num(), next_state->get_num(),
                 state->get_transition()->dot_string().c_str());
    } else
      dot_output("\"%ld\" -> \"%ld\" [%s];\n", state->get_num(),
                 visited_state_->original_num_ == -1 ? visited_state_->num_ : visited_state_->original_num_,
                 state->get_transition()->dot_string().c_str());

    stack_.push_back(std::move(next_state));
  }

  log_state();
}

void DFSExplorer::backtrack()
{
  backtrack_count_++;
  XBT_VERB("Backtracking from %s", get_record_trace().to_string().c_str());
  on_backtracking_signal(get_remote_app());
  get_remote_app().check_deadlock();

  /* We may backtrack from somewhere either because it's leaf, or because every enabled process are in done/sleep set.
   * In the first case, we need to remove the last transition corresponding to the Finalize */
  if (stack_.back()->get_transition()->aid_ == 0)
    stack_.pop_back();

  /* Traverse the stack backwards until a state with a non empty interleave set is found, deleting all the states that
   *  have it empty in the way. */
  bool found_backtracking_point = false;
  while (not stack_.empty() && not found_backtracking_point) {
    std::unique_ptr<State> state = std::move(stack_.back());

    stack_.pop_back();


    if (state->count_todo() == 0) { // Empty interleaving set: exploration at this level is over
      XBT_DEBUG("Delete state %ld at depth %zu", state->get_num(), stack_.size() + 1);

    } else {
      XBT_DEBUG("Back-tracking to state %ld at depth %zu: %lu transitions left to be explored", state->get_num(),
                stack_.size() + 1, state->count_todo());
      stack_.push_back(
          std::move(state)); // Put it back on the stack so we can explore the next transition of the interleave
      found_backtracking_point = true;
    }
  }

  if (found_backtracking_point) {
    /* If asked to rollback on a state that has a snapshot, restore it */
    State* last_state = stack_.back().get();
    if (const auto* system_state = last_state->get_system_state()) {
      system_state->restore(*get_remote_app().get_remote_process_memory());
      on_restore_system_state_signal(last_state, get_remote_app());
      return;
    }

    /* if no snapshot, we need to restore the initial state and replay the transitions */
    get_remote_app().restore_initial_state();
    on_restore_initial_state_signal(get_remote_app());

    /* Traverse the stack from the state at position start and re-execute the transitions */
    for (std::unique_ptr<State> const& state : stack_) {
      if (state == stack_.back()) /* If we are arrived on the target state, don't replay the outgoing transition */
        break;
      state->get_transition()->replay(get_remote_app());
      on_transition_replay_signal(state->get_transition(), get_remote_app());
      visited_states_count_++;
    }
  } // If no backtracing point, then the stack is empty and the exploration is over
}

DFSExplorer::DFSExplorer(const std::vector<char*>& args, bool with_dpor, bool need_memory_info)
    : Exploration(args, need_memory_info || _sg_mc_termination)
{
  if (with_dpor)
    reduction_mode_ = ReductionMode::dpor;
  else
    reduction_mode_ = ReductionMode::none;

  if (_sg_mc_termination) {
    if (with_dpor) {
      XBT_INFO("Check non progressive cycles (turning DPOR off)");
      reduction_mode_ = ReductionMode::none;
    } else {
      XBT_INFO("Check non progressive cycles");
    }
  } else
    XBT_INFO("Start a DFS exploration. Reduction is: %s.", to_c_str(reduction_mode_));

  auto initial_state = std::make_unique<State>(get_remote_app());

  XBT_DEBUG("**************************************************");

  /* Get an enabled actor and insert it in the interleave set of the initial state */
  XBT_DEBUG("Initial state. %lu actors to consider", initial_state->get_actor_count());
  if (reduction_mode_ == ReductionMode::dpor)
    initial_state->consider_best();
  else
    initial_state->consider_all();

  stack_.push_back(std::move(initial_state));
}

Exploration* create_dfs_exploration(const std::vector<char*>& args, bool with_dpor)
{
  return new DFSExplorer(args, with_dpor);
}

} // namespace simgrid::mc
