/* Copyright (c) 2011-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstring>

#include <algorithm>
#include <memory>
#include <list>

#include <unistd.h>
#include <sys/wait.h>

#include <xbt/automaton.h>
#include <xbt/dynar.h>
#include <xbt/dynar.hpp>
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

/********* Static functions *********/

namespace simgrid {
namespace mc {

VisitedPair::VisitedPair(
  int pair_num, xbt_automaton_state_t automaton_state,
  std::vector<int> const& atomic_propositions, std::shared_ptr<simgrid::mc::State> graph_state)
{
  simgrid::mc::Process* process = &(mc_model_checker->process());

  this->graph_state = std::move(graph_state);
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
  this->atomic_propositions = atomic_propositions;
}

VisitedPair::~VisitedPair()
{
}

static bool evaluate_label(
  xbt_automaton_exp_label_t l, std::vector<int> const& values)
{
  switch (l->type) {
  case xbt_automaton_exp_label::AUT_OR:
    return evaluate_label(l->u.or_and.left_exp, values)
      || evaluate_label(l->u.or_and.right_exp, values);
  case xbt_automaton_exp_label::AUT_AND:
    return evaluate_label(l->u.or_and.left_exp, values)
      && evaluate_label(l->u.or_and.right_exp, values);
  case xbt_automaton_exp_label::AUT_NOT:
    return !evaluate_label(l->u.exp_not, values);
  case xbt_automaton_exp_label::AUT_PREDICAT:{
      unsigned int cursor = 0;
      xbt_automaton_propositional_symbol_t p = nullptr;
      xbt_dynar_foreach(simgrid::mc::property_automaton->propositional_symbols, cursor, p) {
        if (std::strcmp(xbt_automaton_propositional_symbol_get_name(p), l->u.predicat) == 0)
          return values[cursor] != 0;
      }
      xbt_die("Missing predicate");
    }
  case xbt_automaton_exp_label::AUT_ONE:
    return true;
  default:
    xbt_die("Unexpected vaue for automaton");
  }
}

Pair::Pair() : num(++mc_stats->expanded_pairs)
{}

Pair::~Pair() {}

std::vector<int> LivenessChecker::getPropositionValues()
{
  std::vector<int> values;
  unsigned int cursor = 0;
  xbt_automaton_propositional_symbol_t ps = nullptr;
  xbt_dynar_foreach(simgrid::mc::property_automaton->propositional_symbols, cursor, ps)
    values.push_back(xbt_automaton_propositional_symbol_evaluate(ps));
  return values;
}

int LivenessChecker::compare(simgrid::mc::VisitedPair* state1, simgrid::mc::VisitedPair* state2)
{
  simgrid::mc::Snapshot* s1 = state1->graph_state->system_state.get();
  simgrid::mc::Snapshot* s2 = state2->graph_state->system_state.get();
  int num1 = state1->num;
  int num2 = state2->num;
  return simgrid::mc::snapshot_compare(num1, s1, num2, s2);
}

std::shared_ptr<VisitedPair> LivenessChecker::insertAcceptancePair(simgrid::mc::Pair* pair)
{
  std::shared_ptr<VisitedPair> new_pair = std::make_shared<VisitedPair>(
    pair->num, pair->automaton_state, pair->atomic_propositions,
    pair->graph_state);

  auto res = std::equal_range(acceptancePairs_.begin(), acceptancePairs_.end(),
    new_pair.get(), simgrid::mc::DerefAndCompareByNbProcessesAndUsedHeap());

  if (pair->search_cycle) for (auto i = res.first; i != res.second; ++i) {
    std::shared_ptr<simgrid::mc::VisitedPair> const& pair_test = *i;
    if (xbt_automaton_state_compare(
          pair_test->automaton_state, new_pair->automaton_state) != 0
        || pair_test->atomic_propositions != new_pair->atomic_propositions
        || this->compare(pair_test.get(), new_pair.get()) != 0)
      continue;
    XBT_INFO("Pair %d already reached (equal to pair %d) !",
      new_pair->num, pair_test->num);
    livenessStack_.pop_back();
    if (dot_output != nullptr)
      fprintf(dot_output, "\"%d\" -> \"%d\" [%s];\n",
        initial_global_state->prev_pair, pair_test->num,
        initial_global_state->prev_req);
    return nullptr;
  }

  acceptancePairs_.insert(res.first, new_pair);
  return new_pair;
}

void LivenessChecker::removeAcceptancePair(int pair_num)
{
  for (auto i = acceptancePairs_.begin(); i != acceptancePairs_.end(); ++i)
    if ((*i)->num == pair_num) {
      acceptancePairs_.erase(i);
      break;
    }
}

void LivenessChecker::prepare(void)
{
  mc_model_checker->wait_for_requests();

  initial_global_state->snapshot = simgrid::mc::take_snapshot(0);
  initial_global_state->prev_pair = 0;

  unsigned int cursor = 0;
  xbt_automaton_state_t automaton_state;

  xbt_dynar_foreach(simgrid::mc::property_automaton->states, cursor, automaton_state) {
    if (automaton_state->type == -1) {  /* Initial automaton state */

      std::shared_ptr<Pair> initial_pair = std::make_shared<Pair>();
      initial_pair->automaton_state = automaton_state;
      initial_pair->graph_state = std::shared_ptr<simgrid::mc::State>(MC_state_new());
      initial_pair->atomic_propositions = this->getPropositionValues();
      initial_pair->depth = 1;

      /* Get enabled processes and insert them in the interleave set of the graph_state */
      for (auto& p : mc_model_checker->process().simix_processes())
        if (simgrid::mc::process_is_enabled(&p.copy))
          MC_state_interleave_process(initial_pair->graph_state.get(), &p.copy);

      initial_pair->requests = MC_state_interleave_size(initial_pair->graph_state.get());
      initial_pair->search_cycle = false;

      livenessStack_.push_back(std::move(initial_pair));
    }
  }
}


void LivenessChecker::replay()
{
  XBT_DEBUG("**** Begin Replay ****");

  /* Intermediate backtracking */
  if(_sg_mc_checkpoint > 0) {
    simgrid::mc::Pair* pair = livenessStack_.back().get();
    if(pair->graph_state->system_state){
      simgrid::mc::restore_snapshot(pair->graph_state->system_state);
      return;
    }
  }

  /* Restore the initial state */
  simgrid::mc::restore_snapshot(initial_global_state->snapshot);

  /* Traverse the stack from the initial state and re-execute the transitions */
  int depth = 1;
  for (std::shared_ptr<Pair> const& pair : livenessStack_) {
    if (pair == livenessStack_.back())
      break;

      std::shared_ptr<State> state = pair->graph_state;

      if (pair->exploration_started) {

        int value;
        smx_simcall_t saved_req = MC_state_get_executed_request(state.get(), &value);

        smx_simcall_t req = nullptr;

        if (saved_req != nullptr) {
          /* because we got a copy of the executed request, we have to fetch the
             real one, pointed by the request field of the issuer process */
          const smx_process_t issuer = MC_smx_simcall_get_issuer(saved_req);
          req = &issuer->simcall;

          /* Debug information */
          if (XBT_LOG_ISENABLED(mc_liveness, xbt_log_priority_debug)) {
            char* req_str = simgrid::mc::request_to_string(req, value, simgrid::mc::RequestType::simix);
            XBT_DEBUG("Replay (depth = %d) : %s (%p)", depth, req_str, state.get());
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
int LivenessChecker::insertVisitedPair(std::shared_ptr<VisitedPair> visited_pair, simgrid::mc::Pair* pair)
{
  if (_sg_mc_visited == 0)
    return -1;

  if (visited_pair == nullptr)
    visited_pair = std::make_shared<VisitedPair>(
      pair->num, pair->automaton_state, pair->atomic_propositions,
      pair->graph_state);

  auto range = std::equal_range(visitedPairs_.begin(), visitedPairs_.end(),
    visited_pair.get(), simgrid::mc::DerefAndCompareByNbProcessesAndUsedHeap());

  for (auto i = range.first; i != range.second; ++i) {
    VisitedPair* pair_test = i->get();
    if (xbt_automaton_state_compare(
          pair_test->automaton_state, visited_pair->automaton_state) != 0
        || pair_test->atomic_propositions != visited_pair->atomic_propositions
        || this->compare(pair_test, visited_pair.get()) != 0)
        continue;
    if (pair_test->other_num == -1)
      visited_pair->other_num = pair_test->num;
    else
      visited_pair->other_num = pair_test->other_num;
    if (dot_output == nullptr)
      XBT_DEBUG("Pair %d already visited ! (equal to pair %d)",
      visited_pair->num, pair_test->num);
    else
      XBT_DEBUG("Pair %d already visited ! (equal to pair %d (pair %d in dot_output))",
        visited_pair->num, pair_test->num, visited_pair->other_num);
    (*i) = std::move(visited_pair);
    return (*i)->other_num;
  }

  visitedPairs_.insert(range.first, std::move(visited_pair));
  this->purgeVisitedPairs();
  return -1;
}

void LivenessChecker::purgeVisitedPairs()
{
  if (_sg_mc_visited != 0 && visitedPairs_.size() > (std::size_t) _sg_mc_visited) {
    // Remove the oldest entry with a linear search:
    visitedPairs_.erase(std::min_element(
      visitedPairs_.begin(), visitedPairs_.end(),
      [](std::shared_ptr<VisitedPair> const a, std::shared_ptr<VisitedPair> const& b) {
        return a->num < b->num; } ));
  }
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
  for (std::shared_ptr<Pair> const& pair : livenessStack_) {
    int value;
    smx_simcall_t req = MC_state_get_executed_request(pair->graph_state.get(), &value);
    if (req && req->call != SIMCALL_NONE) {
      smx_process_t issuer = MC_smx_simcall_get_issuer(req);
      const int pid = issuer->pid;

      // Serialization the (pid, value) pair:
      res.push_back(RecordTraceElement(pid, value));
    }
  }
  return res;
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
  for (std::shared_ptr<Pair> const& pair : livenessStack_) {
    int value;
    smx_simcall_t req = MC_state_get_executed_request(pair->graph_state.get(), &value);
    if (req && req->call != SIMCALL_NONE) {
      char* req_str = simgrid::mc::request_to_string(req, value, simgrid::mc::RequestType::executed);
      trace.push_back(std::string(req_str));
      xbt_free(req_str);
    }
  }
  return trace;
}

int LivenessChecker::main(void)
{
  int visited_num = -1;

  while (!livenessStack_.empty()){

    /* Get current pair */
    std::shared_ptr<Pair> current_pair = livenessStack_.back();

    /* Update current state in buchi automaton */
    simgrid::mc::property_automaton->current_state = current_pair->automaton_state;

    XBT_DEBUG("********************* ( Depth = %d, search_cycle = %d, interleave size = %d, pair_num = %d, requests = %d)",
       current_pair->depth, current_pair->search_cycle,
       MC_state_interleave_size(current_pair->graph_state.get()), current_pair->num,
       current_pair->requests);

    if (current_pair->requests > 0) {

      std::shared_ptr<VisitedPair> reached_pair;
      if (current_pair->automaton_state->type == 1 && !current_pair->exploration_started
          && (reached_pair = this->insertAcceptancePair(current_pair.get())) == nullptr) {
        this->showAcceptanceCycle(current_pair->depth);
        return SIMGRID_MC_EXIT_LIVENESS;
      }

      /* Pair already visited ? stop the exploration on the current path */
      if ((!current_pair->exploration_started)
        && (visited_num = this->insertVisitedPair(
          reached_pair, current_pair.get())) != -1) {

        if (dot_output != nullptr){
          fprintf(dot_output, "\"%d\" -> \"%d\" [%s];\n", initial_global_state->prev_pair, visited_num, initial_global_state->prev_req);
          fflush(dot_output);
        }

        XBT_DEBUG("Pair already visited (equal to pair %d), exploration on the current path stopped.", visited_num);
        current_pair->requests = 0;
        goto backtracking;

      }else{

        int value;
        smx_simcall_t req = MC_state_get_request(current_pair->graph_state.get(), &value);

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
         MC_state_set_executed_request(current_pair->graph_state.get(), req, value);

         /* Update mc_stats */
         mc_stats->executed_transitions++;
         if (!current_pair->exploration_started)
           mc_stats->visited_pairs++;

         /* Answer the request */
         simgrid::mc::handle_simcall(req, value);

         /* Wait for requests (schedules processes) */
         mc_model_checker->wait_for_requests();

         current_pair->requests--;
         current_pair->exploration_started = true;

         /* Get values of atomic propositions (variables used in the property formula) */
         std::vector<int> prop_values = this->getPropositionValues();

         /* Evaluate enabled/true transitions in automaton according to atomic propositions values and create new pairs */
         int cursor = xbt_dynar_length(current_pair->automaton_state->out) - 1;
         while (cursor >= 0) {
           xbt_automaton_transition_t transition_succ = (xbt_automaton_transition_t)xbt_dynar_get_as(current_pair->automaton_state->out, cursor, xbt_automaton_transition_t);
           if (evaluate_label(transition_succ->label, prop_values)) {
              std::shared_ptr<Pair> next_pair = std::make_shared<Pair>();
              next_pair->graph_state = std::shared_ptr<simgrid::mc::State>(MC_state_new());
              next_pair->automaton_state = transition_succ->dst;
              next_pair->atomic_propositions = this->getPropositionValues();
              next_pair->depth = current_pair->depth + 1;
              /* Get enabled processes and insert them in the interleave set of the next graph_state */
              for (auto& p : mc_model_checker->process().simix_processes())
                if (simgrid::mc::process_is_enabled(&p.copy))
                  MC_state_interleave_process(next_pair->graph_state.get(), &p.copy);

              next_pair->requests = MC_state_interleave_size(next_pair->graph_state.get());

              /* FIXME : get search_cycle value for each acceptant state */
              if (next_pair->automaton_state->type == 1 || current_pair->search_cycle)
                next_pair->search_cycle = true;

              /* Add new pair to the exploration stack */
              livenessStack_.push_back(std::move(next_pair));

           }
           cursor--;
         }

      } /* End of visited_pair test */

    } else {

    backtracking:
      if(visited_num == -1)
        XBT_DEBUG("No more request to execute. Looking for backtracking point.");

      /* Traverse the stack backwards until a pair with a non empty interleave
         set is found, deleting all the pairs that have it empty in the way. */
      while (!livenessStack_.empty()) {
        std::shared_ptr<simgrid::mc::Pair> current_pair = livenessStack_.back();
        livenessStack_.pop_back();
        if (current_pair->requests > 0) {
          /* We found a backtracking point */
          XBT_DEBUG("Backtracking to depth %d", current_pair->depth);
          livenessStack_.push_back(std::move(current_pair));
          this->replay();
          XBT_DEBUG("Backtracking done");
          break;
        }else{
          XBT_DEBUG("Delete pair %d at depth %d", current_pair->num, current_pair->depth);
          if (current_pair->automaton_state->type == 1)
            this->removeAcceptancePair(current_pair->num);
        }
      }

    } /* End of if (current_pair->requests > 0) else ... */

  }

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

  /* Create the initial state */
  simgrid::mc::initial_global_state = std::unique_ptr<s_mc_global_t>(new s_mc_global_t());

  this->prepare();
  int res = this->main();
  simgrid::mc::initial_global_state = nullptr;

  return res;
}

Checker* createLivenessChecker(Session& session)
{
  return new LivenessChecker(session);
}

}
}
