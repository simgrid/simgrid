/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/mc_liveness.h"
#include "src/mc/mc_private.h"

extern "C" {

mc_pair_t MC_pair_new()
{
  mc_pair_t p = NULL;
  p = xbt_new0(s_mc_pair_t, 1);
  p->num = ++mc_stats->expanded_pairs;
  p->exploration_started = 0;
  p->search_cycle = 0;
  p->visited_pair_removed = _sg_mc_visited > 0 ? 0 : 1;
  return p;
}

void MC_pair_delete(mc_pair_t p)
{
  p->automaton_state = NULL;
  if(p->visited_pair_removed)
    MC_state_delete(p->graph_state, 1);
  xbt_dynar_free(&(p->atomic_propositions));
  xbt_free(p);
  p = NULL;
}

}
