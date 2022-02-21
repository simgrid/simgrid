/* Copyright (c) 2007-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_LIVENESS_CHECKER_HPP
#define SIMGRID_MC_LIVENESS_CHECKER_HPP

#include "src/mc/api/State.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "xbt/automaton.hpp"

#include <list>
#include <memory>
#include <vector>

namespace simgrid {
namespace mc {

class XBT_PRIVATE Pair {
public:
  int num                               = 0;
  bool search_cycle                     = false;
  std::shared_ptr<State> graph_state    = nullptr; /* System state included */
  xbt_automaton_state_t automaton_state = nullptr;
  std::shared_ptr<const std::vector<int>> atomic_propositions;
  int requests             = 0;
  int depth                = 0;
  bool exploration_started = false;

  explicit Pair(unsigned long expanded_pairs);

  Pair(Pair const&) = delete;
  Pair& operator=(Pair const&) = delete;
};

class XBT_PRIVATE VisitedPair {
public:
  int num;
  int other_num                      = 0;       /* Dot output for */
  std::shared_ptr<State> graph_state = nullptr; /* System state included */
  xbt_automaton_state_t automaton_state;
  std::shared_ptr<const std::vector<int>> atomic_propositions;
  std::size_t heap_bytes_used = 0;
  int actors_count            = 0;

  VisitedPair(int pair_num, xbt_automaton_state_t automaton_state,
              std::shared_ptr<const std::vector<int>> atomic_propositions, std::shared_ptr<State> graph_state);
};

class XBT_PRIVATE LivenessChecker : public Exploration {
public:
  explicit LivenessChecker(Session* session);
  void run() override;
  RecordTrace get_record_trace() override;
  std::vector<std::string> get_textual_trace() override;
  void log_state() override;

private:
  std::shared_ptr<const std::vector<int>> get_proposition_values() const;
  std::shared_ptr<VisitedPair> insert_acceptance_pair(Pair* pair);
  int insert_visited_pair(std::shared_ptr<VisitedPair> visited_pair, Pair* pair);
  void show_acceptance_cycle(std::size_t depth);
  void replay();
  void remove_acceptance_pair(int pair_num);
  void purge_visited_pairs();
  void backtrack();
  std::shared_ptr<Pair> create_pair(const Pair* pair, xbt_automaton_state_t state,
                                    std::shared_ptr<const std::vector<int>> propositions);

  // A stack of (application_state, automaton_state) pairs for DFS exploration:
  std::list<std::shared_ptr<Pair>> exploration_stack_;
  std::list<std::shared_ptr<VisitedPair>> acceptance_pairs_;
  std::list<std::shared_ptr<VisitedPair>> visited_pairs_;
  unsigned long visited_pairs_count_  = 0;
  unsigned long expanded_pairs_count_ = 0;
  int previous_pair_                  = 0;
  std::string previous_request_;
};

} // namespace mc
} // namespace simgrid

#endif
