/* Copyright (c) 2011-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <unistd.h>
#include <sys/wait.h>

#include <memory>
#include <algorithm>

#include <xbt/automaton.h>
#include <xbt/log.h>
#include <xbt/sysdep.h>
#include <xbt/dynar.h>
#include <xbt/fifo.h>

#include "src/mc/mc_comm_pattern.h"
#include "src/mc/mc_safety.h"
#include "src/mc/mc_liveness.h"
#include "src/mc/mc_private.h"
#include "src/mc/Process.hpp"
#include "src/mc/mc_smx.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_visited, mc,
                                "Logging specific to state equaity detection mechanisms");

namespace simgrid {
namespace mc {

std::vector<std::unique_ptr<simgrid::mc::VisitedState>> visited_states;

static int is_exploration_stack_state(simgrid::mc::VisitedState* state){
  xbt_fifo_item_t item = xbt_fifo_get_first_item(mc_stack);
  while (item) {
    if (((mc_state_t)xbt_fifo_get_item_content(item))->num == state->num){
      ((mc_state_t)xbt_fifo_get_item_content(item))->in_visited_states = 0;
      return 1;
    }
    item = xbt_fifo_get_next_item(item);
  }
  return 0;
}

/**
 * \brief Save the current state
 * \return Snapshot of the current state.
 */
VisitedState::VisitedState()
{
  simgrid::mc::Process* process = &(mc_model_checker->process());
  this->heap_bytes_used = mmalloc_get_bytes_used_remote(
    process->get_heap()->heaplimit,
    process->get_malloc_info());

  this->nb_processes =
    mc_model_checker->process().simix_processes().size();

  this->system_state = simgrid::mc::take_snapshot(mc_stats->expanded_states);
  this->num = mc_stats->expanded_states;
  this->other_num = -1;
}

VisitedState::~VisitedState()
{
  if(!is_exploration_stack_state(this))
    delete this->system_state;
}

VisitedPair::VisitedPair(int pair_num, xbt_automaton_state_t automaton_state, xbt_dynar_t atomic_propositions, mc_state_t graph_state)
{
  simgrid::mc::Process* process = &(mc_model_checker->process());

  this->graph_state = graph_state;
  if(this->graph_state->system_state == nullptr)
    this->graph_state->system_state = simgrid::mc::take_snapshot(pair_num);
  this->heap_bytes_used = mmalloc_get_bytes_used_remote(
    process->get_heap()->heaplimit,
    process->get_malloc_info());

  this->nb_processes =
    mc_model_checker->process().simix_processes().size();

  this->automaton_state = automaton_state;
  this->num = pair_num;
  this->other_num = -1;
  this->acceptance_removed = 0;
  this->visited_removed = 0;
  this->acceptance_pair = 0;
  this->atomic_propositions = simgrid::xbt::unique_ptr<s_xbt_dynar_t>(
    xbt_dynar_new(sizeof(int), nullptr));

  unsigned int cursor = 0;
  int value;
  xbt_dynar_foreach(atomic_propositions, cursor, value)
      xbt_dynar_push_as(this->atomic_propositions.get(), int, value);
}

static int is_exploration_stack_pair(simgrid::mc::VisitedPair* pair){
  xbt_fifo_item_t item = xbt_fifo_get_first_item(mc_stack);
  while (item) {
    if (((simgrid::mc::Pair*)xbt_fifo_get_item_content(item))->num == pair->num){
      ((simgrid::mc::Pair*)xbt_fifo_get_item_content(item))->visited_pair_removed = 1;
      return 1;
    }
    item = xbt_fifo_get_next_item(item);
  }
  return 0;
}

VisitedPair::~VisitedPair()
{
  if( !is_exploration_stack_pair(this))
    MC_state_delete(this->graph_state, 1);
}

}
}

static
bool some_communications_are_not_finished()
{
  for (size_t current_process = 1; current_process < MC_smx_get_maxpid(); current_process++) {
    xbt_dynar_t pattern = xbt_dynar_get_as(
      incomplete_communications_pattern, current_process, xbt_dynar_t);
    if (!xbt_dynar_is_empty(pattern)) {
      XBT_DEBUG("Some communications are not finished, cannot stop the exploration ! State not visited.");
      return true;
    }
  }
  return false;
}

namespace simgrid {
namespace mc {

/**
 * \brief Checks whether a given state has already been visited by the algorithm.
 */
std::unique_ptr<simgrid::mc::VisitedState> is_visited_state(mc_state_t graph_state)
{
  if (_sg_mc_visited == 0)
    return nullptr;

  /* If comm determinism verification, we cannot stop the exploration if some 
     communications are not finished (at least, data are transfered). These communications 
     are incomplete and they cannot be analyzed and compared with the initial pattern. */
  int partial_comm = (_sg_mc_comms_determinism || _sg_mc_send_determinism) &&
    some_communications_are_not_finished();

  std::unique_ptr<simgrid::mc::VisitedState> new_state =
    std::unique_ptr<simgrid::mc::VisitedState>(new VisitedState());
  graph_state->system_state = new_state->system_state;
  graph_state->in_visited_states = 1;
  XBT_DEBUG("Snapshot %p of visited state %d (exploration stack state %d)", new_state->system_state, new_state->num, graph_state->num);

  auto range = std::equal_range(visited_states.begin(), visited_states.end(),
    new_state.get(), simgrid::mc::DerefAndCompareByNbProcessesAndUsedHeap());

      if (_sg_mc_safety || (!partial_comm
        && initial_global_state->initial_communications_pattern_done)) {

        for (auto i = range.first; i != range.second; ++i) {
          auto& visited_state = *i;
          if (snapshot_compare(visited_state.get(), new_state.get()) == 0) {
            // The state has been visited:

            std::unique_ptr<simgrid::mc::VisitedState> old_state =
              std::move(visited_state);

            if (old_state->other_num == -1)
              new_state->other_num = old_state->num;
            else
              new_state->other_num = old_state->other_num;

            if (dot_output == nullptr)
              XBT_DEBUG("State %d already visited ! (equal to state %d)",
                new_state->num, old_state->num);
            else
              XBT_DEBUG(
                "State %d already visited ! (equal to state %d (state %d in dot_output))",
                new_state->num, old_state->num, new_state->other_num);

            /* Replace the old state with the new one (with a bigger num)
               (when the max number of visited states is reached,  the oldest
               one is removed according to its number (= with the min number) */
            XBT_DEBUG("Replace visited state %d with the new visited state %d",
              old_state->num, new_state->num);

            visited_state = std::move(new_state);
            return std::move(old_state);
          }
        }
      }

  XBT_DEBUG("Insert new visited state %d (total : %lu)", new_state->num, (unsigned long) visited_states.size());
  visited_states.insert(range.first, std::move(new_state));

  if (visited_states.size() <= (std::size_t) _sg_mc_visited)
    return nullptr;

  // We have reached the maximum number of stored states;

      XBT_DEBUG("Try to remove visited state (maximum number of stored states reached)");

      // Find the (index of the) older state (with the smallest num):
      int min2 = mc_stats->expanded_states;
      unsigned int index2 = 0;

      for (std::size_t cursor2 = 0; cursor2 != visited_states.size(); ++cursor2)
        if (!mc_model_checker->is_important_snapshot(
              *visited_states[cursor2]->system_state)
            && visited_states[cursor2]->num < min2) {
          index2 = cursor2;
          min2 = visited_states[cursor2]->num;
        }

      // and drop it:
      visited_states.erase(visited_states.begin() + index2);
      XBT_DEBUG("Remove visited state (maximum number of stored states reached)");

    return nullptr;
}

}
}