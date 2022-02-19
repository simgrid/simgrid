/* Copyright (c) 2016-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/checker/SafetyChecker.hpp"
#include "src/mc/Session.hpp"
#include "src/mc/VisitedState.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/mc_record.hpp"
#include "src/mc/transition/Transition.hpp"

#include "src/xbt/mmalloc/mmprivate.h"
#include "xbt/log.h"
#include "xbt/sysdep.h"

#include <cassert>
#include <cstdio>

#include <memory>
#include <string>
#include <vector>

using api = simgrid::mc::Api;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_safety, mc, "Logging specific to MC safety verification ");

namespace simgrid {
namespace mc {

xbt::signal<void()> SafetyChecker::on_exploration_start_signal;
xbt::signal<void()> SafetyChecker::on_backtracking_signal;

xbt::signal<void(State*)> SafetyChecker::on_state_creation_signal;

xbt::signal<void(State*)> SafetyChecker::on_restore_system_state_signal;
xbt::signal<void()> SafetyChecker::on_restore_initial_state_signal;
xbt::signal<void(Transition*)> SafetyChecker::on_transition_replay_signal;
xbt::signal<void(Transition*)> SafetyChecker::on_transition_execute_signal;

xbt::signal<void()> SafetyChecker::on_log_state_signal;

void SafetyChecker::check_non_termination(const State* current_state)
{
  for (auto state = stack_.rbegin(); state != stack_.rend(); ++state)
    if (api::get().snapshot_equal((*state)->system_state_.get(), current_state->system_state_.get())) {
      XBT_INFO("Non-progressive cycle: state %ld -> state %ld", (*state)->num_, current_state->num_);
      XBT_INFO("******************************************");
      XBT_INFO("*** NON-PROGRESSIVE CYCLE DETECTED ***");
      XBT_INFO("******************************************");
      XBT_INFO("Counter-example execution trace:");
      for (auto const& s : get_textual_trace())
        XBT_INFO("  %s", s.c_str());
      XBT_INFO("Path = %s", get_record_trace().to_string().c_str());
      log_state();

      throw TerminationError();
    }
}

RecordTrace SafetyChecker::get_record_trace() // override
{
  RecordTrace res;
  for (auto const& state : stack_)
    res.push_back(state->get_transition());
  return res;
}

std::vector<std::string> SafetyChecker::get_textual_trace() // override
{
  std::vector<std::string> trace;
  for (auto const& state : stack_)
    trace.push_back(state->get_transition()->to_string());
  return trace;
}

void SafetyChecker::log_state() // override
{
  on_log_state_signal();
  XBT_INFO("DFS exploration ended. %ld unique states visited; %ld backtracks (%lu transition replays, %lu states "
           "visited overall)",
           State::get_expanded_states(), backtrack_count_, api::get().mc_get_visited_states(),
           Transition::get_replayed_transitions());
}

void SafetyChecker::run()
{
  on_exploration_start_signal();
  /* This function runs the DFS algorithm the state space.
   * We do so iteratively instead of recursively, dealing with the call stack manually.
   * This allows one to explore the call stack at will. */

  while (not stack_.empty()) {
    /* Get current state */
    State* state = stack_.back().get();

    XBT_DEBUG("**************************************************");
    XBT_VERB("Exploration depth=%zu (state=%p, num %ld)(%zu interleave)", stack_.size(), state, state->num_,
             state->count_todo());

    api::get().mc_inc_visited_states();

    // Backtrack if we reached the maximum depth
    if (stack_.size() > (std::size_t)_sg_mc_max_depth) {
      if (reductionMode_ == ReductionMode::dpor) {
        XBT_ERROR("/!\\ Max depth of %d reached! THIS WILL PROBABLY BREAK the dpor reduction /!\\",
                  _sg_mc_max_depth.get());
        XBT_ERROR("/!\\ If bad things happen, disable dpor with --cfg=model-check/reduction:none /!\\");
      } else
        XBT_WARN("/!\\ Max depth reached ! /!\\ ");
      this->backtrack();
      continue;
    }

    // Backtrack if we are revisiting a state we saw previously
    if (visited_state_ != nullptr) {
      XBT_DEBUG("State already visited (equal to state %ld), exploration stopped on this path.",
                visited_state_->original_num == -1 ? visited_state_->num : visited_state_->original_num);

      visited_state_ = nullptr;
      this->backtrack();
      continue;
    }

    // Search for the next transition
    int next = state->next_transition();

    if (next < 0) { // If there is no more transition in the current state, backtrack.
      XBT_DEBUG("There remains %zu actors, but none to interleave (depth %zu).",
                mc_model_checker->get_remote_process().actors().size(), stack_.size() + 1);

      if (mc_model_checker->get_remote_process().actors().empty())
        mc_model_checker->finalize_app();
      this->backtrack();
      continue;
    }

    /* Actually answer the request: let's execute the selected request (MCed does one step) */
    state->execute_next(next);
    on_transition_execute_signal(state->get_transition());

    // If there are processes to interleave and the maximum depth has not been
    // reached then perform one step of the exploration algorithm.
    XBT_DEBUG("Execute: %s", state->get_transition()->to_string().c_str());

    std::string req_str;
    if (dot_output != nullptr)
      req_str = state->get_transition()->dot_string();

    /* Create the new expanded state (copy the state of MCed into our MCer data) */
    auto next_state = std::make_unique<State>();
    on_state_creation_signal(next_state.get());

    if (_sg_mc_termination)
      this->check_non_termination(next_state.get());

    /* Check whether we already explored next_state in the past (but only if interested in state-equality reduction) */
    if (_sg_mc_max_visited_states > 0)
      visited_state_ = visited_states_.addVisitedState(next_state->num_, next_state.get(), true);

    /* If this is a new state (or if we don't care about state-equality reduction) */
    if (visited_state_ == nullptr) {
      /* Get an enabled process and insert it in the interleave set of the next state */
      auto actors = api::get().get_actors();
      for (auto& remoteActor : actors) {
        auto actor = remoteActor.copy.get_buffer();
        if (get_session().actor_is_enabled(actor->get_pid())) {
          next_state->mark_todo(actor->get_pid());
          if (reductionMode_ == ReductionMode::dpor)
            break; // With DPOR, we take the first enabled transition
        }
      }

      if (dot_output != nullptr)
        std::fprintf(dot_output, "\"%ld\" -> \"%ld\" [%s];\n", state->num_, next_state->num_, req_str.c_str());

    } else if (dot_output != nullptr)
      std::fprintf(dot_output, "\"%ld\" -> \"%ld\" [%s];\n", state->num_,
                   visited_state_->original_num == -1 ? visited_state_->num : visited_state_->original_num,
                   req_str.c_str());

    stack_.push_back(std::move(next_state));
  }

  log_state();
}

