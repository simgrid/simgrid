/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_LIVENESS_H
#define SIMGRID_MC_LIVENESS_H

#include <stdint.h>

#include <simgrid_config.h>
#include <xbt/base.h>
#include <xbt/fifo.h>
#include <xbt/dynar.h>
#include <xbt/automaton.h>
#include <xbt/memory.hpp>
#include "src/mc/mc_state.h"

SG_BEGIN_DECL()

SG_END_DECL()

namespace simgrid {
namespace mc {

extern XBT_PRIVATE xbt_automaton_t property_automaton;

struct XBT_PRIVATE Pair {
  int num = 0;
  int search_cycle = 0;
  mc_state_t graph_state = nullptr; /* System state included */
  xbt_automaton_state_t automaton_state = nullptr;
  simgrid::xbt::unique_ptr<s_xbt_dynar_t> atomic_propositions;
  int requests = 0;
  int depth = 0;
  int exploration_started = 0;
  int visited_pair_removed = 0;

  Pair();
  ~Pair();

  Pair(Pair const&) = delete;
  Pair& operator=(Pair const&) = delete;
};

struct XBT_PRIVATE VisitedPair {
  int num = 0;
  int other_num = 0; /* Dot output for */
  int acceptance_pair = 0;
  mc_state_t graph_state = nullptr; /* System state included */
  xbt_automaton_state_t automaton_state = nullptr;
  simgrid::xbt::unique_ptr<s_xbt_dynar_t> atomic_propositions;
  size_t heap_bytes_used = 0;
  int nb_processes = 0;
  int acceptance_removed = 0;
  int visited_removed = 0;

  VisitedPair(int pair_num, xbt_automaton_state_t automaton_state, xbt_dynar_t atomic_propositions, mc_state_t graph_state);
  ~VisitedPair();
};

int modelcheck_liveness(void);
XBT_PRIVATE void show_stack_liveness(xbt_fifo_t stack);
XBT_PRIVATE void dump_stack_liveness(xbt_fifo_t stack);

XBT_PRIVATE extern xbt_dynar_t visited_pairs;
XBT_PRIVATE int is_visited_pair(simgrid::mc::VisitedPair* visited_pair, simgrid::mc::Pair* pair);

}
}

#endif
