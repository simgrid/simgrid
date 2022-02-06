/* Copyright (c) 2016-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/Session.hpp"
#include "src/mc/Transition.hpp"
#include "src/mc/VisitedState.hpp"
#include "src/mc/checker/SafetyChecker.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/mc_record.hpp"

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

void SafetyChecker::check_non_termination(const State* current_state)
{
  for (auto state = stack_.rbegin(); state != stack_.rend(); ++state)
    if (api::get().snapshot_equal((*state)->system_state_.get(), current_state->system_state_.get())) {
      XBT_INFO("Non-progressive cycle: state %d -> state %d", (*state)->num_, current_state->num_);
      XBT_INFO("******************************************");
      XBT_INFO("*** NON-PROGRESSIVE CYCLE DETECTED ***");
      XBT_INFO("******************************************");
      XBT_INFO("Counter-example execution trace:");
      for (auto const& s : get_textual_trace())
        XBT_INFO("  %s", s.c_str());
      api::get().dump_record_path();
      api::get().log_state();

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
    trace.push_back(state->transition_.textual);
  return trace;
}

void SafetyChecker::log_state() // override
{
  XBT_INFO("Expanded states = %lu", expanded_states_count_);
  XBT_INFO("Visited states = %lu", api::get().mc_get_visited_states());
  XBT_INFO("Executed transitions = %lu", api::get().mc_get_executed_trans());
}

void SafetyChecker::run()
{
  /* This function runs the DFS algorithm the state space.
   * We do so iteratively instead of recursively, dealing with the call stack manually.
   * This allows one to explore the call stack at will. */

  while (not stack_.empty()) {
    /* Get current state */
    State* state = stack_.back().get();

    XBT_DEBUG("**************************************************");
    XBT_VERB("Exploration depth=%zu (state=%p, num %d)(%zu interleave)", stack_.size(), state, state->num_,
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
      XBT_DEBUG("State already visited (equal to state %d), exploration stopped on this path.",
                visited_state_->original_num == -1 ? visited_state_->num : visited_state_->original_num);

      visited_state_ = nullptr;
      this->backtrack();
      continue;
    }

    // Search for the next transition. If found, state is modified accordingly (transition and executed_req are set)
    // If there is no more transition in the current state, backtrack.

    if (not api::get().mc_state_choose_request(state)) {

      XBT_DEBUG("There remains %zu actors, but none to interleave (depth %zu).",
                mc_model_checker->get_remote_process().actors().size(), stack_.size() + 1);

      if (mc_model_checker->get_remote_process().actors().empty())
        mc_model_checker->finalize_app();
      this->backtrack();
      continue;
    }

    /* Actually answer the request: let execute the selected request (MCed does one step) */
    auto remote_observer = api::get().execute(state->transition_);

    // If there are processes to interleave and the maximum depth has not been
    // reached then perform one step of the exploration algorithm.
    XBT_DEBUG("Execute: %s", state->transition_.textual.c_str());

    std::string req_str;
    if (dot_output != nullptr)
      req_str = api::get().request_get_dot_output(state->transition_.aid_, state->transition_.times_considered_);

    api::get().mc_inc_executed_trans();

    /* Create the new expanded state (copy the state of MCed into our MCer data) */
    ++expanded_states_count_;
    auto next_state = std::make_unique<State>(expanded_states_count_);
    next_state->remote_observer_ = remote_observer;

    if (_sg_mc_termination)
      this->check_non_termination(next_state.get());

    /* Check whether we already explored next_state in the past (but only if interested in state-equality reduction) */
    if (_sg_mc_max_visited_states > 0)
      visited_state_ = visited_states_.addVisitedState(expanded_states_count_, next_state.get(), true);

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
        std::fprintf(dot_output, "\"%d\" -> \"%d\" [%s];\n", state->num_, next_state->num_, req_str.c_str());

    } else if (dot_output != nullptr)
      std::fprintf(dot_output, "\"%d\" -> \"%d\" [%s];\n", state->num_,
                   visited_state_->original_num == -1 ? visited_state_->num : visited_state_->original_num,
                   req_str.c_str());

    stack_.push_back(std::move(next_state));
  }

  XBT_INFO("No property violation found.");
  api::get().log_state();
}

