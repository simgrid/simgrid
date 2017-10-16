/* Copyright (c) 2016-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cassert>
#include <cstdio>

#include <memory>
#include <string>
#include <vector>

#include <xbt/log.h>
#include <xbt/sysdep.h>

#include "src/mc/Session.hpp"
#include "src/mc/Transition.hpp"
#include "src/mc/VisitedState.hpp"
#include "src/mc/checker/SafetyChecker.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/mc_record.hpp"
#include "src/mc/mc_request.hpp"
#include "src/mc/mc_smx.hpp"

#include "src/xbt/mmalloc/mmprivate.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_safety, mc,
                                "Logging specific to MC safety verification ");
namespace simgrid {
namespace mc {

static int snapshot_compare(simgrid::mc::State* state1, simgrid::mc::State* state2)
{
  simgrid::mc::Snapshot* s1 = state1->system_state.get();
  simgrid::mc::Snapshot* s2 = state2->system_state.get();
  int num1 = state1->num;
  int num2 = state2->num;
  return snapshot_compare(num1, s1, num2, s2);
}

void SafetyChecker::checkNonTermination(simgrid::mc::State* current_state)
{
  for (auto state = stack_.rbegin(); state != stack_.rend(); ++state)
    if (snapshot_compare(state->get(), current_state) == 0) {
      XBT_INFO("Non-progressive cycle: state %d -> state %d", (*state)->num, current_state->num);
      XBT_INFO("******************************************");
      XBT_INFO("*** NON-PROGRESSIVE CYCLE DETECTED ***");
      XBT_INFO("******************************************");
      XBT_INFO("Counter-example execution trace:");
      for (auto const& s : mc_model_checker->getChecker()->getTextualTrace())
        XBT_INFO("%s", s.c_str());
      simgrid::mc::session->logState();

      throw simgrid::mc::TerminationError();
    }
}

RecordTrace SafetyChecker::getRecordTrace() // override
{
  RecordTrace res;
  for (auto const& state : stack_)
    res.push_back(state->getTransition());
  return res;
}

std::vector<std::string> SafetyChecker::getTextualTrace() // override
{
  std::vector<std::string> trace;
  for (auto const& state : stack_) {
    int value = state->transition.argument;
    smx_simcall_t req = &state->executed_req;
    if (req)
      trace.push_back(simgrid::mc::request_to_string(
        req, value, simgrid::mc::RequestType::executed));
  }
  return trace;
}

void SafetyChecker::logState() // override
{
  XBT_INFO("Expanded states = %lu", expandedStatesCount_);
  XBT_INFO("Visited states = %lu", mc_model_checker->visited_states);
  XBT_INFO("Executed transitions = %lu", mc_model_checker->executed_transitions);
}

void SafetyChecker::run()
{
  /* This function runs the DFS algorithm the state space.
   * We do so iteratively instead of recursively, dealing with the call stack manually.
   * This allows to explore the call stack at wish. */

  while (not stack_.empty()) {

    /* Get current state */
    simgrid::mc::State* state = stack_.back().get();

    XBT_DEBUG("**************************************************");
    XBT_DEBUG("Exploration depth=%zu (state=%p, num %d)(%zu interleave)", stack_.size(), state, state->num,
              state->interleaveSize());

    mc_model_checker->visited_states++;

    // Backtrack if we reached the maximum depth
    if (stack_.size() > (std::size_t)_sg_mc_max_depth) {
      XBT_WARN("/!\\ Max depth reached ! /!\\ ");
      this->backtrack();
      continue;
    }

    // Backtrack if we are revisiting a state we saw previously
    if (visitedState_ != nullptr) {
      XBT_DEBUG("State already visited (equal to state %d), exploration stopped on this path.",
                visitedState_->original_num == -1 ? visitedState_->num : visitedState_->original_num);

      visitedState_ = nullptr;
      this->backtrack();
      continue;
    }

    // Search an enabled transition in the current state; backtrack if the interleave set is empty
    // get_request also sets state.transition to be the one corresponding to the returned req
    smx_simcall_t req = MC_state_get_request(state);
    // req is now the transition of the process that was selected to be executed

    if (req == nullptr) {
      XBT_DEBUG("There are no more processes to interleave. (depth %zu)", stack_.size() + 1);

      this->backtrack();
      continue;
    }

    // If there are processes to interleave and the maximum depth has not been
    // reached then perform one step of the exploration algorithm.
    XBT_DEBUG("Execute: %s",
      simgrid::mc::request_to_string(
        req, state->transition.argument, simgrid::mc::RequestType::simix).c_str());

    std::string req_str;
    if (dot_output != nullptr)
      req_str = simgrid::mc::request_get_dot_output(req, state->transition.argument);

    mc_model_checker->executed_transitions++;

    /* Actually answer the request: let execute the selected request (MCed does one step) */
    this->getSession().execute(state->transition);

    /* Create the new expanded state (copy the state of MCed into our MCer data) */
    std::unique_ptr<simgrid::mc::State> next_state =
        std::unique_ptr<simgrid::mc::State>(new simgrid::mc::State(++expandedStatesCount_));

    if (_sg_mc_termination)
      this->checkNonTermination(next_state.get());

    /* Check whether we already explored next_state in the past (but only if interested in state-equality reduction) */
    if (_sg_mc_max_visited_states > 0)
      visitedState_ = visitedStates_.addVisitedState(expandedStatesCount_, next_state.get(), true);

    /* If this is a new state (or if we don't care about state-equality reduction) */
    if (visitedState_ == nullptr) {

      /* Get an enabled process and insert it in the interleave set of the next state */
      for (auto& remoteActor : mc_model_checker->process().actors()) {
        auto actor = remoteActor.copy.getBuffer();
        if (simgrid::mc::actor_is_enabled(actor)) {
          next_state->addInterleavingSet(actor);
          if (reductionMode_ == simgrid::mc::ReductionMode::dpor)
            break; // With DPOR, we take the first enabled transition
        }
      }

      if (dot_output != nullptr)
        std::fprintf(dot_output, "\"%d\" -> \"%d\" [%s];\n",
          state->num, next_state->num, req_str.c_str());

    } else if (dot_output != nullptr)
      std::fprintf(dot_output, "\"%d\" -> \"%d\" [%s];\n", state->num,
                   visitedState_->original_num == -1 ? visitedState_->num : visitedState_->original_num,
                   req_str.c_str());

    stack_.push_back(std::move(next_state));
  }

  XBT_INFO("No property violation found.");
  simgrid::mc::session->logState();
}

