/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SAFETY_H
#define SIMGRID_MC_SAFETY_H

#include <stdint.h>

#include <simgrid_config.h>
#include <xbt/base.h>
#include <xbt/dict.h>
#include "mc_forward.hpp"
#include "mc_state.h"

SG_BEGIN_DECL()

typedef enum {
  e_mc_reduce_unset,
  e_mc_reduce_none,
  e_mc_reduce_dpor
} e_mc_reduce_t;

extern XBT_PRIVATE e_mc_reduce_t mc_reduce_kind;

int MC_modelcheck_safety(void);

typedef struct XBT_PRIVATE s_mc_visited_state{
  mc_snapshot_t system_state;
  size_t heap_bytes_used;
  int nb_processes;
  int num;
  int other_num; // dot_output for
}s_mc_visited_state_t, *mc_visited_state_t;

extern XBT_PRIVATE xbt_dynar_t visited_states;
XBT_PRIVATE mc_visited_state_t is_visited_state(mc_state_t graph_state);
XBT_PRIVATE void visited_state_free(mc_visited_state_t state);
XBT_PRIVATE void visited_state_free_voidp(void *s);

SG_END_DECL()

#endif
