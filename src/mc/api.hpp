/* Copyright (c) 2020-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_API_HPP
#define SIMGRID_MC_API_HPP

#include <memory>
#include <vector>

#include "simgrid/forward.h"
#include "src/mc/Session.hpp"
#include "src/mc/api/State.hpp"
#include "src/mc/mc_forward.hpp"
#include "src/mc/mc_record.hpp"
#include "xbt/automaton.hpp"
#include "xbt/base.h"

namespace simgrid {
namespace mc {

XBT_DECLARE_ENUM_CLASS(ExplorationAlgorithm, Safety, UDPOR, Liveness, CommDeterminism);

/*
** This class aimes to implement FACADE APIs for simgrid. The FACADE layer sits between the CheckerSide
** (Unfolding_Checker, DPOR, ...) layer and the
** AppSide layer. The goal is to drill down into the entagled details in the CheckerSide layer and break down the
** detailes in a way that the CheckerSide eventually
** be capable to acquire the required information through the FACADE layer rather than the direct access to the AppSide.
*/

class Api {
private:
  Api() = default;

  struct DerefAndCompareByActorsCountAndUsedHeap {
    template <class X, class Y> bool operator()(X const& a, Y const& b) const
    {
      return std::make_pair(a->actors_count, a->heap_bytes_used) < std::make_pair(b->actors_count, b->heap_bytes_used);
    }
  };

  std::unique_ptr<simgrid::mc::Session> session_;

public:
  // No copy:
  Api(Api const&) = delete;
  void operator=(Api const&) = delete;

  static Api& get()
  {
    static Api api;
    return api;
  }

  simgrid::mc::Exploration* initialize(char** argv, simgrid::mc::ExplorationAlgorithm algo);

  // ACTOR APIs
  std::vector<simgrid::mc::ActorInformation>& get_actors() const;
  unsigned long get_maxpid() const;

  // REMOTE APIs
  std::size_t get_remote_heap_bytes() const;

  // MODEL CHECKER APIs
  void mc_inc_visited_states() const;
  unsigned long mc_get_visited_states() const;
  XBT_ATTRIB_NORETURN void mc_exit(int status) const;

  // STATE APIs
  void restore_state(std::shared_ptr<simgrid::mc::Snapshot> system_state) const;

  // SNAPSHOT APIs
  bool snapshot_equal(const Snapshot* s1, const Snapshot* s2) const;
  simgrid::mc::Snapshot* take_snapshot(long num_state) const;

  // SESSION APIs
  simgrid::mc::Session const& get_session() const { return *session_; }
  void s_close();

  // AUTOMATION APIs
  void automaton_load(const char* file) const;
  std::vector<int> automaton_propositional_symbol_evaluate() const;
  std::vector<xbt_automaton_state_t> get_automaton_state() const;
  int compare_automaton_exp_label(const xbt_automaton_exp_label* l) const;
  void set_property_automaton(xbt_automaton_state_t const& automaton_state) const;
  inline DerefAndCompareByActorsCountAndUsedHeap compare_pair() const
  {
    return DerefAndCompareByActorsCountAndUsedHeap();
  }
  xbt_automaton_exp_label_t get_automaton_transition_label(xbt_dynar_t const& dynar, int index) const;
  xbt_automaton_state_t get_automaton_transition_dst(xbt_dynar_t const& dynar, int index) const;
};

} // namespace mc
} // namespace simgrid

#endif