void SafetyChecker::backtrack()
{
  stack_.pop_back();

  /* Check for deadlocks */
  if (mc_model_checker->checkDeadlock()) {
    MC_show_deadlock();
    throw simgrid::mc::DeadlockError();
  }

  /* Traverse the stack backwards until a state with a non empty interleave
     set is found, deleting all the states that have it empty in the way.
     For each deleted state, check if the request that has generated it
     (from it's predecessor state), depends on any other previous request
     executed before it. If it does then add it to the interleave set of the
     state that executed that previous request. */

  while (not stack_.empty()) {
    std::unique_ptr<simgrid::mc::State> state = std::move(stack_.back());
    stack_.pop_back();
    if (reductionMode_ == simgrid::mc::ReductionMode::dpor) {
      smx_simcall_t req = &state->internal_req;
      if (req->call == SIMCALL_MUTEX_LOCK || req->call == SIMCALL_MUTEX_TRYLOCK)
        xbt_die("Mutex is currently not supported with DPOR,  use --cfg=model-check/reduction:none");

      const smx_actor_t issuer = MC_smx_simcall_get_issuer(req);
      for (auto i = stack_.rbegin(); i != stack_.rend(); ++i) {
        simgrid::mc::State* prev_state = i->get();
        if (simgrid::mc::request_depend(req, &prev_state->internal_req)) {
          if (XBT_LOG_ISENABLED(mc_safety, xbt_log_priority_debug)) {
            XBT_DEBUG("Dependent Transitions:");
            int value = prev_state->transition.argument;
            smx_simcall_t prev_req = &prev_state->executed_req;
            XBT_DEBUG("%s (state=%d)",
              simgrid::mc::request_to_string(
                prev_req, value, simgrid::mc::RequestType::internal).c_str(),
              prev_state->num);
            value = state->transition.argument;
            prev_req = &state->executed_req;
            XBT_DEBUG("%s (state=%d)",
              simgrid::mc::request_to_string(
                prev_req, value, simgrid::mc::RequestType::executed).c_str(),
              state->num);
          }

          if (not prev_state->actorStates[issuer->pid].isDone())
            prev_state->addInterleavingSet(issuer);
          else
            XBT_DEBUG("Process %p is in done set", req->issuer);

          break;

        } else if (req->issuer == prev_state->internal_req.issuer) {

          XBT_DEBUG("Simcall %d and %d with same issuer", req->call, prev_state->internal_req.call);
          break;

        } else {

          const smx_actor_t previous_issuer = MC_smx_simcall_get_issuer(&prev_state->internal_req);
          XBT_DEBUG("Simcall %d, process %lu (state %d) and simcall %d, process %lu (state %d) are independent",
                    req->call, issuer->pid, state->num,
                    prev_state->internal_req.call,
                    previous_issuer->pid,
                    prev_state->num);

        }
      }
    }

    if (state->interleaveSize()
        && stack_.size() < (std::size_t) _sg_mc_max_depth) {
      /* We found a back-tracking point, let's loop */
      XBT_DEBUG("Back-tracking to state %d at depth %zu", state->num, stack_.size() + 1);
      stack_.push_back(std::move(state));
      this->restoreState();
      XBT_DEBUG("Back-tracking to state %d at depth %zu done", stack_.back()->num, stack_.size());
      break;
    } else {
      XBT_DEBUG("Delete state %d at depth %zu", state->num, stack_.size() + 1);
    }
  }
}

