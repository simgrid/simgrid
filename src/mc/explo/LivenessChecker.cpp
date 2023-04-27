/* Copyright (c) 2011-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/LivenessChecker.hpp"
#include "src/mc/api/RemoteApp.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/mc/mc_private.hpp"

#include <boost/range/algorithm.hpp>
#include <cstring>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_liveness, mc, "Logging specific to algorithms for liveness properties verification");

/********* Static functions *********/

namespace simgrid::mc {

VisitedPair::VisitedPair(int pair_num, xbt_automaton_state_t prop_state,
                         std::shared_ptr<const std::vector<int>> atomic_propositions, std::shared_ptr<State> app_state,
                         RemoteApp& remote_app)
    : num(pair_num), prop_state_(prop_state)
{
  auto* memory     = remote_app.get_remote_process_memory();
  this->app_state_ = std::move(app_state);
  if (not this->app_state_->get_system_state())
    this->app_state_->set_system_state(std::make_shared<Snapshot>(pair_num, remote_app.get_page_store(), *memory));
  this->heap_bytes_used     = memory->get_remote_heap_bytes();
  this->actor_count_        = app_state_->get_actor_count();
  this->other_num           = -1;
  this->atomic_propositions = std::move(atomic_propositions);
}

bool LivenessChecker::evaluate_label(const xbt_automaton_exp_label* l, std::vector<int> const& values)
{
  switch (l->type) {
    case xbt_automaton_exp_label::AUT_OR:
      return evaluate_label(l->u.or_and.left_exp, values) || evaluate_label(l->u.or_and.right_exp, values);
    case xbt_automaton_exp_label::AUT_AND:
      return evaluate_label(l->u.or_and.left_exp, values) && evaluate_label(l->u.or_and.right_exp, values);
    case xbt_automaton_exp_label::AUT_NOT:
      return not evaluate_label(l->u.exp_not, values);
    case xbt_automaton_exp_label::AUT_PREDICAT:
      return values.at(compare_automaton_exp_label(l)) != 0;
    case xbt_automaton_exp_label::AUT_ONE:
      return true;
    default:
      xbt_die("Unexpected value for automaton");
  }
}

Pair::Pair(unsigned long expanded_pairs) : num(expanded_pairs) {}

std::shared_ptr<const std::vector<int>> LivenessChecker::get_proposition_values() const
{
  auto values = automaton_propositional_symbol_evaluate();
  return std::make_shared<const std::vector<int>>(std::move(values));
}

std::shared_ptr<VisitedPair> LivenessChecker::insert_acceptance_pair(simgrid::mc::Pair* pair)
{
  auto new_pair = std::make_shared<VisitedPair>(pair->num, pair->prop_state_, pair->atomic_propositions,
                                                pair->app_state_, get_remote_app());

  auto [res_begin,
        res_end] = boost::range::equal_range(acceptance_pairs_, new_pair.get(), [](auto const& a, auto const& b) {
    return std::make_pair(a->actor_count_, a->heap_bytes_used) < std::make_pair(b->actor_count_, b->heap_bytes_used);
  });

  if (pair->search_cycle)
    for (auto i = res_begin; i != res_end; ++i) {
      std::shared_ptr<simgrid::mc::VisitedPair> const& pair_test = *i;
      if (xbt_automaton_state_compare(pair_test->prop_state_, new_pair->prop_state_) != 0 ||
          *(pair_test->atomic_propositions) != *(new_pair->atomic_propositions) ||
          (not pair_test->app_state_->get_system_state()->equals_to(*new_pair->app_state_->get_system_state(),
                                                                    *get_remote_app().get_remote_process_memory())))
        continue;
      XBT_INFO("Pair %d already reached (equal to pair %d) !", new_pair->num, pair_test->num);
      exploration_stack_.pop_back();
      dot_output("\"%d\" -> \"%d\" [%s];\n", this->previous_pair_, pair_test->num, this->previous_request_.c_str());
      return nullptr;
    }

  acceptance_pairs_.insert(res_begin, new_pair);
  return new_pair;
}

void LivenessChecker::remove_acceptance_pair(int pair_num)
{
  for (auto i = acceptance_pairs_.begin(); i != acceptance_pairs_.end(); ++i)
    if ((*i)->num == pair_num) {
      acceptance_pairs_.erase(i);
      break;
    }
}

void LivenessChecker::replay()
{
  XBT_DEBUG("**** Begin Replay ****");

  /* Intermediate backtracking */
  if (_sg_mc_checkpoint > 0) {
    const Pair* pair = exploration_stack_.back().get();
    if (const auto* system_state = pair->app_state_->get_system_state()) {
      system_state->restore(*get_remote_app().get_remote_process_memory());
      return;
    }
  }

  get_remote_app().restore_initial_state();

  /* Traverse the stack from the initial state and re-execute the transitions */
  int depth = 1;
  for (std::shared_ptr<Pair> const& pair : exploration_stack_) {
    if (pair == exploration_stack_.back())
      break;

    std::shared_ptr<State> state = pair->app_state_;

    if (pair->exploration_started) {
      state->get_transition_out()->replay(get_remote_app());
      XBT_DEBUG("Replay (depth = %d) : %s (%p)", depth, state->get_transition_out()->to_string().c_str(), state.get());
    }

    /* Update statistics */
    visited_pairs_count_++;
    depth++;
  }
  XBT_DEBUG("**** End Replay ****");
}

/**
 * @brief Checks whether a given pair has already been visited by the algorithm.
 */
int LivenessChecker::insert_visited_pair(std::shared_ptr<VisitedPair> visited_pair, simgrid::mc::Pair* pair)
{
  if (_sg_mc_max_visited_states == 0)
    return -1;

  if (visited_pair == nullptr)
    visited_pair = std::make_shared<VisitedPair>(pair->num, pair->prop_state_, pair->atomic_propositions,
                                                 pair->app_state_, get_remote_app());

  auto [range_begin,
        range_end] = boost::range::equal_range(visited_pairs_, visited_pair.get(), [](auto const& a, auto const& b) {
    return std::make_pair(a->actor_count_, a->heap_bytes_used) < std::make_pair(b->actor_count_, b->heap_bytes_used);
  });

  for (auto i = range_begin; i != range_end; ++i) {
    const VisitedPair* pair_test = i->get();
    if (xbt_automaton_state_compare(pair_test->prop_state_, visited_pair->prop_state_) != 0 ||
        *(pair_test->atomic_propositions) != *(visited_pair->atomic_propositions) ||
        (not pair_test->app_state_->get_system_state()->equals_to(*visited_pair->app_state_->get_system_state(),
                                                                  *get_remote_app().get_remote_process_memory())))
      continue;
    if (pair_test->other_num == -1)
      visited_pair->other_num = pair_test->num;
    else
      visited_pair->other_num = pair_test->other_num;
    XBT_DEBUG("Pair %d already visited ! (equal to pair %d (pair %d in dot_output))", visited_pair->num, pair_test->num,
              visited_pair->other_num);
    (*i) = std::move(visited_pair);
    return (*i)->other_num;
  }

  visited_pairs_.insert(range_begin, std::move(visited_pair));
  this->purge_visited_pairs();
  return -1;
}

void LivenessChecker::purge_visited_pairs()
{
  if (_sg_mc_max_visited_states != 0 && visited_pairs_.size() > (std::size_t)_sg_mc_max_visited_states) {
    // Remove the oldest entry with a linear search:
    visited_pairs_.erase(
        boost::min_element(visited_pairs_, [](std::shared_ptr<VisitedPair> const a,
                                              std::shared_ptr<VisitedPair> const& b) { return a->num < b->num; }));
  }
}

LivenessChecker::LivenessChecker(const std::vector<char*>& args) : Exploration(args, true) {}
LivenessChecker::~LivenessChecker()
{
  xbt_automaton_free(property_automaton_);
}

xbt_automaton_t LivenessChecker::property_automaton_ = nullptr;

void LivenessChecker::automaton_load(const char* file) const
{
  if (property_automaton_ == nullptr)
    property_automaton_ = xbt_automaton_new();

  xbt_automaton_load(property_automaton_, file);
}

std::vector<int> LivenessChecker::automaton_propositional_symbol_evaluate() const
{
  unsigned int cursor = 0;
  std::vector<int> values;
  xbt_automaton_propositional_symbol_t ps = nullptr;
  xbt_dynar_foreach (property_automaton_->propositional_symbols, cursor, ps)
    values.push_back(xbt_automaton_propositional_symbol_evaluate(ps));
  return values;
}

std::vector<xbt_automaton_state_t> LivenessChecker::get_automaton_state() const
{
  std::vector<xbt_automaton_state_t> automaton_stack;
  unsigned int cursor = 0;
  xbt_automaton_state_t automaton_state;
  xbt_dynar_foreach (property_automaton_->states, cursor, automaton_state)
    if (automaton_state->type == -1)
      automaton_stack.push_back(automaton_state);
  return automaton_stack;
}

int LivenessChecker::compare_automaton_exp_label(const xbt_automaton_exp_label* l) const
{
  unsigned int cursor                    = 0;
  xbt_automaton_propositional_symbol_t p = nullptr;
  xbt_dynar_foreach (property_automaton_->propositional_symbols, cursor, p) {
    if (std::strcmp(xbt_automaton_propositional_symbol_get_name(p), l->u.predicat) == 0)
      return cursor;
  }
  return -1;
}

void LivenessChecker::set_property_automaton(xbt_automaton_state_t const& automaton_state) const
{
  property_automaton_->current_state = automaton_state;
}

xbt_automaton_exp_label_t LivenessChecker::get_automaton_transition_label(xbt_dynar_t const& dynar, int index) const
{
  const xbt_automaton_transition* transition = xbt_dynar_get_as(dynar, index, xbt_automaton_transition_t);
  return transition->label;
}

xbt_automaton_state_t LivenessChecker::get_automaton_transition_dst(xbt_dynar_t const& dynar, int index) const
{
  const xbt_automaton_transition* transition = xbt_dynar_get_as(dynar, index, xbt_automaton_transition_t);
  return transition->dst;
}
void LivenessChecker::automaton_register_symbol(RemoteProcessMemory const& remote_process, const char* name,
                                                RemotePtr<int> address)
{
  if (property_automaton_ == nullptr)
    property_automaton_ = xbt_automaton_new();

  xbt::add_proposition(property_automaton_, name,
                       [&remote_process, address]() { return remote_process.read(address); });
}

RecordTrace LivenessChecker::get_record_trace() // override
{
  RecordTrace res;
  for (std::shared_ptr<Pair> const& pair : exploration_stack_)
    if (pair->app_state_->get_transition_out() != nullptr)
      res.push_back(pair->app_state_->get_transition_out().get());
  return res;
}

void LivenessChecker::log_state() // override
{
  XBT_INFO("Expanded pairs = %lu", expanded_pairs_count_);
  XBT_INFO("Visited pairs = %lu", visited_pairs_count_);
  XBT_INFO("Executed transitions = %lu", Transition::get_executed_transitions());
  Exploration::log_state();
}

void LivenessChecker::show_acceptance_cycle(std::size_t depth)
{
  XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
  XBT_INFO("|             ACCEPTANCE CYCLE            |");
  XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
  XBT_INFO("Counter-example that violates formula:");
  for (auto const& s : this->get_textual_trace())
    XBT_INFO("  %s", s.c_str());
  XBT_INFO("You can debug the problem (and see the whole details) by rerunning out of simgrid-mc with "
           "--cfg=model-check/replay:'%s'",
           get_record_trace().to_string().c_str());
  log_state();
  XBT_INFO("Counter-example depth: %zu", depth);
}

std::shared_ptr<Pair> LivenessChecker::create_pair(const Pair* current_pair, xbt_automaton_state_t state,
                                                   std::shared_ptr<const std::vector<int>> propositions)
{
  ++expanded_pairs_count_;
  auto next_pair                 = std::make_shared<Pair>(expanded_pairs_count_);
  next_pair->prop_state_         = state;
  next_pair->app_state_          = std::make_shared<State>(get_remote_app());
  next_pair->atomic_propositions = std::move(propositions);
  if (current_pair)
    next_pair->depth = current_pair->depth + 1;
  else
    next_pair->depth = 1;
  /* Add all enabled actors to the interleave set of the initial state */
  for (auto const& [aid, _] : next_pair->app_state_->get_actors_list())
    if (next_pair->app_state_->is_actor_enabled(aid))
      next_pair->app_state_->consider_one(aid);

  next_pair->requests = next_pair->app_state_->count_todo();
  /* FIXME : get search_cycle value for each accepting state */
  if (next_pair->prop_state_->type == 1 || (current_pair && current_pair->search_cycle))
    next_pair->search_cycle = true;
  else
    next_pair->search_cycle = false;
  return next_pair;
}

void LivenessChecker::backtrack()
{
  /* Traverse the stack backwards until a pair with a non empty interleave
     set is found, deleting all the pairs that have it empty in the way. */
  while (not exploration_stack_.empty()) {
    std::shared_ptr<simgrid::mc::Pair> current_pair = exploration_stack_.back();
    exploration_stack_.pop_back();
    if (current_pair->requests > 0) {
      /* We found a backtracking point */
      XBT_DEBUG("Backtracking to depth %d", current_pair->depth);
      exploration_stack_.push_back(std::move(current_pair));
      this->replay();
      XBT_DEBUG("Backtracking done");
      break;
    } else {
      XBT_DEBUG("Delete pair %d at depth %d", current_pair->num, current_pair->depth);
      if (current_pair->prop_state_->type == 1)
        this->remove_acceptance_pair(current_pair->num);
    }
  }
}

void LivenessChecker::run()
{
  XBT_INFO("Check the liveness property %s", _sg_mc_property_file.get().c_str());
  automaton_load(_sg_mc_property_file.get().c_str());

  XBT_DEBUG("Starting the liveness algorithm");

  /* Initialize */
  this->previous_pair_ = 0;

  std::shared_ptr<const std::vector<int>> propos = this->get_proposition_values();

  // For each initial state of the property automaton, push a
  // (application_state, automaton_state) pair to the exploration stack:
  auto automaton_stack = get_automaton_state();
  for (auto* automaton_state : automaton_stack) {
    if (automaton_state->type == -1)
      exploration_stack_.push_back(this->create_pair(nullptr, automaton_state, propos));
  }

  /* Actually run the double DFS search for counter-examples */
  while (not exploration_stack_.empty()) {
    std::shared_ptr<Pair> current_pair = exploration_stack_.back();

    /* Update current state in buchi automaton */
    set_property_automaton(current_pair->prop_state_);

    XBT_DEBUG(
        "********************* ( Depth = %d, search_cycle = %d, interleave size = %zu, pair_num = %d, requests = %d)",
        current_pair->depth, current_pair->search_cycle, current_pair->app_state_->count_todo(), current_pair->num,
        current_pair->requests);

    if (current_pair->requests == 0) {
      this->backtrack();
      continue;
    }

    std::shared_ptr<VisitedPair> reached_pair;
    if (current_pair->prop_state_->type == 1 && not current_pair->exploration_started) {
      reached_pair = this->insert_acceptance_pair(current_pair.get());
      if (reached_pair == nullptr) {
        this->show_acceptance_cycle(current_pair->depth);
        throw McError(ExitStatus::LIVENESS);
      }
    }

    /* Pair already visited ? stop the exploration on the current path */
    if (not current_pair->exploration_started) {
      int visited_num = this->insert_visited_pair(reached_pair, current_pair.get());
      if (visited_num != -1) {
        dot_output("\"%d\" -> \"%d\" [%s];\n", this->previous_pair_, visited_num, this->previous_request_.c_str());

        XBT_DEBUG("Pair already visited (equal to pair %d), exploration on the current path stopped.", visited_num);
        current_pair->requests = 0;
        this->backtrack();
        continue;
      }
    }

    current_pair->app_state_->execute_next(current_pair->app_state_->next_transition(), get_remote_app());
    XBT_DEBUG("Execute: %s", current_pair->app_state_->get_transition_out()->to_string().c_str());

    /* Update the dot output */
    if (this->previous_pair_ != 0 && this->previous_pair_ != current_pair->num) {
      dot_output("\"%d\" -> \"%d\" [%s];\n", this->previous_pair_, current_pair->num, this->previous_request_.c_str());
      this->previous_request_.clear();
    }
    this->previous_pair_    = current_pair->num;
    this->previous_request_ = current_pair->app_state_->get_transition_out()->dot_string();
    if (current_pair->search_cycle)
      dot_output("%d [shape=doublecircle];\n", current_pair->num);

    if (not current_pair->exploration_started)
      visited_pairs_count_++;

    current_pair->requests--;
    current_pair->exploration_started = true;

    /* Get values of atomic propositions (variables used in the property formula) */
    std::shared_ptr<const std::vector<int>> prop_values = this->get_proposition_values();

    // For each enabled transition in the property automaton, push a
    // (application_state, automaton_state) pair to the exploration stack:
    for (int i = xbt_dynar_length(current_pair->prop_state_->out) - 1; i >= 0; i--) {
      const auto* transition_succ_label = get_automaton_transition_label(current_pair->prop_state_->out, i);
      auto* transition_succ_dst         = get_automaton_transition_dst(current_pair->prop_state_->out, i);
      if (evaluate_label(transition_succ_label, *prop_values))
        exploration_stack_.push_back(this->create_pair(current_pair.get(), transition_succ_dst, prop_values));
    }
  }

  XBT_INFO("No property violation found.");
  log_state();
}

Exploration* create_liveness_checker(const std::vector<char*>& args)
{
  return new LivenessChecker(args);
}

} // namespace simgrid::mc