void SafetyChecker::backtrack()
{
  stack_.pop_back();

  api::get().mc_check_deadlock();

  /* Traverse the stack backwards until a state with a non empty interleave set is found, deleting all the states that
   *  have it empty in the way. For each deleted state, check if the request that has generated it (from its
   *  predecessor state), depends on any other previous request executed before it. If it does then add it to the
   *  interleave set of the state that executed that previous request. */

  while (not stack_.empty()) {
    std::unique_ptr<State> state = std::move(stack_.back());
    stack_.pop_back();
    if (reductionMode_ == ReductionMode::dpor) {
      aid_t issuer_id = state->transition_.aid_;
      for (auto i = stack_.rbegin(); i != stack_.rend(); ++i) {
        State* prev_state = i->get();
        if (state->transition_.aid_ == prev_state->transition_.aid_) {
          XBT_DEBUG("Simcall >>%s<< and >>%s<< with same issuer %ld", state->transition_.textual.c_str(),
                    prev_state->transition_.textual.c_str(), issuer_id);
          break;
        } else if (api::get().requests_are_dependent(prev_state->remote_observer_, state->remote_observer_)) {
          if (XBT_LOG_ISENABLED(mc_safety, xbt_log_priority_debug)) {
            XBT_DEBUG("Dependent Transitions:");
            XBT_DEBUG("  %s (state=%d)", prev_state->transition_.textual.c_str(), prev_state->num_);
            XBT_DEBUG("  %s (state=%d)", state->transition_.textual.c_str(), state->num_);
          }

          if (not prev_state->actor_states_[issuer_id].is_done())
            prev_state->mark_todo(issuer_id);
          else
            XBT_DEBUG("Actor %ld is in done set", issuer_id);
          break;
        } else {
          if (XBT_LOG_ISENABLED(mc_safety, xbt_log_priority_debug)) {
            XBT_DEBUG("INDEPENDENT Transitions:");
            XBT_DEBUG("  %s (state=%d)", prev_state->transition_.textual.c_str(), prev_state->num_);
            XBT_DEBUG("  %s (state=%d)", state->transition_.textual.c_str(), state->num_);
          }
        }
      }
    }

    if (state->count_todo() && stack_.size() < (std::size_t)_sg_mc_max_depth) {
      /* We found a back-tracking point, let's loop */
      XBT_DEBUG("Back-tracking to state %d at depth %zu", state->num_, stack_.size() + 1);
      stack_.push_back(std::move(state));
      this->restore_state();
      XBT_DEBUG("Back-tracking to state %d at depth %zu done", stack_.back()->num_, stack_.size());
      break;
    } else {
      XBT_DEBUG("Delete state %d at depth %zu", state->num_, stack_.size() + 1);
    }
  }
}

void SafetyChecker::restore_state()
{
  /* Intermediate backtracking */
  const State* last_state = stack_.back().get();
  if (last_state->system_state_) {
    api::get().restore_state(last_state->system_state_);
    return;
  }

  get_session().restore_initial_state();

  /* Traverse the stack from the state at position start and re-execute the transitions */
  for (std::unique_ptr<State> const& state : stack_) {
    if (state == stack_.back())
      break;
    api::get().execute(state->transition_);
    /* Update statistics */
    api::get().mc_inc_visited_states();
    api::get().mc_inc_executed_trans();
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
    XBT_INFO("Check a safety property. Reduction is: %s.",
             (reductionMode_ == ReductionMode::none ? "none"
                                                    : (reductionMode_ == ReductionMode::dpor ? "dpor" : "unknown")));

  get_session().take_initial_snapshot();

  XBT_DEBUG("Starting the safety algorithm");

  ++expanded_states_count_;
  auto initial_state = std::make_unique<State>(expanded_states_count_);

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
