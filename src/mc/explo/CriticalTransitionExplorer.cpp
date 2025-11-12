/* Copyright (c) 2016-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/CriticalTransitionExplorer.hpp"
#include "simgrid/forward.h"
#include "src/mc/explo/DFSExplorer.hpp"
#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/explo/odpor/odpor_forward.hpp"
#include "src/mc/explo/reduction/DPOR.hpp"
#include "src/mc/explo/reduction/NoReduction.hpp"
#include "src/mc/explo/reduction/ODPOR.hpp"
#include "src/mc/explo/reduction/SDPOR.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/mc/mc_forward.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/mc_record.hpp"
#include "src/mc/remote/mc_protocol.h"
#include "src/mc/transition/Transition.hpp"

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

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(
    mc_ct, mc,
    "Critical transition exploration algorithm. For now, the answer is approximate when used with reduction");

namespace simgrid::mc {

void CriticalTransitionExplorer::log_end_exploration()
{
  XBT_INFO("*********************************");
  XBT_INFO("*** CRITICAL TRANSITION FOUND ***");
  XBT_INFO("*********************************");
  log_stack();
}

void CriticalTransitionExplorer::log_stack()
{
  XBT_INFO("Current knowledge of explored stack:");
  unsigned depth = 0;
  for (const auto& [state, out_transition] : initial_bugged_stack) {
    std::string status;
    if (state->has_correct_execution())
      status = "  CORRECT";
    else if (depth >= stack_->size() - 1)
      status = "INCORRECT";
    else
      status = "  UNKNOWN";
    if (out_transition == nullptr)
      return; // We reached the last state of the stack and nothing is going out of it
    XBT_INFO("  (%s) Actor %ld in %s ==> simcall: %s", status.c_str(), out_transition->aid_,
             out_transition->get_call_location().c_str(), out_transition->to_string(true).c_str());
    depth++;
  }
}

void CriticalTransitionExplorer::run()
{
  XBT_INFO("Start the critical transition detection phase.");
  XBT_DEBUG("**************************************************");

  odpor::Execution execution_seq_ = odpor::Execution();
  for (auto iter = std::next(stack_->begin()); iter != stack_->end(); ++iter) {
    execution_seq_.push_transition((*iter)->get_transition_in());
  }

  // If the stack is at its maximal it won't have any children
  // Let's do the exploration one more time so we finalize exploring it
  // In particular, this allows ODPOR to cleanup its things
  if (stack_->back()->get_transition_out() == nullptr) {
    reduction_algo_->on_backtrack(stack_->back().get());
    stack_->pop_back();
    execution_seq_.remove_last_event();
  }
  while (stack_->size() > 1 and not stack_->back()->has_correct_execution()) {
    auto current_candidate = stack_->back()->get_transition_out();

    XBT_VERB("Looking at depth %zu", stack_->size() - 1);
    if (XBT_LOG_ISENABLED(mc_ct, xbt_log_priority_verbose))
      log_stack();

    backtrack_to_state(stack_->back().get());
    is_execution_descending = true;
    try {
      XBT_DEBUG("Running the DFS exploration from after transition Actor %ld : %s", current_candidate->aid_,
                current_candidate->to_string().c_str());
      XBT_DEBUG("Execution sequence <%s>", execution_seq_.get_one_string_textual_trace().c_str());
      DFSExplorer::explore(execution_seq_, *stack_);
    } catch (McError& error) {
      xbt_assert(error.value == ExitStatus::SUCCESS);
      log_end_exploration();
      XBT_INFO("Found a correct execution of the programm!");
      XBT_INFO("Found the critical transition: Actor %ld ==> simcall: %s", current_candidate->aid_,
               current_candidate->to_string(true).c_str());
      return;
    }
    reduction_algo_->on_backtrack(stack_->back().get());
    stack_->pop_back();
    execution_seq_.remove_last_event();
  }

  log_end_exploration();
  if (stack_->size() == 1)
    XBT_INFO("The critical transition explorer reached the beginning of the stack without finding a correct execution. "
             "The program may have no correct behavior.");
  auto critical_transition = stack_->back()->get_transition_out();
  XBT_INFO("Found the critical transition: Actor %ld ==> simcall: %s", critical_transition->aid_,
           critical_transition->to_string(true).c_str());
  return;
}

CriticalTransitionExplorer::CriticalTransitionExplorer(std::unique_ptr<RemoteApp> remote_app, ReductionMode mode,
                                                       stack_t* stack)
    : DFSExplorer(std::move(remote_app), mode)
{
  stack_               = stack;
  for (auto const& state : get_stack())
    initial_bugged_stack.emplace_back(state, state->get_transition_out());
  run();
}

} // namespace simgrid::mc
