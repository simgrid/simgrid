/* Copyright (c) 2007-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_SAFETY_H
#define MC_SAFETY_H

#include <stdint.h>

#include <simgrid_config.h>
#include <xbt/dict.h>
#include "mc_interface.h"

SG_BEGIN_DECL()

typedef enum {
  e_mc_reduce_unset,
  e_mc_reduce_none,
  e_mc_reduce_dpor
} e_mc_reduce_t;

extern e_mc_reduce_t mc_reduce_kind;
extern xbt_dict_t first_enabled_state;

void MC_pre_modelcheck_safety(void);
void MC_modelcheck_safety(void);

typedef struct s_mc_visited_state{
  mc_snapshot_t system_state;
  size_t heap_bytes_used;
  int nb_processes;
  int num;
  int other_num; // dot_output for
}s_mc_visited_state_t, *mc_visited_state_t;

extern xbt_dynar_t visited_states;
mc_visited_state_t is_visited_state(void);
void visited_state_free(mc_visited_state_t state);
void visited_state_free_voidp(void *s);

SG_END_DECL()

#endif
