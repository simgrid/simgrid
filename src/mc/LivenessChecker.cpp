/* Copyright (c) 2011-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstring>

#include <algorithm>
#include <memory>

#include <unistd.h>
#include <sys/wait.h>

#include <xbt/automaton.h>
#include <xbt/dynar.h>
#include <xbt/dynar.hpp>
#include <xbt/fifo.h>
#include <xbt/log.h>
#include <xbt/sysdep.h>

#include "src/mc/mc_request.h"
#include "src/mc/LivenessChecker.hpp"
#include "src/mc/mc_private.h"
#include "src/mc/mc_record.h"
#include "src/mc/mc_smx.h"
#include "src/mc/Client.hpp"
#include "src/mc/mc_private.h"
#include "src/mc/mc_replay.h"
#include "src/mc/mc_safety.h"
#include "src/mc/mc_exit.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_liveness, mc,
                                "Logging specific to algorithms for liveness properties verification");

/********* Global variables *********/

xbt_dynar_t acceptance_pairs;
static xbt_fifo_t liveness_stack;

/********* Static functions *********/

namespace simgrid {
namespace mc {

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
  xbt_fifo_item_t item = xbt_fifo_get_first_item(liveness_stack);
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

static int MC_automaton_evaluate_label(xbt_automaton_exp_label_t l,
                                       xbt_dynar_t atomic_propositions_values)
{

  switch (l->type) {
  case 0:{
      int left_res = MC_automaton_evaluate_label(l->u.or_and.left_exp, atomic_propositions_values);
      int right_res = MC_automaton_evaluate_label(l->u.or_and.right_exp, atomic_propositions_values);
      return (left_res || right_res);
    }
  case 1:{
      int left_res = MC_automaton_evaluate_label(l->u.or_and.left_exp, atomic_propositions_values);
      int right_res = MC_automaton_evaluate_label(l->u.or_and.right_exp, atomic_propositions_values);
      return (left_res && right_res);
    }
  case 2:{
      int res = MC_automaton_evaluate_label(l->u.exp_not, atomic_propositions_values);
      return (!res);
    }
  case 3:{
      unsigned int cursor = 0;
      xbt_automaton_propositional_symbol_t p = nullptr;
      xbt_dynar_foreach(simgrid::mc::property_automaton->propositional_symbols, cursor, p) {
        if (std::strcmp(xbt_automaton_propositional_symbol_get_name(p), l->u.predicat) == 0)
          return (int) xbt_dynar_get_as(atomic_propositions_values, cursor, int);
      }
      return -1;
    }
  case 4:
    return 2;
  default:
    return -1;
  }
}

static xbt_dynar_t visited_pairs;

Pair::Pair() : num(++mc_stats->expanded_pairs),
  visited_pair_removed(_sg_mc_visited > 0 ? 0 : 1)
{}

Pair::~Pair() {
  if (this->visited_pair_removed)
    MC_state_delete(this->graph_state, 1);
}

simgrid::xbt::unique_ptr<s_xbt_dynar_t> LivenessChecker::getPropositionValues()
{
  unsigned int cursor = 0;
  xbt_automaton_propositional_symbol_t ps = nullptr;
  simgrid::xbt::unique_ptr<s_xbt_dynar_t> values = simgrid::xbt::unique_ptr<s_xbt_dynar_t>(xbt_dynar_new(sizeof(int), nullptr));
  xbt_dynar_foreach(simgrid::mc::property_automaton->propositional_symbols, cursor, ps) {
    int res = xbt_automaton_propositional_symbol_evaluate(ps);
    xbt_dynar_push_as(values.get(), int, res);
  }
  return std::move(values);
}

int LivenessChecker::compare(simgrid::mc::VisitedPair* state1, simgrid::mc::VisitedPair* state2)
{
  simgrid::mc::Snapshot* s1 = state1->graph_state->system_state;
  simgrid::mc::Snapshot* s2 = state2->graph_state->system_state;
  int num1 = state1->num;
  int num2 = state2->num;
  return simgrid::mc::snapshot_compare(num1, s1, num2, s2);
}

simgrid::mc::VisitedPair* LivenessChecker::insertAcceptancePair(simgrid::mc::Pair* pair)
{
  auto acceptance_pairs = simgrid::xbt::range<simgrid::mc::VisitedPair*>(::acceptance_pairs);
  auto new_pair =
    std::unique_ptr<simgrid::mc::VisitedPair>(new VisitedPair(
      pair->num, pair->automaton_state, pair->atomic_propositions.get(),
      pair->graph_state));
  new_pair->acceptance_pair = 1;

  auto res = std::equal_range(acceptance_pairs.begin(), acceptance_pairs.end(),
    new_pair, simgrid::mc::DerefAndCompareByNbProcessesAndUsedHeap());

  if (pair->search_cycle == 1)
    for (auto i = res.first; i != res.second; ++i) {
      simgrid::mc::VisitedPair* pair_test = *i;
      if (xbt_automaton_state_compare(pair_test->automaton_state, new_pair->automaton_state) == 0) {
        if (xbt_automaton_propositional_symbols_compare_value(
            pair_test->atomic_propositions.get(),
            new_pair->atomic_propositions.get()) == 0) {
          if (this->compare(pair_test, new_pair.get()) == 0) {
            XBT_INFO("Pair %d already reached (equal to pair %d) !", new_pair->num, pair_test->num);
            xbt_fifo_shift(liveness_stack);
            if (dot_output != nullptr)
              fprintf(dot_output, "\"%d\" -> \"%d\" [%s];\n", initial_global_state->prev_pair, pair_test->num, initial_global_state->prev_req);
            return nullptr;
          }
        }
      }
    }

  auto new_raw_pair = new_pair.release();
  xbt_dynar_insert_at(
    ::acceptance_pairs, res.first - acceptance_pairs.begin(), &new_raw_pair);
  return new_raw_pair;
}

void LivenessChecker::removeAcceptancePair(int pair_num)
{
  unsigned int cursor = 0;
  simgrid::mc::VisitedPair* pair_test = nullptr;
  int pair_found = 0;

  xbt_dynar_foreach(acceptance_pairs, cursor, pair_test)
    if (pair_test->num == pair_num) {
       pair_found = 1;
      break;
    }

  if(pair_found == 1) {
    xbt_dynar_remove_at(acceptance_pairs, cursor, &pair_test);

    pair_test->acceptance_removed = 1;

    if (_sg_mc_visited == 0 || pair_test->visited_removed == 1) 
      delete pair_test;

  }
}

void LivenessChecker::prepare(void)
{
  simgrid::mc::Pair* initial_pair = nullptr;
  mc_model_checker->wait_for_requests();
  acceptance_pairs = xbt_dynar_new(sizeof(simgrid::mc::VisitedPair*), nullptr);
  if(_sg_mc_visited > 0)
    simgrid::mc::visited_pairs = xbt_dynar_new(sizeof(simgrid::mc::VisitedPair*), nullptr);

  initial_global_state->snapshot = simgrid::mc::take_snapshot(0);
  initial_global_state->prev_pair = 0;

  unsigned int cursor = 0;
  xbt_automaton_state_t automaton_state;

  xbt_dynar_foreach(simgrid::mc::property_automaton->states, cursor, automaton_state) {
    if (automaton_state->type == -1) {  /* Initial automaton state */

      initial_pair = new Pair();
      initial_pair->automaton_state = automaton_state;
      initial_pair->graph_state = MC_state_new();
      initial_pair->atomic_propositions = this->getPropositionValues();
      initial_pair->depth = 1;

      /* Get enabled processes and insert them in the interleave set of the graph_state */
      for (auto& p : mc_model_checker->process().simix_processes())
        if (simgrid::mc::process_is_enabled(&p.copy))
          MC_state_interleave_process(initial_pair->graph_state, &p.copy);

      initial_pair->requests = MC_state_interleave_size(initial_pair->graph_state);
      initial_pair->search_cycle = 0;

      xbt_fifo_unshift(liveness_stack, initial_pair);
    }
  }
}


void LivenessChecker::replay(xbt_fifo_t stack)
{
  xbt_fifo_item_t item;
  simgrid::mc::Pair* pair = nullptr;
  mc_state_t state = nullptr;
  smx_simcall_t req = nullptr, saved_req = NULL;
  int value, depth = 1;
  char *req_str;

  XBT_DEBUG("**** Begin Replay ****");

  /* Intermediate backtracking */
  if(_sg_mc_checkpoint > 0) {
    item = xbt_fifo_get_first_item(stack);
    pair = (simgrid::mc::Pair*) xbt_fifo_get_item_content(item);
    if(pair->graph_state->system_state){
      simgrid::mc::restore_snapshot(pair->graph_state->system_state);
      return;
    }
  }

  /* Restore the initial state */
  simgrid::mc::restore_snapshot(initial_global_state->snapshot);

    /* Traverse the stack from the initial state and re-execute the transitions */
    for (item = xbt_fifo_get_last_item(stack);
         item != xbt_fifo_get_first_item(stack);
         item = xbt_fifo_get_prev_item(item)) {

      pair = (simgrid::mc::Pair*) xbt_fifo_get_item_content(item);

      state = (mc_state_t) pair->graph_state;

      if (pair->exploration_started) {

        saved_req = MC_state_get_executed_request(state, &value);

        if (saved_req != nullptr) {
          /* because we got a copy of the executed request, we have to fetch the
             real one, pointed by the request field of the issuer process */
          const smx_process_t issuer = MC_smx_simcall_get_issuer(saved_req);
          req = &issuer->simcall;

          /* Debug information */
          if (XBT_LOG_ISENABLED(mc_liveness, xbt_log_priority_debug)) {
            req_str = simgrid::mc::request_to_string(req, value, simgrid::mc::RequestType::simix);
            XBT_DEBUG("Replay (depth = %d) : %s (%p)", depth, req_str, state);
            xbt_free(req_str);
          }

        }

        simgrid::mc::handle_simcall(req, value);
        mc_model_checker->wait_for_requests();
      }

      /* Update statistics */
      mc_stats->visited_pairs++;
      mc_stats->executed_transitions++;

      depth++;

    }

  XBT_DEBUG("**** End Replay ****");
}

/**
 * \brief Checks whether a given pair has already been visited by the algorithm.
 */
int LivenessChecker::insertVisitedPair(simgrid::mc::VisitedPair* visited_pair, simgrid::mc::Pair* pair)
{
  if (_sg_mc_visited == 0)
    return -1;

  simgrid::mc::VisitedPair* new_visited_pair = nullptr;
  if (visited_pair == nullptr)
    new_visited_pair = new simgrid::mc::VisitedPair(
      pair->num, pair->automaton_state, pair->atomic_propositions.get(),
      pair->graph_state);
  else
    new_visited_pair = visited_pair;

  auto visited_pairs = simgrid::xbt::range<simgrid::mc::VisitedPair*>(simgrid::mc::visited_pairs);

  auto range = std::equal_range(visited_pairs.begin(), visited_pairs.end(),
    new_visited_pair, simgrid::mc::DerefAndCompareByNbProcessesAndUsedHeap());

  for (auto i = range.first; i != range.second; ++i) {
    simgrid::mc::VisitedPair* pair_test = *i;
    std::size_t cursor = i - visited_pairs.begin();
    if (xbt_automaton_state_compare(pair_test->automaton_state, new_visited_pair->automaton_state) == 0) {
      if (xbt_automaton_propositional_symbols_compare_value(
          pair_test->atomic_propositions.get(),
          new_visited_pair->atomic_propositions.get()) == 0) {
        if (this->compare(pair_test, new_visited_pair) == 0) {
          if (pair_test->other_num == -1)
            new_visited_pair->other_num = pair_test->num;
          else
            new_visited_pair->other_num = pair_test->other_num;
          if (dot_output == nullptr)
            XBT_DEBUG("Pair %d already visited ! (equal to pair %d)", new_visited_pair->num, pair_test->num);
          else
            XBT_DEBUG("Pair %d already visited ! (equal to pair %d (pair %d in dot_output))", new_visited_pair->num, pair_test->num, new_visited_pair->other_num);
          xbt_dynar_remove_at(simgrid::mc::visited_pairs, cursor, &pair_test);
          xbt_dynar_insert_at(simgrid::mc::visited_pairs, cursor, &new_visited_pair);
          pair_test->visited_removed = 1;
          if (!pair_test->acceptance_pair
              || pair_test->acceptance_removed == 1)
            delete pair_test;
          return new_visited_pair->other_num;
        }
      }
    }
  }

  xbt_dynar_insert_at(simgrid::mc::visited_pairs, range.first - visited_pairs.begin(), &new_visited_pair);

  if ((ssize_t) visited_pairs.size() > _sg_mc_visited) {
    int min2 = mc_stats->expanded_pairs;
    unsigned int index2 = 0;
    for (std::size_t i = 0; i != (std::size_t) visited_pairs.size(); ++i) {
      simgrid::mc::VisitedPair* pair_test = visited_pairs[i];
      if (!mc_model_checker->is_important_snapshot(*pair_test->graph_state->system_state)
          && pair_test->num < min2) {
        index2 = i;
        min2 = pair_test->num;
      }
    }
    simgrid::mc::VisitedPair* pair_test = nullptr;
    xbt_dynar_remove_at(simgrid::mc::visited_pairs, index2, &pair_test);
    pair_test->visited_removed = 1;
    if (!pair_test->acceptance_pair || pair_test->acceptance_removed)
      delete pair_test;
  }

  return -1;
}

LivenessChecker::LivenessChecker(Session& session) : Checker(session)
{
}

LivenessChecker::~LivenessChecker()
{
}

RecordTrace LivenessChecker::getRecordTrace() // override
{
  RecordTrace res;

  xbt_fifo_item_t start = xbt_fifo_get_last_item(liveness_stack);
  for (xbt_fifo_item_t item = start; item; item = xbt_fifo_get_prev_item(item)) {
    simgrid::mc::Pair* pair = (simgrid::mc::Pair*) xbt_fifo_get_item_content(item);
    int value;
    smx_simcall_t req = MC_state_get_executed_request(pair->graph_state, &value);
    if (req && req->call != SIMCALL_NONE) {
      smx_process_t issuer = MC_smx_simcall_get_issuer(req);
      const int pid = issuer->pid;

      // Serialization the (pid, value) pair:
      res.push_back(RecordTraceElement(pid, value));
    }
  }

  return std::move(res);
}

void LivenessChecker::showAcceptanceCycle(std::size_t depth)
{
  XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
  XBT_INFO("|             ACCEPTANCE CYCLE            |");
  XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
  XBT_INFO("Counter-example that violates formula :");
  simgrid::mc::dumpRecordPath();
  for (auto& s : this->getTextualTrace())
    XBT_INFO("%s", s.c_str());
  MC_print_statistics(mc_stats);
  XBT_INFO("Counter-example depth : %zd", depth);
}

std::vector<std::string> LivenessChecker::getTextualTrace() // override
{
  std::vector<std::string> trace;
  for (xbt_fifo_item_t item = xbt_fifo_get_last_item(liveness_stack);
       item; item = xbt_fifo_get_prev_item(item)) {
    simgrid::mc::Pair* pair = (simgrid::mc::Pair*) xbt_fifo_get_item_content(item);
    int value;
    smx_simcall_t req = MC_state_get_executed_request(pair->graph_state, &value);
    if (req && req->call != SIMCALL_NONE) {
      char* req_str = simgrid::mc::request_to_string(req, value, simgrid::mc::RequestType::executed);
      trace.push_back(std::string(req_str));
      xbt_free(req_str);
    }
  }
  return std::move(trace);
}

int LivenessChecker::main(void)
{
  simgrid::mc::Pair* current_pair = nullptr;
  int value, res, visited_num = -1;
  smx_simcall_t req = nullptr;
  xbt_automaton_transition_t transition_succ = nullptr;
  int cursor = 0;
  simgrid::mc::Pair* next_pair = nullptr;
  simgrid::xbt::unique_ptr<s_xbt_dynar_t> prop_values;
  simgrid::mc::VisitedPair* reached_pair = nullptr;

  while(xbt_fifo_size(liveness_stack) > 0){

    /* Get current pair */
    current_pair = (simgrid::mc::Pair*) xbt_fifo_get_item_content(xbt_fifo_get_first_item(liveness_stack));

    /* Update current state in buchi automaton */
    simgrid::mc::property_automaton->current_state = current_pair->automaton_state;

    XBT_DEBUG("********************* ( Depth = %d, search_cycle = %d, interleave size = %d, pair_num = %d, requests = %d)",
       current_pair->depth, current_pair->search_cycle,
       MC_state_interleave_size(current_pair->graph_state), current_pair->num,
       current_pair->requests);

    if (current_pair->requests > 0) {

      if (current_pair->automaton_state->type == 1 && current_pair->exploration_started == 0) {
        /* If new acceptance pair, return new pair */
        if ((reached_pair = this->insertAcceptancePair(current_pair)) == nullptr) {
          this->showAcceptanceCycle(current_pair->depth);
          return SIMGRID_MC_EXIT_LIVENESS;
        }
      }

      /* Pair already visited ? stop the exploration on the current path */
      if ((current_pair->exploration_started == 0)
        && (visited_num = this->insertVisitedPair(
          reached_pair, current_pair)) != -1) {

        if (dot_output != nullptr){
          fprintf(dot_output, "\"%d\" -> \"%d\" [%s];\n", initial_global_state->prev_pair, visited_num, initial_global_state->prev_req);
          fflush(dot_output);
        }

        XBT_DEBUG("Pair already visited (equal to pair %d), exploration on the current path stopped.", visited_num);
        current_pair->requests = 0;
        goto backtracking;

      }else{

        req = MC_state_get_request(current_pair->graph_state, &value);

         if (dot_output != nullptr) {
           if (initial_global_state->prev_pair != 0 && initial_global_state->prev_pair != current_pair->num) {
             fprintf(dot_output, "\"%d\" -> \"%d\" [%s];\n", initial_global_state->prev_pair, current_pair->num, initial_global_state->prev_req);
             xbt_free(initial_global_state->prev_req);
           }
           initial_global_state->prev_pair = current_pair->num;
           initial_global_state->prev_req = simgrid::mc::request_get_dot_output(req, value);
           if (current_pair->search_cycle)
             fprintf(dot_output, "%d [shape=doublecircle];\n", current_pair->num);
           fflush(dot_output);
         }

         char* req_str = simgrid::mc::request_to_string(req, value, simgrid::mc::RequestType::simix);
         XBT_DEBUG("Execute: %s", req_str);
         xbt_free(req_str);

         /* Set request as executed */
         MC_state_set_executed_request(current_pair->graph_state, req, value);

         /* Update mc_stats */
         mc_stats->executed_transitions++;
         if(current_pair->exploration_started == 0)
           mc_stats->visited_pairs++;

         /* Answer the request */
         simgrid::mc::handle_simcall(req, value);

         /* Wait for requests (schedules processes) */
         mc_model_checker->wait_for_requests();

         current_pair->requests--;
         current_pair->exploration_started = 1;

         /* Get values of atomic propositions (variables used in the property formula) */
         prop_values = this->getPropositionValues();

         /* Evaluate enabled/true transitions in automaton according to atomic propositions values and create new pairs */
         cursor = xbt_dynar_length(current_pair->automaton_state->out) - 1;
         while (cursor >= 0) {
           transition_succ = (xbt_automaton_transition_t)xbt_dynar_get_as(current_pair->automaton_state->out, cursor, xbt_automaton_transition_t);
           res = MC_automaton_evaluate_label(transition_succ->label, prop_values.get());
           if (res == 1 || res == 2) { /* 1 = True transition (always enabled), 2 = enabled transition according to atomic prop values */
              next_pair = new Pair();
              next_pair->graph_state = MC_state_new();
              next_pair->automaton_state = transition_succ->dst;
              next_pair->atomic_propositions = this->getPropositionValues();
              next_pair->depth = current_pair->depth + 1;
              /* Get enabled processes and insert them in the interleave set of the next graph_state */
              for (auto& p : mc_model_checker->process().simix_processes())
                if (simgrid::mc::process_is_enabled(&p.copy))
                  MC_state_interleave_process(next_pair->graph_state, &p.copy);

              next_pair->requests = MC_state_interleave_size(next_pair->graph_state);

              /* FIXME : get search_cycle value for each acceptant state */
              if (next_pair->automaton_state->type == 1 || current_pair->search_cycle)
                next_pair->search_cycle = 1;

              /* Add new pair to the exploration stack */
              xbt_fifo_unshift(liveness_stack, next_pair);

           }
           cursor--;
         }

      } /* End of visited_pair test */

    } else {

    backtracking:
      if(visited_num == -1)
        XBT_DEBUG("No more request to execute. Looking for backtracking point.");

      prop_values.reset();

      /* Traverse the stack backwards until a pair with a non empty interleave
         set is found, deleting all the pairs that have it empty in the way. */
      while ((current_pair = (simgrid::mc::Pair*) xbt_fifo_shift(liveness_stack)) != nullptr) {
        if (current_pair->requests > 0) {
          /* We found a backtracking point */
          XBT_DEBUG("Backtracking to depth %d", current_pair->depth);
          xbt_fifo_unshift(liveness_stack, current_pair);
          this->replay(liveness_stack);
          XBT_DEBUG("Backtracking done");
          break;
        }else{
          /* Delete pair */
          XBT_DEBUG("Delete pair %d at depth %d", current_pair->num, current_pair->depth);
          if (current_pair->automaton_state->type == 1)
            this->removeAcceptancePair(current_pair->num);
          delete current_pair;
        }
      }

    } /* End of if (current_pair->requests > 0) else ... */

  } /* End of while(xbt_fifo_size(liveness_stack) > 0) */

  XBT_INFO("No property violation found.");
  MC_print_statistics(mc_stats);
  return SIMGRID_MC_EXIT_SUCCESS;
}

int LivenessChecker::run()
{
  XBT_INFO("Check the liveness property %s", _sg_mc_property_file);
  MC_automaton_load(_sg_mc_property_file);
  mc_model_checker->wait_for_requests();

  XBT_DEBUG("Starting the liveness algorithm");
  _sg_mc_liveness = 1;

  /* Create exploration stack */
  liveness_stack = xbt_fifo_new();

  /* Create the initial state */
  initial_global_state = xbt_new0(s_mc_global_t, 1);

  this->prepare();
  return this->main();
}

}
}
