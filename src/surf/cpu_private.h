/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_CPU_PRIVATE_H
#define _SURF_CPU_PRIVATE_H

#include "surf_private.h"

typedef struct surf_action_cpu {
  s_surf_action_t generic_action;
  lmm_variable_t variable;
} s_surf_action_cpu_t, *surf_action_cpu_t;

typedef struct cpu {
  surf_resource_t resource;	/* Any such object, added in a trace
				   should start by this field!!! */
  const char *name;
  xbt_maxmin_float_t power_scale;
  xbt_maxmin_float_t power_current;
  tmgr_trace_event_t power_event;
  e_surf_cpu_state_t state_current;
  tmgr_trace_event_t state_event;
  lmm_constraint_t constraint;
} s_cpu_t, *cpu_t;

#endif				/* _SURF_CPU_PRIVATE_H */
