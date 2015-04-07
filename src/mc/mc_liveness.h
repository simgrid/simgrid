/* Copyright (c) 2007-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_LIVENESS_H
#define MC_LIVENESS_H

#include <stdint.h>

#include <simgrid_config.h>
#include <xbt/fifo.h>
#include <xbt/dynar.h>
#include <xbt/automaton.h>
#include "mc_state.h"

SG_BEGIN_DECL()

extern xbt_automaton_t _mc_property_automaton;

typedef struct s_mc_pair{
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

typedef struct s_mc_visited_pair{
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

mc_pair_t MC_pair_new(void);
void MC_pair_delete(mc_pair_t);
void mc_pair_free_voidp(void *p);
mc_visited_pair_t MC_visited_pair_new(int pair_num, xbt_automaton_state_t automaton_state, xbt_dynar_t atomic_propositions, mc_state_t graph_state);
void MC_visited_pair_delete(mc_visited_pair_t p);

void MC_modelcheck_liveness(void);
void MC_show_stack_liveness(xbt_fifo_t stack);
void MC_dump_stack_liveness(xbt_fifo_t stack);

extern xbt_dynar_t visited_pairs;
int is_visited_pair(mc_visited_pair_t visited_pair, mc_pair_t pair);

SG_END_DECL()

#endif
