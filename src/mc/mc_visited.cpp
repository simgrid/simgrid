/* Copyright (c) 2011-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <unistd.h>
#include <sys/wait.h>

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

xbt_dynar_t visited_pairs;
xbt_dynar_t visited_states;

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

/**
 *  \brief Find a suitable subrange of candidate duplicates for a given state
 *  \param list dynamic array of states/pairs with candidate duplicates of the current state;
 *  \param ref current state/pair;
 *  \param min (output) index of the beginning of the the subrange
 *  \param max (output) index of the enf of the subrange
 *
 *  Given a suitably ordered array of states/pairs, this function extracts a subrange
 *  (with index *min <= i <= *max) with candidate duplicates of the given state/pair.
 *  This function uses only fast discriminating criterions and does not use the
 *  full state/pair comparison algorithms.
 *
 *  The states/pairs in list MUST be ordered using a (given) weak order
 *  (based on nb_processes and heap_bytes_used).
 *  The subrange is the subrange of "equivalence" of the given state/pair.
 */
int get_search_interval(xbt_dynar_t list, void *ref, int *min, int *max)
{
  int cursor = 0, previous_cursor;
  int nb_processes, heap_bytes_used, nb_processes_test, heap_bytes_used_test;
  void *ref_test;

  if (_sg_mc_liveness) {
    nb_processes = ((simgrid::mc::VisitedPair*) ref)->nb_processes;
    heap_bytes_used = ((simgrid::mc::VisitedPair*) ref)->heap_bytes_used;
  } else {
    nb_processes = ((simgrid::mc::VisitedState*) ref)->nb_processes;
    heap_bytes_used = ((simgrid::mc::VisitedState*) ref)->heap_bytes_used;
  }

  int start = 0;
  int end = xbt_dynar_length(list) - 1;

  while (start <= end) {
    cursor = (start + end) / 2;
    if (_sg_mc_liveness) {
      ref_test = (simgrid::mc::VisitedPair*) xbt_dynar_get_as(list, cursor, simgrid::mc::VisitedPair*);
      nb_processes_test = ((simgrid::mc::VisitedPair*) ref_test)->nb_processes;
      heap_bytes_used_test = ((simgrid::mc::VisitedPair*) ref_test)->heap_bytes_used;
    } else {
      ref_test = (simgrid::mc::VisitedState*) xbt_dynar_get_as(list, cursor, simgrid::mc::VisitedState*);
      nb_processes_test = ((simgrid::mc::VisitedState*) ref_test)->nb_processes;
      heap_bytes_used_test = ((simgrid::mc::VisitedState*) ref_test)->heap_bytes_used;
    }
    if (nb_processes_test < nb_processes)
      start = cursor + 1;
    else if (nb_processes_test > nb_processes)
      end = cursor - 1;
    else if (heap_bytes_used_test < heap_bytes_used)
      start = cursor + 1;
    else if (heap_bytes_used_test > heap_bytes_used)
      end = cursor - 1;
    else {
        *min = *max = cursor;
        previous_cursor = cursor - 1;
        while (previous_cursor >= 0) {
          if (_sg_mc_liveness) {
            ref_test = (simgrid::mc::VisitedPair*) xbt_dynar_get_as(list, previous_cursor, simgrid::mc::VisitedPair*);
            nb_processes_test = ((simgrid::mc::VisitedPair*) ref_test)->nb_processes;
            heap_bytes_used_test = ((simgrid::mc::VisitedPair*) ref_test)->heap_bytes_used;
          } else {
            ref_test = (simgrid::mc::VisitedState*) xbt_dynar_get_as(list, previous_cursor, simgrid::mc::VisitedState*);
            nb_processes_test = ((simgrid::mc::VisitedState*) ref_test)->nb_processes;
            heap_bytes_used_test = ((simgrid::mc::VisitedState*) ref_test)->heap_bytes_used;
          }
          if (nb_processes_test != nb_processes || heap_bytes_used_test != heap_bytes_used)
            break;
          *min = previous_cursor;
          previous_cursor--;
        }
        size_t next_cursor = cursor + 1;
        while (next_cursor < xbt_dynar_length(list)) {
          if (_sg_mc_liveness) {
            ref_test = (simgrid::mc::VisitedPair*) xbt_dynar_get_as(list, next_cursor, simgrid::mc::VisitedPair*);
            nb_processes_test = ((simgrid::mc::VisitedPair*) ref_test)->nb_processes;
            heap_bytes_used_test = ((simgrid::mc::VisitedPair*) ref_test)->heap_bytes_used;
          } else {
            ref_test = (simgrid::mc::VisitedState*) xbt_dynar_get_as(list, next_cursor, simgrid::mc::VisitedState*);
            nb_processes_test = ((simgrid::mc::VisitedState*) ref_test)->nb_processes;
            heap_bytes_used_test = ((simgrid::mc::VisitedState*) ref_test)->heap_bytes_used;
          }
          if (nb_processes_test != nb_processes || heap_bytes_used_test != heap_bytes_used)
            break;
          *max = next_cursor;
          next_cursor++;
        }
        return -1;
    }
  }
  return cursor;
}

