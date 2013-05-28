/* Copyright (c) 2013 Da SimGrid Team. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc_private.h"

mc_pair_t MC_pair_new(){
  mc_pair_t p = NULL;
  p = xbt_new0(s_mc_pair_t, 1);
  p->heap_bytes_used = mmalloc_get_bytes_used(std_heap);
  p->nb_processes = xbt_swag_size(simix_global->process_list);
  p->num = ++mc_stats->expanded_pairs;
  return p;
}

void MC_pair_delete(mc_pair_t p){
  p->automaton_state = NULL;
  MC_state_delete(p->graph_state);
  xbt_dynar_free(&(p->atomic_propositions));
  xbt_free(p);
}
