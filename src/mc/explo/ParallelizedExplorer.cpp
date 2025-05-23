/* Copyright (c) 2016-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/ParallelizedExplorer.hpp"
#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/explo/reduction/ODPOR.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/mc/mc_forward.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/mc_record.hpp"
#include "src/mc/remote/mc_protocol.h"
#include "src/mc/transition/Transition.hpp"

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
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_parallel, mc, "Parallel exploration algorithm of the model-checker");

namespace simgrid::mc {

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

void TreeHandler(Reduction* reduction_algo_, parallel_channel<State*>* opened_heads_,
                 parallel_channel<Reduction::RaceUpdate*>* races_list_)
{

  // This local counter will repreent the number of things in opened_heads_ + the number of currently working Explorer
  int remaining_todo = 1; // The intial state

  // While there is something in opened_heads_ OR an Explorer is working
  while (remaining_todo > 0) {

    XBT_DEBUG("[tid:%d] New round of tree handling! There are currently %d remaining todo", gettid(), remaining_todo);

    Reduction::RaceUpdate* to_apply;
    // pop/push on boost lockfree queue returns true iff the pop worked; a while loop is required by spec.
    while (!races_list_->pop(to_apply)) {
    }

    // The race correspond to an explorer finishing, so we decrement the count
    remaining_todo--;
    XBT_DEBUG("[tid:%d] Received a race update! Going to apply it", gettid());

    std::vector<StatePtr> new_opened;
    remaining_todo +=
        reduction_algo_->apply_race_update(Exploration::get_instance()->get_remote_app(), to_apply, &new_opened);
    XBT_DEBUG("[tid:%d] The update contained %lu new states, so now there are %d remaining todo", gettid(),
              new_opened.size(), remaining_todo);

    // pop/push on boost lockfree queue returns true iff the pop worked; a while loop is required by spec.
    for (auto state : new_opened)
      while (!opened_heads_->push(state.get()))
        ;

    delete to_apply;
  }

  while (!opened_heads_->push(nullptr))
    ; // In the future, we should push as many nullptr as explorer
}

void ThreadLocalExplorer::backtrack_to_state(State* to_visit)
{
  Exploration::get_instance()->backtrack_remote_app_to_state(get_remote_app(), to_visit);
}

void ThreadLocalExplorer::check_deadlock()
{
  if (get_remote_app().check_deadlock()) {
    throw McError(ExitStatus::DEADLOCK);
  }
}

void Explorer(const std::vector<char*>& args, Reduction* reduction_algo_, parallel_channel<State*>* opened_heads_,
              parallel_channel<Reduction::RaceUpdate*>* races_list_)
{

  // Local structure containing helper functions and data regarding this thread and the exploration
  ThreadLocalExplorer local_explorer(args);

  XBT_DEBUG("Lauching thread %d", gettid());

  // While true -> we leave when receiving a poisoned value (nullptr)
  while (true) {

    State* to_visit;
    // pop/push on boost lockfree queue returns true iff the pop worked; a while loop is required by spec.
    while (!opened_heads_->pop(to_visit)) {
    }

    if (to_visit == nullptr) {
      XBT_DEBUG("[tid:%d] Drinking the Kool-Aid sent by the TreeHandler! See ya", gettid());
      break;
    }

    XBT_DEBUG("Found a next candidate to visit: state #%ld!", to_visit->get_num());

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

        XBT_VERB("%lu actors remain, but none of them need to be interleaved (depth %zu).", state->get_actor_count(),
                 local_explorer.stack.size() + 1);
        local_explorer.check_deadlock(); // DEADLOCK CASE

        if (state->get_actor_count() == 0) {
          // LEAF CASE
          // Compute the race when reaching a leaf, and push it in the shared structure
          Reduction::RaceUpdate* todo_updates =
              reduction_algo_->races_computation(local_explorer.execution_seq, &local_explorer.stack);
          // pop/push on boost lockfree queue returns true iff the pop worked; a while loop is required by spec.
          while (!races_list_->push(todo_updates))
            ;
          local_explorer.explored_traces++;

          local_explorer.get_remote_app().finalize_app();
          XBT_VERB("Execution came to an end at %s",
                   get_record_trace_from_stack(local_explorer.stack).to_string().c_str());
          XBT_VERB("(state: %ld, depth: %zu, %lu explored traces)", state->get_num(), local_explorer.stack.size(),
                   local_explorer.explored_traces);
        } else {
          // BLOCKED CASE
          // We still need to produce an empty RaceUpdate so the TreeHandler knows we returned from this exploration
          auto todo_updates = reduction_algo_->empty_race_update();
          // pop/push on boost lockfree queue returns true iff the pop worked; a while loop is required by spec.
          while (!races_list_->push(todo_updates))
            ;
        }

        do_explore = false;
        continue;
      }

      // If there is a next, explore it

      xbt_assert(state->is_actor_enabled(next));
      auto executed_transition = state->execute_next(next, local_explorer.get_remote_app());

      XBT_VERB("[tid:%d] Executed %ld: %.60s (stack depth: %zu, state: %ld)", gettid(),
               state->get_transition_out()->aid_, state->get_transition_out()->to_string().c_str(),
               local_explorer.stack.size(), state->get_num());

      auto next_state = reduction_algo_->state_create(local_explorer.get_remote_app(), state, executed_transition);

      reduction_algo_->on_backtrack(state);

      local_explorer.stack.emplace_back(std::move(next_state));
      local_explorer.execution_seq.push_transition(std::move(executed_transition));
    }

    XBT_DEBUG("[tid:%d] Ended exploration, going to wait for next state. (%lu explored traces so far)", gettid(),
              local_explorer.explored_traces);
  }

  // Do some logging that makes sens for now since there is only one thread

  XBT_INFO(
      "Parallel exploration ended. %ld unique states visited; %lu explored traces (%lu transition replays, %lu states "
      "visited overall)",
      State::get_expanded_states(), local_explorer.explored_traces, Transition::get_replayed_transitions(),
      local_explorer.visited_states_count);
  Exploration::get_instance()->log_state();
}

void ParallelizedExplorer::run()
{
  XBT_INFO("Start a Parallel exploration with one thread. Reduction is: %s.", to_c_str(reduction_mode_));

  auto initial_state = reduction_algo_->state_create(get_remote_app());

  /* Get an enabled actor and insert it in the interleave set of the initial state */
  XBT_DEBUG("Initial state. %lu actors to consider", initial_state->get_actor_count());

  // pop/push on boost lockfree queue returns true iff the pop worked; a while loop is required by spec.
  while (!opened_heads_.push(initial_state.get()))
    ;

  // Create an explorer
  std::thread* texplorer =
      new std::thread(Explorer, std::ref(args_), reduction_algo_.get(), &opened_heads_, &races_list_);

  thread_pool_.push_back(texplorer);

  TreeHandler(reduction_algo_.get(), &opened_heads_, &races_list_);

  for (auto t : thread_pool_) {
    t->join();
    delete t;
  }

  // Terminate the original remote_app from the main explorer
  get_remote_app().terminate_one_way();
  get_remote_app().finalize_app();
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

  XBT_INFO("Start a parallelized exploration. Reduction is: %s.", to_c_str(reduction_mode_));

  XBT_DEBUG("**************************************************");
}

Exploration* create_parallelized_exploration(const std::vector<char*>& args, ReductionMode mode)
{
  return new ParallelizedExplorer(args, mode);
}

} // namespace simgrid::mc
