/* Copyright (c) 2020-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_API_HPP
#define SIMGRID_MC_API_HPP

#include <memory>
#include <vector>

#include "simgrid/forward.h"
#include "src/mc/api/State.hpp"
#include "src/mc/mc_forward.hpp"
#include "src/mc/mc_record.hpp"
#include "xbt/automaton.hpp"
#include "xbt/base.h"

namespace simgrid {
namespace mc {

XBT_DECLARE_ENUM_CLASS(CheckerAlgorithm, Safety, UDPOR, Liveness, CommDeterminism);

/**
 * @brief Maintains the transition's information.
 */
struct s_transition_detail {
  simgrid::simix::Simcall call_ = simgrid::simix::Simcall::NONE;
  long issuer_id                = -1;
  RemotePtr<kernel::activity::MailboxImpl> mbox_remote_addr {}; // used to represent mailbox remote address for isend and ireceive transitions
  RemotePtr<kernel::activity::ActivityImpl> comm_remote_addr {}; // the communication this transition concerns (to be used only for isend, ireceive, wait and test)
};

using transition_detail_t = std::unique_ptr<s_transition_detail>;

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

public:
  // No copy:
  Api(Api const&) = delete;
  void operator=(Api const&) = delete;

  static Api& get()
  {
    static Api api;
    return api;
  }

  simgrid::mc::Exploration* initialize(char** argv, simgrid::mc::CheckerAlgorithm algo) const;

  // ACTOR APIs
  std::vector<simgrid::mc::ActorInformation>& get_actors() const;
  unsigned long get_maxpid() const;

  // COMMUNICATION APIs
  xbt::string const& get_actor_name(smx_actor_t actor) const;
  xbt::string const& get_actor_host_name(smx_actor_t actor) const;

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
  void s_close() const;

// AUTOMATION APIs
#if SIMGRID_HAVE_MC
  void automaton_load(const char* file) const;
#endif
  std::vector<int> automaton_propositional_symbol_evaluate() const;
  std::vector<xbt_automaton_state_t> get_automaton_state() const;
  int compare_automaton_exp_label(const xbt_automaton_exp_label* l) const;
  void set_property_automaton(xbt_automaton_state_t const& automaton_state) const;
  inline DerefAndCompareByActorsCountAndUsedHeap compare_pair() const
  {
    return DerefAndCompareByActorsCountAndUsedHeap();
  }
  inline int automaton_state_compare(const_xbt_automaton_state_t const& s1, const_xbt_automaton_state_t const& s2) const
  {
    return xbt_automaton_state_compare(s1, s2);
  }
  xbt_automaton_exp_label_t get_automaton_transition_label(xbt_dynar_t const& dynar, int index) const;
  xbt_automaton_state_t get_automaton_transition_dst(xbt_dynar_t const& dynar, int index) const;
};

} // namespace mc
} // namespace simgrid

#endif
