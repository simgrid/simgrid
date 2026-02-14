/* Copyright (c) 2016-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/DFSExplorer.hpp"
#include "simgrid/forward.h"
#include "src/mc/api/states/SoftLockedState.hpp"
#include "src/mc/explo/ReductedExplorer.hpp"
#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/explo/odpor/odpor_forward.hpp"
#include "src/mc/explo/reduction/DPOR.hpp"
#include "src/mc/explo/reduction/NoReduction.hpp"
#include "src/mc/explo/reduction/ODPOR.hpp"
#include "src/mc/explo/reduction/Reduction.hpp"
#include "src/mc/explo/reduction/SDPOR.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/mc/mc_forward.hpp"
#include "src/mc/mc_record.hpp"
#include "src/mc/remote/mc_protocol.h"
#include "src/mc/transition/Transition.hpp"

#include "xbt/asserts.h"
#include "xbt/log.h"

#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_dfs, mc, "DFS exploration algorithm of the model-checker");

namespace simgrid::mc {

RecordTrace DFSExplorer::get_record_trace() // override
{
  RecordTrace res;

  // Programm did nothing or crashed immediatly, nothing to see here
  if (stack_->size() == 0)
    return res;

  if (const auto trans = stack_->back()->get_transition_out(); trans != nullptr)
    res.push_back(trans.get());
  for (const auto* state = stack_->back().get(); state != nullptr; state = state->get_parent_state())
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
           static_cast<unsigned long>(visited_states_count_));
  Exploration::log_state();
}

void DFSExplorer::step_exploration(odpor::Execution& S, aid_t next_actor, stack_t& state_stack)
{

  // This means the exploration asked us to visit a parallel history
  // So first, go to there in the application
  if (not is_execution_descending) {

    backtrack_to_state(state_stack.back().get());
    is_execution_descending = true;
  }

  auto state = state_stack.back();

  // If we use a state containing a sleep state, display it during debug
  if (XBT_LOG_ISENABLED(mc_dfs, xbt_log_priority_verbose) && reduction_->get_kind() != ReductionMode::none) {
    auto sleep_state = static_cast<SleepSetState*>(state.get());
    if (not sleep_state->get_sleep_set().empty()) {
      XBT_DEBUG("Sleep set actually containing:");

      for (const auto& [aid, transition] : sleep_state->get_sleep_set())
        XBT_DEBUG("  <%ld,%s>", aid, transition->to_string().c_str());
    }
  }

  XBT_DEBUG("Going to execute actor %ld", next_actor);

  TransitionPtr executed_transition;
  StatePtr next_state;
  try {
    executed_transition = state->execute_next(next_actor, get_remote_app());
    on_transition_execute_signal(executed_transition.get(), get_remote_app());
    XBT_VERB("Executed %ld: %.60s (stack depth: %zu, state: %lu, %zu interleaves)", executed_transition->aid_,
             executed_transition->to_string().c_str(), state_stack.size(), state->get_num(), state->count_todo());

    next_state = reduction_->state_create(get_remote_app(), state, executed_transition);

    if (_sg_mc_cached_states_interval > 0 && next_state->get_num() % _sg_mc_cached_states_interval == 0) {
      next_state->set_state_factory(get_remote_app().clone_checker_side());
    }
  } catch (McWarning& error) {
    // If an error is reached while executing the transition ...
    if (XBT_LOG_ISENABLED(mc_dfs, xbt_log_priority_debug)) {
      auto transition_to_be_executed = state->get_actor_at(next_actor).get_transition();
      XBT_DEBUG("An error occured while executing %ld: %.60s (stack depth: %zu, state: %lu, %zu interleaves)",
                transition_to_be_executed->aid_, transition_to_be_executed->to_string().c_str(), state_stack.size(),
                state->get_num(), state->count_todo());
    }

    // ... reset the application to the step before dying
    // ... the normal backtrack style would be upset, because currently the application is dead
    backtrack_to_state(state.get(), false);

    // ... construct a fake state with no successors so the reduction think it is time to compute races
    executed_transition = TransitionPtr(new Transition(Transition::Type::UNKNOWN, next_actor, 0));
    next_state          = StatePtr(new SoftLockedState(get_remote_app(), state, executed_transition));

    // ... Add that fake state and compute races
    state_stack.emplace_back(std::move(next_state));
    stack_ = &state_stack;
    S.push_transition(executed_transition);
    Reduction::RaceUpdate* todo_updates = reduction_->races_computation(S, stack_);
    reduction_->apply_race_update(get_remote_app(), todo_updates);
    reduction_->delete_race_update(todo_updates);

    // ... If we are not already doing it, start critical exploration
    run_critical_exploration_on_need(error.value);

    // ... Unwind the addition of this fake state and prepare to let the algo keep going
    reduction_->on_backtrack(state_stack.back().get());

    is_execution_descending = false;

    state_stack.pop_back();
    stack_ = &state_stack;

    S.remove_last_event();

    // ... reduction trick to keep it sound
    reduction_->consider_best(state);

    return;
  }

  on_state_creation_signal(next_state.get(), get_remote_app());

  state_stack.emplace_back(std::move(next_state));
  stack_ = &state_stack;

  S.push_transition(executed_transition);

  // Backtrack if we reached the maximum depth
  if (state_stack.size() > (std::size_t)_sg_mc_max_depth) {
    if (reduction_->get_kind() == ReductionMode::dpor) {
      XBT_ERROR("/!\\ Max depth of %d reached! THIS WILL PROBABLY BREAK the dpor reduction /!\\",
                _sg_mc_max_depth.get());
      XBT_ERROR("/!\\ If bad things happen, disable dpor with --cfg=model-check/reduction:none /!\\");
    } else if (reduction_->get_kind() == ReductionMode::sdpor || reduction_->get_kind() == ReductionMode::odpor) {
      XBT_WARN("/!\\ Max depth of %d reached! THIS **WILL** BREAK the reduction, which is not sound "
               "when stopping at a fixed depth /!\\",
               _sg_mc_max_depth.get());
      XBT_WARN("/!\\ If bad things happen, disable the reduction with --cfg=model-check/reduction:none /!\\");
    } else {
      XBT_WARN("/!\\ Max depth reached ! /!\\ ");
    }
    XBT_DEBUG("Finalizing App ASAP to explore somewhere else");
    get_remote_app().finalize_app(true);

    backtrack_to_state(stack_->back().get(), false);

    stack_->back()->mark_as_leaf();
  }

  explore(S, state_stack);

  XBT_DEBUG("Backtracking from the exploration by one step");

  reduction_->on_backtrack(state_stack.back().get());
  State::garbage_collect();
  is_execution_descending = false;

  state_stack.pop_back();
  stack_ = &state_stack;

  S.remove_last_event();
}

void DFSExplorer::explore(odpor::Execution& S, stack_t& state_stack)
{
  visited_states_count_++;

  State* s = state_stack.back().get();

  aid_t next_to_explore;

  while ((next_to_explore = reduction_->next_to_explore(S, &state_stack)) != -1) {

    step_exploration(S, next_to_explore, state_stack);
  }

  Reduction::RaceUpdate* todo_updates = reduction_->races_computation(S, &state_stack);
  reduction_->apply_race_update(get_remote_app(), todo_updates);
  reduction_->delete_race_update(todo_updates);

  XBT_DEBUG("%lu actors remain, but none of them need to be interleaved (depth %zu).", s->get_actor_count(),
            state_stack.size() + 1);
  Exploration::check_deadlock();

  if (s->get_actor_count() == 0) {
    explored_traces_++;
    get_remote_app().finalize_app();
    XBT_VERB("Execution came to an end at %.100s", get_record_trace().to_string().c_str());
    XBT_VERB("(state: %lu, depth: %zu, %lu explored traces)", s->get_num(), state_stack.size(), backtrack_count_ + 1);

    state_stack.back()->on_branch_completion();
    if (_sg_mc_eta_steps != 0 and explored_traces_ % abs(_sg_mc_eta_steps) == 0)
      XBT_INFO("Explored a total of %.5e/%.5e states. Hence %3.2f completion rate",
               (double)State::get_expanded_states(), state_stack.front()->get_expected_total_children(),
               100 * (double)State::get_expanded_states() / state_stack.front()->get_expected_total_children());

    report_correct_execution(s);
    if (_sg_mc_debug_optimality)
      odpor::MazurkiewiczTraces::record_new_execution(S);
  }
  s->mark_to_delete();

  XBT_DEBUG("End of Exploration at depth %lu", S.size() + 1);
}

void DFSExplorer::run()
{
  XBT_INFO("Start a DFS exploration. Reduction is: %s.", to_c_str(reduction_->get_kind()));

  auto initial_state = reduction_->state_create(get_remote_app());
  on_state_creation_signal(initial_state.get(), get_remote_app());

  XBT_DEBUG("**************************************************");

  static stack_t state_stack = std::deque<StatePtr>();
  state_stack.emplace_back(std::move(initial_state));

  odpor::Execution empty_seq = odpor::Execution();

  on_exploration_start_signal(get_remote_app());

  explore(empty_seq, state_stack);

  log_state();
}

DFSExplorer::DFSExplorer(std::unique_ptr<RemoteApp> remote_app, std::unique_ptr<Reduction> reduction)
    : ReductedExplorer(std::move(remote_app), std::move(reduction))
{
  auto mode = reduction_->get_kind();
  xbt_assert(mode == ReductionMode::dpor || mode == ReductionMode::sdpor || mode == ReductionMode::odpor ||
                 mode == ReductionMode::none,
             "Reduction mode %s not supported yet by DFS explorer", to_c_str(mode));
}

DFSExplorer::DFSExplorer(const std::vector<char*>& args, std::unique_ptr<Reduction> reduction)
    : ReductedExplorer(std::move(reduction))
{
  Exploration::initialize_remote_app(args);

  auto mode = reduction_->get_kind();
  xbt_assert(mode == ReductionMode::dpor || mode == ReductionMode::sdpor || mode == ReductionMode::odpor ||
                 mode == ReductionMode::none,
             "Reduction mode %s not supported yet by DFS explorer", to_c_str(mode));
}

Exploration* create_dfs_exploration(const std::vector<char*>& args, std::unique_ptr<Reduction> reduction)
{
  return new DFSExplorer(args, std::move(reduction));
}

} // namespace simgrid::mc
