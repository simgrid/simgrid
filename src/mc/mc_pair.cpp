/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/dynar.h>
#include <xbt/sysdep.h>

#include "src/mc/mc_liveness.h"
#include "src/mc/mc_private.h"

namespace simgrid {
namespace mc {

simgrid::mc::Pair* pair_new()
{
  simgrid::mc::Pair* p = nullptr;
  p = xbt_new0(simgrid::mc::Pair, 1);
  p->num = ++mc_stats->expanded_pairs;
  p->exploration_started = 0;
  p->search_cycle = 0;
  p->visited_pair_removed = _sg_mc_visited > 0 ? 0 : 1;
  return p;
}

void pair_delete(simgrid::mc::Pair* p)
{
  p->automaton_state = nullptr;
  if(p->visited_pair_removed)
    MC_state_delete(p->graph_state, 1);
  xbt_dynar_free(&(p->atomic_propositions));
  xbt_free(p);
  p = nullptr;
}

}
}