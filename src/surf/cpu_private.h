/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_CPU_PRIVATE_H
#define _SURF_CPU_PRIVATE_H

#include "surf_private.h"
#include "xbt/dict.h"

typedef struct surf_action_cpu_Cas01 {
  s_surf_action_t generic_action;
  lmm_variable_t variable;
  int suspended;
} s_surf_action_cpu_Cas01_t, *surf_action_cpu_Cas01_t;

typedef struct cpu_Cas01 {
  s_surf_resource_t generic_resource;
  char *name;
  double power_scale;
  double power_current;
  tmgr_trace_event_t power_event;
  e_surf_cpu_state_t state_current;
  tmgr_trace_event_t state_event;
  lmm_constraint_t constraint;
  /*Handles the properties that can be added to cpu's */
  xbt_dict_t properties;
} s_cpu_Cas01_t, *cpu_Cas01_t;

#endif /* _SURF_CPU_PRIVATE_H */
