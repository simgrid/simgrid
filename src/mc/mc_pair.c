/* Copyright (c) 2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc_private.h"

mc_pair_t MC_pair_new(){
  mc_pair_t p = NULL;
  p = xbt_new0(s_mc_pair_t, 1);
  p->num = ++mc_stats->expanded_pairs;
  p->search_cycle = 0;
  return p;
}

mc_visited_pair_t MC_visited_pair_new(int pair_num, xbt_automaton_state_t automaton_state, xbt_dynar_t atomic_propositions){
  mc_visited_pair_t pair = NULL;
  pair = xbt_new0(s_mc_visited_pair_t, 1);
  pair->graph_state = MC_state_new();
  pair->graph_state->system_state = MC_take_snapshot(pair_num);
  pair->heap_bytes_used = mmalloc_get_bytes_used(std_heap);
  pair->nb_processes = xbt_swag_size(simix_global->process_list);
  pair->automaton_state = automaton_state;
  pair->num = pair_num;
  pair->other_num = -1;
  pair->acceptance_removed = 0;
  pair->visited_removed = 0;
  pair->acceptance_pair = 0;
  pair->atomic_propositions = xbt_dynar_new(sizeof(int), NULL);
  unsigned int cursor = 0;
  int value;
  xbt_dynar_foreach(atomic_propositions, cursor, value)
    xbt_dynar_push_as(pair->atomic_propositions, int, value);
  return pair;
}

void MC_visited_pair_delete(mc_visited_pair_t p){
  p->automaton_state = NULL;
  MC_state_delete(p->graph_state);
  xbt_dynar_free(&(p->atomic_propositions));
  xbt_free(p);
  p = NULL;
}

void MC_pair_delete(mc_pair_t p){
  p->automaton_state = NULL;
  MC_state_delete(p->graph_state);
  xbt_dynar_free(&(p->atomic_propositions));
  xbt_free(p);
  p = NULL;
}

void mc_pair_free_voidp(void *p){
  MC_pair_delete((mc_pair_t) * (void **)p);
}
