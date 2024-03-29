/* Copyright (c) 2016-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/DFSExplorer.hpp"
#include "simgrid/forward.h"
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

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_dfs, mc, "DFS exploration algorithm of the model-checker");

namespace simgrid::mc {

xbt::signal<void(RemoteApp&)> DFSExplorer::on_exploration_start_signal;

xbt::signal<void(State*, RemoteApp&)> DFSExplorer::on_state_creation_signal;

xbt::signal<void(Transition*, RemoteApp&)> DFSExplorer::on_transition_execute_signal;

xbt::signal<void(RemoteApp&)> DFSExplorer::on_log_state_signal;

RecordTrace DFSExplorer::get_record_trace() // override
{
  RecordTrace res;

  if (const auto trans = stack_->back()->get_transition_out(); trans != nullptr)
    res.push_back(trans.get());
  for (const auto* state = stack_->back().get(); state != nullptr; state = state->get_parent_state().get())
    if (state->get_transition_in() != nullptr)
      res.push_front(state->get_transition_in().get());

  return res;
}

void DFSExplorer::log_state() // override
{
  on_log_state_signal(get_remote_app());
  XBT_INFO("DFS exploration ended. %ld unique states visited; %lu explored traces (%lu transition replays, %lu states "
           "visited overall)",
           State::get_expanded_states(), explored_traces_, Transition::get_replayed_transitions(),
           visited_states_count_);
  Exploration::log_state();
}

void DFSExplorer::simgrid_wrapper_explore(odpor::Execution& S, aid_t next_actor, stack_t& state_stack)
{

  // This means the exploration asked us to visit a parallel history
  // So first, go to there in the application
  if (not is_execution_descending) {

    explored_traces_++;
    backtrack_to_state(state_stack.back().get());
    is_execution_descending = true;
  }

  auto state = state_stack.back();

  // If we use a state containing a sleep state, display it during debug
  if (XBT_LOG_ISENABLED(mc_dfs, xbt_log_priority_verbose)) {
    std::shared_ptr<SleepSetState> sleep_state = std::static_pointer_cast<SleepSetState>(state);
    if (sleep_state != nullptr and not sleep_state->get_sleep_set().empty()) {
      XBT_VERB("Sleep set actually containing:");

      for (const auto& [aid, transition] : sleep_state->get_sleep_set())
        XBT_VERB("  <%ld,%s>", aid, transition->to_string().c_str());
    }
  }

  XBT_DEBUG("Going to execute actor %ld", next_actor);
  auto transition_to_be_executed = state->get_actors_list().at(next_actor).get_transition();

  auto executed_transition = state->execute_next(next_actor, get_remote_app());
  on_transition_execute_signal(executed_transition.get(), get_remote_app());

  XBT_VERB("Executed %ld: %.60s (stack depth: %zu, state: %ld, %zu interleaves)", executed_transition->aid_,
           executed_transition->to_string().c_str(), state_stack.size(), state->get_num(), state->count_todo());

  auto next_state = reduction_algo_->state_create(get_remote_app(), state);
  if (_sg_mc_cached_states_interval > 0 && next_state->get_num() % _sg_mc_cached_states_interval == 0) {
    next_state->set_state_factory(get_remote_app().clone_checker_side());
  }
  on_state_creation_signal(next_state.get(), get_remote_app());

  state_stack.emplace_back(std::move(next_state));
  stack_ = &state_stack;

  S.push_transition(executed_transition);

  explore(S, state_stack);

  XBT_DEBUG("Backtracking from the exploration by one step");

  reduction_algo_->on_backtrack(state_stack.back().get());

  is_execution_descending = false;

  state_stack.pop_back();
  stack_ = &state_stack;

  S.remove_last_event();
}

void DFSExplorer::explore(odpor::Execution& S, stack_t& state_stack)
{

  visited_states_count_++;

  State* s = state_stack.back().get();

  reduction_algo_->races_computation(S, &state_stack);

  aid_t next_to_explore;

  while ((next_to_explore = reduction_algo_->next_to_explore(S, &state_stack)) != -1) {

    simgrid_wrapper_explore(S, next_to_explore, state_stack);
  }

  XBT_DEBUG("%lu actors remain, but none of them need to be interleaved (depth %zu).", s->get_actor_count(),
            state_stack.size() + 1);
  Exploration::check_deadlock();

  if (s->get_actor_count() == 0) {
    get_remote_app().finalize_app();
    XBT_VERB("Execution came to an end at %s", get_record_trace().to_string().c_str());
    XBT_VERB("(state: %ld, depth: %zu, %lu explored traces)", s->get_num(), state_stack.size(), backtrack_count_ + 1);
  }

  XBT_DEBUG("End of Exploration at depth %lu", S.size() + 1);
}

void DFSExplorer::run()
{
  XBT_INFO("Start a DFS exploration. Reduction is: %s.", to_c_str(reduction_mode_));

  auto initial_state = reduction_algo_->state_create(get_remote_app());
  on_state_creation_signal(initial_state.get(), get_remote_app());

  XBT_DEBUG("**************************************************");

  stack_t state_stack = std::deque<std::shared_ptr<State>>();
  state_stack.emplace_back(std::move(initial_state));

  odpor::Execution empty_seq = odpor::Execution();

  on_exploration_start_signal(get_remote_app());

  explore(empty_seq, state_stack);

  log_state();
}

DFSExplorer::DFSExplorer(const std::vector<char*>& args, ReductionMode mode) : Exploration(args), reduction_mode_(mode)
{

  if (reduction_mode_ == ReductionMode::dpor)
    reduction_algo_ = std::make_unique<DPOR>();
  else if (reduction_mode_ == ReductionMode::sdpor)
    reduction_algo_ = std::make_unique<SDPOR>();
  else if (reduction_mode_ == ReductionMode::odpor)
    reduction_algo_ = std::make_unique<ODPOR>();
  else {
    xbt_assert(reduction_mode_ == ReductionMode::none, "Reduction mode %s not supported yet by DFS explorer",
               to_c_str(reduction_mode_));
    reduction_algo_ = std::make_unique<NoReduction>();
  }
}

Exploration* create_dfs_exploration(const std::vector<char*>& args, ReductionMode mode)
{
  return new DFSExplorer(args, mode);
}

} // namespace simgrid::mc
