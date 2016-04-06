/* Copyright (c) 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cassert>
#include <cstdio>

#include <memory>
#include <string>
#include <vector>

#include <xbt/log.h>
#include <xbt/sysdep.h>

#include "src/mc/mc_state.h"
#include "src/mc/mc_request.h"
#include "src/mc/mc_safety.h"
#include "src/mc/mc_private.h"
#include "src/mc/mc_record.h"
#include "src/mc/mc_smx.h"
#include "src/mc/Client.hpp"
#include "src/mc/mc_exit.h"
#include "src/mc/Checker.hpp"
#include "src/mc/SafetyChecker.hpp"
#include "src/mc/VisitedState.hpp"

#include "src/xbt/mmalloc/mmprivate.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_safety, mc,
                                "Logging specific to MC safety verification ");
namespace simgrid {
namespace mc {

static void MC_show_non_termination(void)
{
  XBT_INFO("******************************************");
  XBT_INFO("*** NON-PROGRESSIVE CYCLE DETECTED ***");
  XBT_INFO("******************************************");
  XBT_INFO("Counter-example execution trace:");
  for (auto& s : mc_model_checker->getChecker()->getTextualTrace())
    XBT_INFO("%s", s.c_str());
  MC_print_statistics(mc_stats);
}

static int snapshot_compare(simgrid::mc::State* state1, simgrid::mc::State* state2)
{
  simgrid::mc::Snapshot* s1 = state1->system_state.get();
  simgrid::mc::Snapshot* s2 = state2->system_state.get();
  int num1 = state1->num;
  int num2 = state2->num;
  return snapshot_compare(num1, s1, num2, s2);
}

bool SafetyChecker::checkNonTermination(simgrid::mc::State* current_state)
{
  for (auto i = stack_.rbegin(); i != stack_.rend(); ++i)
    if (snapshot_compare(i->get(), current_state) == 0){
      XBT_INFO("Non-progressive cycle : state %d -> state %d",
        (*i)->num, current_state->num);
      return true;
    }
  return false;
}

RecordTrace SafetyChecker::getRecordTrace() // override
{
  RecordTrace res;
  for (auto const& state : stack_) {
    int value = 0;
    smx_simcall_t saved_req = MC_state_get_executed_request(state.get(), &value);
    const smx_process_t issuer = MC_smx_simcall_get_issuer(saved_req);
    const int pid = issuer->pid;
    res.push_back(RecordTraceElement(pid, value));
  }
  return res;
}

std::vector<std::string> SafetyChecker::getTextualTrace() // override
{
  std::vector<std::string> trace;
  for (auto const& state : stack_) {
    int value;
    smx_simcall_t req = MC_state_get_executed_request(state.get(), &value);
    if (req)
      trace.push_back(simgrid::mc::request_to_string(
        req, value, simgrid::mc::RequestType::executed));
  }
  return trace;
}

int SafetyChecker::run()
{
  this->init();

  int value;

  while (!stack_.empty()) {

    /* Get current state */
    simgrid::mc::State* state = stack_.back().get();

    XBT_DEBUG("**************************************************");
    XBT_DEBUG(
      "Exploration depth=%zi (state=%p, num %d)(%zu interleave)",
      stack_.size(), state, state->num,
      state->interleaveSize());

    mc_stats->visited_states++;

    // The interleave set is empty or the maximum depth is reached,
    // let's back-track.
    smx_simcall_t req = nullptr;
    if (stack_.size() > (std::size_t) _sg_mc_max_depth
        || (req = MC_state_get_request(state, &value)) == nullptr
        || visitedState_ != nullptr) {
      int res = this->backtrack();
      if (res)
        return res;
      else
        continue;
    }

    // If there are processes to interleave and the maximum depth has not been
    // reached then perform one step of the exploration algorithm.
    XBT_DEBUG("Execute: %s",
      simgrid::mc::request_to_string(
        req, value, simgrid::mc::RequestType::simix).c_str());

    std::string req_str;
    if (dot_output != nullptr)
      req_str = simgrid::mc::request_get_dot_output(req, value);

    mc_stats->executed_transitions++;

    // TODO, bundle both operations in a single message
    //   MC_execute_transition(req, value)

    /* Answer the request */
    simgrid::mc::handle_simcall(req, value);
    mc_model_checker->wait_for_requests();

    /* Create the new expanded state */
    std::unique_ptr<simgrid::mc::State> next_state =
      std::unique_ptr<simgrid::mc::State>(MC_state_new());

    if (_sg_mc_termination && this->checkNonTermination(next_state.get())) {
        MC_show_non_termination();
        return SIMGRID_MC_EXIT_NON_TERMINATION;
    }

    if (_sg_mc_visited == 0
        || (visitedState_ = visitedStates_.addVisitedState(next_state.get(), true)) == nullptr) {

      /* Get an enabled process and insert it in the interleave set of the next state */
      for (auto& p : mc_model_checker->process().simix_processes())
        if (simgrid::mc::process_is_enabled(&p.copy)) {
          MC_state_interleave_process(next_state.get(), &p.copy);
          if (reductionMode_ != simgrid::mc::ReductionMode::none)
            break;
        }

      if (dot_output != nullptr)
        std::fprintf(dot_output, "\"%d\" -> \"%d\" [%s];\n",
          state->num, next_state->num, req_str.c_str());

    } else if (dot_output != nullptr)
      std::fprintf(dot_output, "\"%d\" -> \"%d\" [%s];\n",
        state->num,
        visitedState_->other_num == -1 ? visitedState_->num : visitedState_->other_num, req_str.c_str());

    stack_.push_back(std::move(next_state));
  }

  XBT_INFO("No property violation found.");
  MC_print_statistics(mc_stats);
  initial_global_state = nullptr;
  return SIMGRID_MC_EXIT_SUCCESS;
}

