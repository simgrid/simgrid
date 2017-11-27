/* Copyright (c) 2007-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_LIVENESS_CHECKER_HPP
#define SIMGRID_MC_LIVENESS_CHECKER_HPP

#include <cstddef>

#include <string>
#include <list>
#include <memory>
#include <vector>

#include "src/mc/checker/Checker.hpp"
#include "src/mc/mc_state.hpp"
#include <simgrid_config.h>
#include <xbt/automaton.h>
#include <xbt/base.h>

namespace simgrid {
namespace mc {

class XBT_PRIVATE Pair {
public:
  int num = 0;
  bool search_cycle = false;
  std::shared_ptr<simgrid::mc::State> graph_state = nullptr; /* System state included */
  xbt_automaton_state_t automaton_state = nullptr;
  std::shared_ptr<const std::vector<int>> atomic_propositions;
  int requests = 0;
  int depth = 0;
  bool exploration_started = false;

  explicit Pair(unsigned long expanded_pairs);
  ~Pair() = default;

  Pair(Pair const&) = delete;
  Pair& operator=(Pair const&) = delete;
};

class XBT_PRIVATE VisitedPair {
public:
  int num;
  int other_num = 0; /* Dot output for */
  std::shared_ptr<simgrid::mc::State> graph_state = nullptr; /* System state included */
  xbt_automaton_state_t automaton_state;
  std::shared_ptr<const std::vector<int>> atomic_propositions;
  std::size_t heap_bytes_used = 0;
  int actors_count            = 0;

  VisitedPair(
    int pair_num, xbt_automaton_state_t automaton_state,
    std::shared_ptr<const std::vector<int>> atomic_propositions,
    std::shared_ptr<simgrid::mc::State> graph_state);
  ~VisitedPair() = default;
};

class XBT_PRIVATE LivenessChecker : public Checker {
public:
  explicit LivenessChecker(Session& session);
  ~LivenessChecker() = default;
  void run() override;
  RecordTrace getRecordTrace() override;
  std::vector<std::string> getTextualTrace() override;
  void logState() override;
private:
  int compare(simgrid::mc::VisitedPair* state1, simgrid::mc::VisitedPair* state2);
  std::shared_ptr<const std::vector<int>> getPropositionValues();
  std::shared_ptr<VisitedPair> insertAcceptancePair(simgrid::mc::Pair* pair);
  int insertVisitedPair(std::shared_ptr<VisitedPair> visited_pair, simgrid::mc::Pair* pair);
  void showAcceptanceCycle(std::size_t depth);
  void replay();
  void removeAcceptancePair(int pair_num);
  void purgeVisitedPairs();
  void backtrack();
  std::shared_ptr<Pair> newPair(Pair* pair, xbt_automaton_state_t state, std::shared_ptr<const std::vector<int>> propositions);

  // A stack of (application_state, automaton_state) pairs for DFS exploration:
  std::list<std::shared_ptr<Pair>> explorationStack_;
  std::list<std::shared_ptr<VisitedPair>> acceptancePairs_;
  std::list<std::shared_ptr<VisitedPair>> visitedPairs_;
  unsigned long visitedPairsCount_ = 0;
  unsigned long expandedPairsCount_ = 0;
  unsigned long expandedStatesCount_ = 0;
  int previousPair_ = 0;
  std::string previousRequest_;
};

}
}

#endif