static
void replace_state(
  simgrid::mc::VisitedState* state_test, simgrid::mc::VisitedState* new_state, int cursor)
{
  if (state_test->other_num == -1)
    new_state->other_num = state_test->num;
  else
    new_state->other_num = state_test->other_num;

  if (dot_output == nullptr)
    XBT_DEBUG("State %d already visited ! (equal to state %d)",
      new_state->num, state_test->num);
  else
    XBT_DEBUG(
      "State %d already visited ! (equal to state %d (state %d in dot_output))",
      new_state->num, state_test->num, new_state->other_num);

  /* Replace the old state with the new one (with a bigger num)
     (when the max number of visited states is reached,  the oldest
     one is removed according to its number (= with the min number) */
  xbt_dynar_remove_at(simgrid::mc::visited_states, cursor, nullptr);
  xbt_dynar_insert_at(simgrid::mc::visited_states, cursor, &new_state);
  XBT_DEBUG("Replace visited state %d with the new visited state %d",
    state_test->num, new_state->num);
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
simgrid::mc::VisitedState* is_visited_state(mc_state_t graph_state)
{
  if (_sg_mc_visited == 0)
    return nullptr;

  /* If comm determinism verification, we cannot stop the exploration if some 
     communications are not finished (at least, data are transfered). These communications 
     are incomplete and they cannot be analyzed and compared with the initial pattern. */
  int partial_comm = (_sg_mc_comms_determinism || _sg_mc_send_determinism) &&
    some_communications_are_not_finished();

  simgrid::mc::VisitedState* new_state = new VisitedState();
  graph_state->system_state = new_state->system_state;
  graph_state->in_visited_states = 1;
  XBT_DEBUG("Snapshot %p of visited state %d (exploration stack state %d)", new_state->system_state, new_state->num, graph_state->num);

  if (xbt_dynar_is_empty(visited_states)) {
    xbt_dynar_push(visited_states, &new_state);
    return nullptr;
  }

    int min = -1, max = -1, index;

    index = get_search_interval(visited_states, new_state, &min, &max);

    if (min != -1 && max != -1) {

      if (_sg_mc_safety || (!partial_comm
        && initial_global_state->initial_communications_pattern_done)) {

        int cursor = min;
        while (cursor <= max) {
          simgrid::mc::VisitedState* state_test = (simgrid::mc::VisitedState*) xbt_dynar_get_as(visited_states, cursor, simgrid::mc::VisitedState*);
          if (snapshot_compare(state_test, new_state) == 0) {
            // The state has been visited:

            replace_state(state_test, new_state, cursor);
            return state_test;
          }
          cursor++;
        }
      }
      
      xbt_dynar_insert_at(visited_states, min, &new_state);
      XBT_DEBUG("Insert new visited state %d (total : %lu)", new_state->num, xbt_dynar_length(visited_states));
      
    } else {

      // The state has not been visited: insert the state in the dynamic array.
      simgrid::mc::VisitedState* state_test = (simgrid::mc::VisitedState*) xbt_dynar_get_as(visited_states, index, simgrid::mc::VisitedState*);
      if (state_test->nb_processes < new_state->nb_processes)
        xbt_dynar_insert_at(visited_states, index + 1, &new_state);
      else if (state_test->heap_bytes_used < new_state->heap_bytes_used)
        xbt_dynar_insert_at(visited_states, index + 1, &new_state);
      else
        xbt_dynar_insert_at(visited_states, index, &new_state);

       XBT_DEBUG("Insert new visited state %d (total : %lu)", new_state->num, xbt_dynar_length(visited_states));

    }

  if ((ssize_t) xbt_dynar_length(visited_states) <= _sg_mc_visited)
    return nullptr;

  // We have reached the maximum number of stored states;

      XBT_DEBUG("Try to remove visited state (maximum number of stored states reached)");

      // Find the (index of the) older state (with the smallest num):
      int min2 = mc_stats->expanded_states;
      unsigned int cursor2 = 0;
      unsigned int index2 = 0;

      simgrid::mc::VisitedState* state_test;
      xbt_dynar_foreach(visited_states, cursor2, state_test)
        if (!mc_model_checker->is_important_snapshot(*state_test->system_state)
            && state_test->num < min2) {
          index2 = cursor2;
          min2 = state_test->num;
        }

      // and drop it:
      xbt_dynar_remove_at(visited_states, index2, nullptr);
      XBT_DEBUG("Remove visited state (maximum number of stored states reached)");

    return nullptr;
}

/**
 * \brief Checks whether a given pair has already been visited by the algorithm.
 */
int is_visited_pair(simgrid::mc::VisitedPair* visited_pair, simgrid::mc::Pair* pair) {

  if (_sg_mc_visited == 0)
    return -1;

  simgrid::mc::VisitedPair* new_visited_pair = nullptr;
  if (visited_pair == nullptr)
    new_visited_pair = new simgrid::mc::VisitedPair(
      pair->num, pair->automaton_state, pair->atomic_propositions.get(),
      pair->graph_state);
  else
    new_visited_pair = visited_pair;

  if (xbt_dynar_is_empty(visited_pairs)) {
    xbt_dynar_push(visited_pairs, &new_visited_pair);
    return -1;
  }

    int min = -1, max = -1, index;
    //int res;
    simgrid::mc::VisitedPair* pair_test;
    int cursor;

    index = get_search_interval(visited_pairs, new_visited_pair, &min, &max);

    if (min != -1 && max != -1) {       // Visited pair with same number of processes and same heap bytes used exists
      cursor = min;
      while (cursor <= max) {
        pair_test = (simgrid::mc::VisitedPair*) xbt_dynar_get_as(visited_pairs, cursor, simgrid::mc::VisitedPair*);
        if (xbt_automaton_state_compare(pair_test->automaton_state, new_visited_pair->automaton_state) == 0) {
          if (xbt_automaton_propositional_symbols_compare_value(
              pair_test->atomic_propositions.get(),
              new_visited_pair->atomic_propositions.get()) == 0) {
            if (snapshot_compare(pair_test, new_visited_pair) == 0) {
              if (pair_test->other_num == -1)
                new_visited_pair->other_num = pair_test->num;
              else
                new_visited_pair->other_num = pair_test->other_num;
              if (dot_output == nullptr)
                XBT_DEBUG("Pair %d already visited ! (equal to pair %d)", new_visited_pair->num, pair_test->num);
              else
                XBT_DEBUG("Pair %d already visited ! (equal to pair %d (pair %d in dot_output))", new_visited_pair->num, pair_test->num, new_visited_pair->other_num);
              xbt_dynar_remove_at(visited_pairs, cursor, &pair_test);
              xbt_dynar_insert_at(visited_pairs, cursor, &new_visited_pair);
              pair_test->visited_removed = 1;
              if (!pair_test->acceptance_pair
                  || pair_test->acceptance_removed == 1)
                delete pair_test;
              return new_visited_pair->other_num;
            }
          }
        }
        cursor++;
      }
      xbt_dynar_insert_at(visited_pairs, min, &new_visited_pair);
    } else {
      pair_test = (simgrid::mc::VisitedPair*) xbt_dynar_get_as(visited_pairs, index, simgrid::mc::VisitedPair*);
      if (pair_test->nb_processes < new_visited_pair->nb_processes)
        xbt_dynar_insert_at(visited_pairs, index + 1, &new_visited_pair);
      else if (pair_test->heap_bytes_used < new_visited_pair->heap_bytes_used)
        xbt_dynar_insert_at(visited_pairs, index + 1, &new_visited_pair);
      else
        xbt_dynar_insert_at(visited_pairs, index, &new_visited_pair);
    }

    if ((ssize_t) xbt_dynar_length(visited_pairs) > _sg_mc_visited) {
      int min2 = mc_stats->expanded_pairs;
      unsigned int cursor2 = 0;
      unsigned int index2 = 0;
      xbt_dynar_foreach(visited_pairs, cursor2, pair_test) {
        if (!mc_model_checker->is_important_snapshot(*pair_test->graph_state->system_state)
            && pair_test->num < min2) {
          index2 = cursor2;
          min2 = pair_test->num;
        }
      }
      xbt_dynar_remove_at(visited_pairs, index2, &pair_test);
      pair_test->visited_removed = 1;
      if (!pair_test->acceptance_pair || pair_test->acceptance_removed)
        delete pair_test;
    }
  return -1;
}

}
}