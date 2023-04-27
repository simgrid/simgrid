/* Copyright (c) 2007-2023. The SimGrid Team.
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

namespace simgrid::mc {

class XBT_PRIVATE Pair {
public:
  int num                           = 0;
  bool search_cycle                 = false;
  std::shared_ptr<State> app_state_ = nullptr; /* State of the application (including system state) */
  xbt_automaton_state_t prop_state_ = nullptr; /* State of the property automaton */
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
  std::shared_ptr<State> app_state_  = nullptr; /* State of the application (including system state) */
  xbt_automaton_state_t prop_state_;            /* State of the property automaton */
  std::shared_ptr<const std::vector<int>> atomic_propositions;
  std::size_t heap_bytes_used = 0;
  int actor_count_;

  VisitedPair(int pair_num, xbt_automaton_state_t prop_state,
              std::shared_ptr<const std::vector<int>> atomic_propositions, std::shared_ptr<State> app_state,
              RemoteApp& remote_app);
};

class XBT_PRIVATE LivenessChecker : public Exploration {
public:
  explicit LivenessChecker(const std::vector<char*>& args);
  ~LivenessChecker() override;

  void run() override;
  RecordTrace get_record_trace() override;
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

  /* The property automaton must be a static because it's sometimes used before the explorer is even created.
   *
   * This can happen if some symbols are created during the application's initialization process, before the first
   * decision point for the model-checker. Since the first snapshot is taken at the first decision point and since the
   * explorer is created after the first snapshot, this may result in some symbols being registered even before the
   * model-checker notices that this is a LivenessChecker to create.
   *
   * This situation is unfortunate, but I guess that it's the best I can achieve given the state of our initialization
   * code.
   */
  static xbt_automaton_t property_automaton_;
  bool evaluate_label(const xbt_automaton_exp_label* l, std::vector<int> const& values);

public:
  void automaton_load(const char* file) const;
  std::vector<int> automaton_propositional_symbol_evaluate() const;
  std::vector<xbt_automaton_state_t> get_automaton_state() const;
  int compare_automaton_exp_label(const xbt_automaton_exp_label* l) const;
  void set_property_automaton(xbt_automaton_state_t const& automaton_state) const;
  xbt_automaton_exp_label_t get_automaton_transition_label(xbt_dynar_t const& dynar, int index) const;
  xbt_automaton_state_t get_automaton_transition_dst(xbt_dynar_t const& dynar, int index) const;
  static void automaton_register_symbol(RemoteProcessMemory const& remote_process, const char* name,
                                        RemotePtr<int> addr);
};

} // namespace simgrid::mc

#endif
