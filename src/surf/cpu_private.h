/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_CPU_PRIVATE_H
#define _SURF_CPU_PRIVATE_H

#include "surf/surf_private.h"

typedef struct surf_action_cpu {
  s_surf_action_t generic_action;
  lmm_variable_t variable;
} s_surf_action_cpu_t, *surf_action_cpu_t;

typedef struct cpu {
  const char *name;
  xbt_maxmin_float_t power_scale;
  xbt_maxmin_float_t current_power;
  tmgr_trace_t power_trace;
  e_surf_action_state_t current_state;
  tmgr_trace_t state_trace;
  lmm_constraint_t constraint;
} s_cpu_t, *cpu_t;


#endif				/* _SURF_SURF_PRIVATE_H */
