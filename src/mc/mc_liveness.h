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
  xbt_dynar_t atomic_propositions = nullptr;
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
  int num;
  int other_num; /* Dot output for */
  int acceptance_pair;
  mc_state_t graph_state; /* System state included */
  xbt_automaton_state_t automaton_state;
  xbt_dynar_t atomic_propositions;
  size_t heap_bytes_used;
  int nb_processes;
  int acceptance_removed;
  int visited_removed;
};

XBT_PRIVATE simgrid::mc::VisitedPair* visited_pair_new(int pair_num, xbt_automaton_state_t automaton_state, xbt_dynar_t atomic_propositions, mc_state_t graph_state);
XBT_PRIVATE void visited_pair_delete(simgrid::mc::VisitedPair* p);

int modelcheck_liveness(void);
XBT_PRIVATE void show_stack_liveness(xbt_fifo_t stack);
XBT_PRIVATE void dump_stack_liveness(xbt_fifo_t stack);

XBT_PRIVATE extern xbt_dynar_t visited_pairs;
XBT_PRIVATE int is_visited_pair(simgrid::mc::VisitedPair* visited_pair, simgrid::mc::Pair* pair);

}
}

#endif
