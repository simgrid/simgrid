/* Copyright (c) 2016-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/ParallelizedExplorer.hpp"
#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/explo/reduction/ODPOR.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/mc/mc_forward.hpp"
#include "src/mc/mc_record.hpp"

#include "src/mc/explo/reduction/DPOR.hpp"
#include "src/mc/explo/reduction/NoReduction.hpp"
#include "src/mc/explo/reduction/Reduction.hpp"
#include "src/mc/explo/reduction/SDPOR.hpp"

#include "xbt/asserts.h"
#include "xbt/log.h"

#include <cassert>
#include <cstdio>

#include <algorithm>
#include <exception>
#include <functional>
#include <limits>
#include <locale>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_parallel, mc, "Parallel exploration algorithm of the model-checker");

namespace simgrid::mc {

int ThreadLocalExplorer::explorer_count = 0;

RecordTrace ParallelizedExplorer::get_record_trace()
{
  return {}; // Is it required here?
}

RecordTrace get_record_trace_from_stack(stack_t& stack)
{
  RecordTrace res;

  if (const auto trans = stack.back()->get_transition_out(); trans != nullptr)
    res.push_back(trans.get());
  for (const auto* state = stack.back().get(); state != nullptr; state = state->get_parent_state())
    if (state->get_transition_in() != nullptr)
      res.push_front(state->get_transition_in().get());

  return res;
}

void ParallelizedExplorer::TreeHandler()
{
  long traces_count = 0;
  // This local counter will repreent the number of things in opened_heads_ + the number of currently working Explorer
  int remaining_todo = 1; // The intial state

  // While there is something in opened_heads_ OR an Explorer is working
  while (remaining_todo > 0) {

    XBT_DEBUG("[tid:TreeHandler] New round of tree handling! There are currently %d remaining todo", remaining_todo);

    Reduction::RaceUpdate* to_apply = races_list_.pop();

    if (to_apply == nullptr) {
      // An Explorer reached a bug! We need to terminate the exploration ASAP
      XBT_DEBUG("[tid:TreeHandler] The exploration has been terminated by an explorer");
      break;
    }

    // The race correspond to an explorer finishing, so we decrement the count
    remaining_todo--;
    XBT_DEBUG("[tid:TreeHandler] Received a race update! Going to apply it");

    std::vector<StatePtr> new_opened;
    remaining_todo +=
        reduction_algo_->apply_race_update(Exploration::get_instance()->get_remote_app(), to_apply, &new_opened);
    XBT_DEBUG("[tid:TreeHandler] The update contained %lu new states, so now there are %d remaining todo",
              new_opened.size(), remaining_todo);

    for (auto state_it = new_opened.begin(); state_it != new_opened.end(); state_it++)
      opened_heads_.push((*state_it).get());

    if (to_apply->get_last_explored_state() != nullptr)
      to_apply->get_last_explored_state()->mark_to_delete();
    reduction_algo_->delete_race_update(to_apply);

    State::garbage_collect();
    traces_count++;
    if (traces_count % 10 == 0)
      XBT_INFO("About %ld traces have been explored so far. Remaining todo: %d", traces_count, remaining_todo);

    if (traces_count % 1000 == 0) {
      // State::update_leftness();
      // opened_heads_.sort();
    }
  }

  for (int i = 0; i < number_of_threads; i++)
    opened_heads_.push(nullptr);
}

void ThreadLocalExplorer::backtrack_to_state(State* to_visit)
{
  Exploration::get_instance()->backtrack_remote_app_to_state(get_remote_app(), to_visit);
}

void ThreadLocalExplorer::check_deadlock()
{
  if (get_remote_app().check_deadlock()) {
    throw McWarning(ExitStatus::DEADLOCK);
  }
}

void ExplorerHandler(ThreadLocalExplorer* local_explorer, ExitStatus* exploration_result)
{
  XBT_DEBUG("Lauching Explorer %d", local_explorer->get_explorer_id());

  // Local explorer containing the remote app for this thread
  // ThreadLocalExplorer local_explorer(args);
  // local_explorer.reduction_algo = reduction_algo_;
  // local_explorer.opened_heads   = opened_heads_;
  // local_explorer.races_list     = races_list_;

  XBT_DEBUG("[tid:Explorer %d] RemoteApp created", local_explorer->get_explorer_id());

  // Base case is success
  *exploration_result = ExitStatus::SUCCESS;

  try {
    Explorer(*local_explorer);

  } catch (McWarning& error) {
    // An error occured while exploring
    // We need to tell the Tree Handler to stop immediately
    XBT_DEBUG("[tid:Explorer %d] An error has been found!", local_explorer->get_explorer_id());
    local_explorer->races_list->push(nullptr);

    *exploration_result = error.value;
  }

  // close the connection properly: remember that the exploration might not be over if another explorer found an error
  local_explorer->get_remote_app().finalize_app();
}

void Explorer(ThreadLocalExplorer& local_explorer)
{

  // Loading the local data regarding this thread and the exploration
  auto opened_heads_   = local_explorer.opened_heads;
  auto races_list_     = local_explorer.races_list;
  auto reduction_algo_ = local_explorer.reduction_algo;

  // While true -> we leave when receiving a poisoned value (nullptr)
  while (true) {

    XBT_DEBUG("[tid: Explorer %d] Let's grab something to work on shall we?", local_explorer.get_explorer_id());
    State* to_visit = opened_heads_->pop();

    if (to_visit == nullptr) {
      XBT_DEBUG("[tid:Explorer %d] Drinking the Kool-Aid sent by the TreeHandler! See ya",
                local_explorer.get_explorer_id());
      break;
    }

    if (to_visit->being_explored.test_and_set()) {
      // This state has already been or will be explored very soon by the TreeHandler, skip it
      races_list_->push(reduction_algo_->empty_race_update());
      continue;
    }

    XBT_DEBUG("[tid: Explorer %d] Found a next candidate to visit: state #%ld!", local_explorer.get_explorer_id(),
              to_visit->get_num());

    // Backtrack to to_visit, and restore stack/execution
    local_explorer.backtrack_to_state(to_visit);

    local_explorer.stack.clear();
    local_explorer.execution_seq = odpor::Execution();
    State* current_state         = to_visit;
    local_explorer.stack.emplace_front(current_state);
    // condition corresponds to reaching initial state
    while (current_state->get_parent_state() != nullptr) {
      current_state = current_state->get_parent_state();
      local_explorer.stack.emplace_front(current_state);
    }
    XBT_DEBUG("Replaced stack with %s", get_record_trace_from_stack(local_explorer.stack).to_string().c_str());
    for (auto iter = std::next(local_explorer.stack.begin()); iter != local_explorer.stack.end(); ++iter) {
      XBT_DEBUG("... taking transition <Actor %ld: %s> from state %ld to reconstitute the execution sequence",
                (*iter)->get_transition_in()->aid_, (*iter)->get_transition_in()->to_string().c_str(),
                (*iter)->get_num());
      local_explorer.execution_seq.push_transition((*iter)->get_transition_in());
    }

    // Explore until reaching a leaf
    bool do_explore = true;
    while (do_explore) {
      State* state = local_explorer.stack.back().get();

      const aid_t next = reduction_algo_->next_to_explore(local_explorer.execution_seq, &local_explorer.stack);

      if (next < 0) {
        // It can be: a leaf, a deadlock or a stop caused by reduction (eg. by sleep set)

        XBT_VERB("[tid: Explorer %d] %lu actors remain, but none of them need to be interleaved (depth %zu).",
                 local_explorer.get_explorer_id(), state->get_actor_count(), local_explorer.stack.size() + 1);
        local_explorer.check_deadlock(); // DEADLOCK CASE

        // Compute the race when reaching a leaf, and push it in the shared structure
        Reduction::RaceUpdate* todo_updates =
            reduction_algo_->races_computation(local_explorer.execution_seq, &local_explorer.stack);
        // pop/push on boost lockfree queue returns true iff the pop worked; a while loop is required by spec.

        races_list_->push(todo_updates);

        if (state->get_actor_count() == 0) {
          // LEAF CASE
          local_explorer.explored_traces++;

          local_explorer.get_remote_app().finalize_app();
          XBT_VERB("[tid: Explorer %d] Execution came to an end at %s", local_explorer.get_explorer_id(),
                   get_record_trace_from_stack(local_explorer.stack).to_string().c_str());
          XBT_VERB("[tid: Explorer %d] (state: %ld, depth: %zu, %lu explored traces)", local_explorer.get_explorer_id(),
                   state->get_num(), local_explorer.stack.size(), local_explorer.explored_traces);
        }

        do_explore = false;
        continue;
      }

      // If there is a next, explore it

      xbt_assert(state->is_actor_enabled(next));
      auto executed_transition = state->execute_next(next, local_explorer.get_remote_app());

      XBT_VERB("[tid: Explorer %d] Executed %ld: %.60s (stack depth: %zu, state: %ld)",
               local_explorer.get_explorer_id(), state->get_transition_out()->aid_,
               state->get_transition_out()->to_string().c_str(), local_explorer.stack.size(), state->get_num());

      local_explorer.visited_states_count++;

      auto next_state = reduction_algo_->state_create(local_explorer.get_remote_app(), state, executed_transition);

      reduction_algo_->on_backtrack(state);

      local_explorer.stack.emplace_back(std::move(next_state));
      local_explorer.execution_seq.push_transition(std::move(executed_transition));
    }

    XBT_DEBUG("[tid: Explorer %d] Ended exploration, going to wait for next state. (%lu explored traces so far)",
              local_explorer.get_explorer_id(), local_explorer.explored_traces);
  }

  // Do some logging that makes sens for now since there is only one thread

  XBT_INFO("[tid: Explorer %d] Parallel exploration ended. This explored visited %lu traces and %lu states "
           "visited overall",
           local_explorer.get_explorer_id(), local_explorer.explored_traces, local_explorer.visited_states_count);
  Exploration::get_instance()->log_state();
}

void ParallelizedExplorer::run()
{
  XBT_INFO("Start a Parallel exploration with %d threads. Reduction is: %s.", number_of_threads,
           to_c_str(reduction_mode_));

  one_way_disabled_ = true;

  auto initial_state = reduction_algo_->state_create(get_remote_app());
  initial_state->being_explored.clear();

  one_way_disabled_ = false;

  /* Get an enabled actor and insert it in the interleave set of the initial state */
  XBT_DEBUG("Initial state. %lu actors to consider", initial_state->get_actor_count());

  opened_heads_.push(initial_state.get());

  thread_results_.resize(number_of_threads);

  for (int i = 0; i < number_of_threads; i++) {
    local_explorers_.emplace_back(std::make_shared<ThreadLocalExplorer>(args_));
    local_explorers_[i]->opened_heads   = &opened_heads_;
    local_explorers_[i]->races_list     = &races_list_;
    local_explorers_[i]->reduction_algo = reduction_algo_.get();
  }

  // Create the explorers
  for (int i = 0; i < number_of_threads; i++) {
    thread_pool_.emplace_back(ExplorerHandler, local_explorers_[i].get(), &thread_results_[i]);
    // ExplorerHandler, std::ref(args_), reduction_algo_.get(), &opened_heads_,
    //	     &races_list_, &thread_results_[i]);
  }

  TreeHandler();

  for (int i = 0; i < number_of_threads; i++) {
    thread_pool_[i].join();
  }

  // Terminate the original remote_app from the main explorer
  // get_remote_app().terminate_one_way();
  get_remote_app().finalize_app();

  for (auto result : thread_results_)
    if (result != ExitStatus::SUCCESS)
      throw McWarning(result);

  long unsigned total_traces = 0;
  for (const auto& explo : local_explorers_)
    total_traces += explo->explored_traces;

  XBT_INFO("Parallel exploration ended. %lu explored traces overall", total_traces);
}

ParallelizedExplorer::ParallelizedExplorer(const std::vector<char*>& args, ReductionMode mode)
    : Exploration(), args_(args)
{

  Exploration::initialize_remote_app(args);

  reduction_mode_ = mode;
  if (reduction_mode_ == ReductionMode::dpor)
    reduction_algo_ = std::make_unique<DPOR>();
  else if (reduction_mode_ == ReductionMode::sdpor)
    reduction_algo_ = std::make_unique<SDPOR>();
  else if (reduction_mode_ == ReductionMode::odpor)
    reduction_algo_ = std::make_unique<ODPOR>();
  else {
    xbt_assert(reduction_mode_ == ReductionMode::none, "Reduction mode %s not supported yet by BeFS explorer",
               to_c_str(reduction_mode_));
    reduction_algo_ = std::make_unique<NoReduction>();
  }

  XBT_DEBUG("**************************************************");
}

Exploration* create_parallelized_exploration(const std::vector<char*>& args, ReductionMode mode)
{
  return new ParallelizedExplorer(args, mode);
}

} // namespace simgrid::mc