int SafetyChecker::backtrack()
{
  if (stack_.size() > (std::size_t) _sg_mc_max_depth
      || visitedState_ != nullptr) {
    if (visitedState_ == nullptr)
      XBT_WARN("/!\\ Max depth reached ! /!\\ ");
    else
      XBT_DEBUG("State already visited (equal to state %d),"
        " exploration stopped on this path.",
        visitedState_->other_num == -1 ? visitedState_->num : visitedState_->other_num);
  } else
    XBT_DEBUG("There are no more processes to interleave. (depth %zi)",
      stack_.size() + 1);

  stack_.pop_back();

  visitedState_ = nullptr;

  /* Check for deadlocks */
  if (mc_model_checker->checkDeadlock()) {
    MC_show_deadlock();
    return SIMGRID_MC_EXIT_DEADLOCK;
  }

  /* Traverse the stack backwards until a state with a non empty interleave
     set is found, deleting all the states that have it empty in the way.
     For each deleted state, check if the request that has generated it 
     (from it's predecesor state), depends on any other previous request 
     executed before it. If it does then add it to the interleave set of the
     state that executed that previous request. */

  while (!stack_.empty()) {
    std::unique_ptr<simgrid::mc::State> state = std::move(stack_.back());
    stack_.pop_back();
    if (reductionMode_ == simgrid::mc::ReductionMode::dpor) {
      smx_simcall_t req = MC_state_get_internal_request(state.get());
      if (req->call == SIMCALL_MUTEX_LOCK || req->call == SIMCALL_MUTEX_TRYLOCK)
        xbt_die("Mutex is currently not supported with DPOR, "
          "use --cfg=model-check/reduction:none");
      const smx_process_t issuer = MC_smx_simcall_get_issuer(req);
      for (auto i = stack_.rbegin(); i != stack_.rend(); ++i) {
        simgrid::mc::State* prev_state = i->get();
        if (reductionMode_ != simgrid::mc::ReductionMode::none
            && simgrid::mc::request_depend(req, MC_state_get_internal_request(prev_state))) {
          if (XBT_LOG_ISENABLED(mc_safety, xbt_log_priority_debug)) {
            XBT_DEBUG("Dependent Transitions:");
            int value;
            smx_simcall_t prev_req = MC_state_get_executed_request(prev_state, &value);
            XBT_DEBUG("%s (state=%d)",
              simgrid::mc::request_to_string(
                prev_req, value, simgrid::mc::RequestType::internal).c_str(),
              prev_state->num);
            prev_req = MC_state_get_executed_request(state.get(), &value);
            XBT_DEBUG("%s (state=%d)",
              simgrid::mc::request_to_string(
                prev_req, value, simgrid::mc::RequestType::executed).c_str(),
              state->num);
          }

          if (!prev_state->processStates[issuer->pid].done())
            MC_state_interleave_process(prev_state, issuer);
          else
            XBT_DEBUG("Process %p is in done set", req->issuer);

          break;

        } else if (req->issuer == MC_state_get_internal_request(prev_state)->issuer) {

          XBT_DEBUG("Simcall %d and %d with same issuer", req->call, MC_state_get_internal_request(prev_state)->call);
          break;

        } else {

          const smx_process_t previous_issuer = MC_smx_simcall_get_issuer(MC_state_get_internal_request(prev_state));
          XBT_DEBUG("Simcall %d, process %lu (state %d) and simcall %d, process %lu (state %d) are independant",
                    req->call, issuer->pid, state->num,
                    MC_state_get_internal_request(prev_state)->call,
                    previous_issuer->pid,
                    prev_state->num);

        }
      }
    }

    if (state->interleaveSize()
        && stack_.size() < (std::size_t) _sg_mc_max_depth) {
      /* We found a back-tracking point, let's loop */
      XBT_DEBUG("Back-tracking to state %d at depth %zi",
        state->num, stack_.size() + 1);
      stack_.push_back(std::move(state));
      simgrid::mc::replay(stack_);
      XBT_DEBUG("Back-tracking to state %d at depth %zi done",
        stack_.back()->num, stack_.size());
      break;
    } else {
      XBT_DEBUG("Delete state %d at depth %zi",
        state->num, stack_.size() + 1);
    }
  }
  return SIMGRID_MC_EXIT_SUCCESS;
}

