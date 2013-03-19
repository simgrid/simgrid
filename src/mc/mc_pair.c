/* Copyright (c) 2013 Da SimGrid Team. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc_private.h"

mc_pair_t MC_pair_new(mc_state_t gs, xbt_automaton_state_t as, int r){
  mc_pair_t p = NULL;
  p = xbt_new0(s_mc_pair_t, 1);
  p->automaton_state = as;
  p->graph_state = gs;
  p->system_state = NULL;
  p->requests = r;
  return p;
}

void MC_pair_delete(mc_pair_t p){
  p->automaton_state = NULL;
  if(p->system_state)
    MC_free_snapshot(p->system_state);
  MC_state_delete(p->graph_state);
  xbt_free(p);
}