void SafetyChecker::backtrack()
{
  backtrack_count_++;
  XBT_VERB("Backtracking from %s", get_record_trace().to_string().c_str());
  on_backtracking_signal();
  stack_.pop_back();

  session_singleton->check_deadlock();

  /* Traverse the stack backwards until a state with a non empty interleave set is found, deleting all the states that
   *  have it empty in the way. For each deleted state, check if the request that has generated it (from its
   *  predecessor state), depends on any other previous request executed before it. If it does then add it to the
   *  interleave set of the state that executed that previous request. */

  while (not stack_.empty()) {
    std::unique_ptr<State> state = std::move(stack_.back());
    stack_.pop_back();
    if (reductionMode_ == ReductionMode::dpor) {
      aid_t issuer_id = state->get_transition()->aid_;
      for (auto i = stack_.rbegin(); i != stack_.rend(); ++i) {
        State* prev_state = i->get();
        if (state->get_transition()->aid_ == prev_state->get_transition()->aid_) {
          XBT_DEBUG("Simcall >>%s<< and >>%s<< with same issuer %ld", state->get_transition()->to_string().c_str(),
                    prev_state->get_transition()->to_string().c_str(), issuer_id);
          break;
        } else if (prev_state->get_transition()->depends(state->get_transition())) {
          XBT_VERB("Dependent Transitions:");
          XBT_VERB("  %s (state=%ld)", prev_state->get_transition()->to_string().c_str(), prev_state->num_);
          XBT_VERB("  %s (state=%ld)", state->get_transition()->to_string().c_str(), state->num_);

          if (not prev_state->actor_states_[issuer_id].is_done())
            prev_state->mark_todo(issuer_id);
          else
            XBT_DEBUG("Actor %ld is in done set", issuer_id);
          break;
        } else {
          XBT_VERB("INDEPENDENT Transitions:");
          XBT_VERB("  %s (state=%ld)", prev_state->get_transition()->to_string().c_str(), prev_state->num_);
          XBT_VERB("  %s (state=%ld)", state->get_transition()->to_string().c_str(), state->num_);
        }
      }
    }

    if (state->count_todo() && stack_.size() < (std::size_t)_sg_mc_max_depth) {
      /* We found a back-tracking point, let's loop */
      XBT_DEBUG("Back-tracking to state %ld at depth %zu", state->num_, stack_.size() + 1);
      stack_.push_back(std::move(state));
      this->restore_state();
      XBT_DEBUG("Back-tracking to state %ld at depth %zu done", stack_.back()->num_, stack_.size());
      break;
    } else {
      XBT_DEBUG("Delete state %ld at depth %zu", state->num_, stack_.size() + 1);
    }
  }
}