void SafetyChecker::restoreState()
{
  /* Intermediate backtracking */
  simgrid::mc::State* state = stack_.back().get();
  if (state->system_state) {
    simgrid::mc::restore_snapshot(state->system_state);
    return;
  }

  /* Restore the initial state */
  simgrid::mc::session->restoreInitialState();

  /* Traverse the stack from the state at position start and re-execute the transitions */
  for (std::unique_ptr<simgrid::mc::State> const& state : stack_) {
    if (state == stack_.back())
      break;
    session->execute(state->transition);
    /* Update statistics */
    mc_model_checker->visited_states++;
    mc_model_checker->executed_transitions++;
  }
}

SafetyChecker::SafetyChecker(Session& session) : Checker(session)
{
  reductionMode_ = simgrid::mc::reduction_mode;
  if (_sg_mc_termination)
    reductionMode_ = simgrid::mc::ReductionMode::none;
  else if (reductionMode_ == simgrid::mc::ReductionMode::unset)
    reductionMode_ = simgrid::mc::ReductionMode::dpor;

  if (_sg_mc_termination)
    XBT_INFO("Check non progressive cycles");
  else
    XBT_INFO("Check a safety property. Reduction is: %s.",
        (reductionMode_ == simgrid::mc::ReductionMode::none ? "none":
            (reductionMode_ == simgrid::mc::ReductionMode::dpor ? "dpor": "unknown")));
  simgrid::mc::session->initialize();

  XBT_DEBUG("Starting the safety algorithm");

  std::unique_ptr<simgrid::mc::State> initial_state =
      std::unique_ptr<simgrid::mc::State>(new simgrid::mc::State(++expandedStatesCount_));

  XBT_DEBUG("**************************************************");
  XBT_DEBUG("Initial state");

  /* Get an enabled actor and insert it in the interleave set of the initial state */
  for (auto& actor : mc_model_checker->process().actors())
    if (simgrid::mc::actor_is_enabled(actor.copy.getBuffer())) {
      initial_state->addInterleavingSet(actor.copy.getBuffer());
      if (reductionMode_ != simgrid::mc::ReductionMode::none)
        break;
    }

  stack_.push_back(std::move(initial_state));
}

Checker* createSafetyChecker(Session& session)
{
  return new SafetyChecker(session);
}

}
}
