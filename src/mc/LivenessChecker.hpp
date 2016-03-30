/* Copyright (c) 2007-2015. The SimGrid Team.
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

#include <simgrid_config.h>
#include <xbt/base.h>
#include <xbt/dynar.h>
#include <xbt/automaton.h>
#include <xbt/memory.hpp>
#include "src/mc/mc_state.h"
#include "src/mc/Checker.hpp"

SG_BEGIN_DECL()

SG_END_DECL()

namespace simgrid {
namespace mc {

extern XBT_PRIVATE xbt_automaton_t property_automaton;

struct XBT_PRIVATE Pair {
  int num = 0;
  bool search_cycle = false;
  std::shared_ptr<simgrid::mc::State> graph_state = nullptr; /* System state included */
  xbt_automaton_state_t automaton_state = nullptr;
  std::vector<int> atomic_propositions;
  int requests = 0;
  int depth = 0;
  bool exploration_started = false;

  Pair();
  ~Pair();

  Pair(Pair const&) = delete;
  Pair& operator=(Pair const&) = delete;
};

struct XBT_PRIVATE VisitedPair {
  int num = 0;
  int other_num = 0; /* Dot output for */
  std::shared_ptr<simgrid::mc::State> graph_state = nullptr; /* System state included */
  xbt_automaton_state_t automaton_state = nullptr;
  std::vector<int> atomic_propositions;
  std::size_t heap_bytes_used = 0;
  int nb_processes = 0;

  VisitedPair(
    int pair_num, xbt_automaton_state_t automaton_state,
    std::vector<int> const& atomic_propositions,
    std::shared_ptr<simgrid::mc::State> graph_state);
  ~VisitedPair();
};

class XBT_PRIVATE LivenessChecker : public Checker {
public:
  LivenessChecker(Session& session);
  ~LivenessChecker();
  int run() override;
  RecordTrace getRecordTrace() override;
  std::vector<std::string> getTextualTrace() override;
private:
  int main();
  void prepare();
  int compare(simgrid::mc::VisitedPair* state1, simgrid::mc::VisitedPair* state2);
  std::vector<int> getPropositionValues();
  std::shared_ptr<VisitedPair> insertAcceptancePair(simgrid::mc::Pair* pair);
  int insertVisitedPair(std::shared_ptr<VisitedPair> visited_pair, simgrid::mc::Pair* pair);
  void showAcceptanceCycle(std::size_t depth);
  void replay();
  void removeAcceptancePair(int pair_num);
public:
  std::list<std::shared_ptr<VisitedPair>> acceptancePairs_;
  std::list<Pair*> livenessStack_;
  std::list<std::shared_ptr<VisitedPair>> visitedPairs_;
};

}
}

#endif