void SafetyChecker::restore_state()
{
  /* If asked to rollback on a state that has a snapshot, restore it */
  State* last_state = stack_.back().get();
  if (last_state->system_state_) {
    api::get().restore_state(last_state->system_state_);
    on_restore_system_state_signal(last_state);
    return;
  }

  /* if no snapshot, we need to restore the initial state and replay the transitions */
  get_session().restore_initial_state();
  on_restore_initial_state_signal();

  /* Traverse the stack from the state at position start and re-execute the transitions */
  for (std::unique_ptr<State> const& state : stack_) {
    if (state == stack_.back()) // If we are arrived on the target state, don't replay the outgoing transition *.
      break;
    state->get_transition()->replay();
    on_transition_replay_signal(state->get_transition());
    /* Update statistics */
    api::get().mc_inc_visited_states();
  }
}

SafetyChecker::SafetyChecker(Session* session) : Checker(session)
{
  reductionMode_ = reduction_mode;
  if (_sg_mc_termination)
    reductionMode_ = ReductionMode::none;
  else if (reductionMode_ == ReductionMode::unset)
    reductionMode_ = ReductionMode::dpor;

  if (_sg_mc_termination)
    XBT_INFO("Check non progressive cycles");
  else
    XBT_INFO("Start a DFS exploration. Reduction is: %s.",
             (reductionMode_ == ReductionMode::none ? "none"
                                                    : (reductionMode_ == ReductionMode::dpor ? "dpor" : "unknown")));

  get_session().take_initial_snapshot();

  XBT_DEBUG("Starting the safety algorithm");

  auto initial_state = std::make_unique<State>();

  XBT_DEBUG("**************************************************");

  /* Get an enabled actor and insert it in the interleave set of the initial state */
  auto actors = api::get().get_actors();
  XBT_DEBUG("Initial state. %zu actors to consider", actors.size());
  for (auto& actor : actors) {
    aid_t aid = actor.copy.get_buffer()->get_pid();
    if (get_session().actor_is_enabled(aid)) {
      initial_state->mark_todo(aid);
      if (reductionMode_ == ReductionMode::dpor) {
        XBT_DEBUG("Actor %ld is TODO, DPOR is ON so let's go for this one.", aid);
        break;
      }
      XBT_DEBUG("Actor %ld is TODO", aid);
    }
  }

  stack_.push_back(std::move(initial_state));
}

Checker* create_safety_checker(Session* session)
{
  return new SafetyChecker(session);
}

} // namespace mc
} // namespace simgrid