void SafetyChecker::init()
{
  reductionMode_ = simgrid::mc::reduction_mode;
  if( _sg_mc_termination)
    reductionMode_ = simgrid::mc::ReductionMode::none;
  else if (reductionMode_ == simgrid::mc::ReductionMode::unset)
    reductionMode_ = simgrid::mc::ReductionMode::dpor;

  if (_sg_mc_termination)
    XBT_INFO("Check non progressive cycles");
  else
    XBT_INFO("Check a safety property");
  mc_model_checker->wait_for_requests();

  XBT_DEBUG("Starting the safety algorithm");

  std::unique_ptr<simgrid::mc::State> initial_state =
    std::unique_ptr<simgrid::mc::State>(MC_state_new());

  XBT_DEBUG("**************************************************");
  XBT_DEBUG("Initial state");

  /* Wait for requests (schedules processes) */
  mc_model_checker->wait_for_requests();

  /* Get an enabled process and insert it in the interleave set of the initial state */
  for (auto& p : mc_model_checker->process().simix_processes())
    if (simgrid::mc::process_is_enabled(&p.copy)) {
      MC_state_interleave_process(initial_state.get(), &p.copy);
      if (reductionMode_ != simgrid::mc::ReductionMode::none)
        break;
    }

  stack_.push_back(std::move(initial_state));

  /* Save the initial state */
  initial_global_state = std::unique_ptr<s_mc_global_t>(new s_mc_global_t());
  initial_global_state->snapshot = simgrid::mc::take_snapshot(0);
}

SafetyChecker::SafetyChecker(Session& session) : Checker(session)
{
}

SafetyChecker::~SafetyChecker()
{
}

Checker* createSafetyChecker(Session& session)
{
  return new SafetyChecker(session);
}
  
}
}
