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

extern XBT_PRIVATE xbt_automaton_t _mc_property_automaton;

typedef struct XBT_PRIVATE s_mc_pair {
  int num;
  int search_cycle;
  mc_state_t graph_state; /* System state included */
  xbt_automaton_state_t automaton_state;
  xbt_dynar_t atomic_propositions;
  int requests;
  int depth;
  int exploration_started;
  int visited_pair_removed;
} s_mc_pair_t, *mc_pair_t;

typedef struct XBT_PRIVATE s_mc_visited_pair{
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
} s_mc_visited_pair_t, *mc_visited_pair_t;

XBT_PRIVATE mc_pair_t MC_pair_new(void);
XBT_PRIVATE void MC_pair_delete(mc_pair_t);
XBT_PRIVATE mc_visited_pair_t MC_visited_pair_new(int pair_num, xbt_automaton_state_t automaton_state, xbt_dynar_t atomic_propositions, mc_state_t graph_state);
XBT_PRIVATE void MC_visited_pair_delete(mc_visited_pair_t p);

int MC_modelcheck_liveness(void);
XBT_PRIVATE void MC_show_stack_liveness(xbt_fifo_t stack);
XBT_PRIVATE void MC_dump_stack_liveness(xbt_fifo_t stack);

XBT_PRIVATE extern xbt_dynar_t visited_pairs;
XBT_PRIVATE int is_visited_pair(mc_visited_pair_t visited_pair, mc_pair_t pair);

SG_END_DECL()

#endif
