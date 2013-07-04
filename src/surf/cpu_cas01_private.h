
/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_CPU_CAS01_PRIVATE_H
#define _SURF_CPU_CAS01_PRIVATE_H

#undef GENERIC_LMM_ACTION
#undef GENERIC_ACTION
#undef ACTION_GET_CPU
#define GENERIC_LMM_ACTION(action) action->generic_lmm_action
#define GENERIC_ACTION(action) GENERIC_LMM_ACTION(action).generic_action
#define ACTION_GET_CPU(action) ((surf_action_cpu_Cas01_t) action)->cpu

typedef struct surf_action_cpu_cas01 {
  s_surf_action_lmm_t generic_lmm_action;
} s_surf_action_cpu_Cas01_t, *surf_action_cpu_Cas01_t;


/*
 * Energy-related properties for the cpu_cas01 model
 */
typedef struct energy_cpu_cas01 {
	xbt_dynar_t power_range_watts_list;		/*< List of (min_power,max_power) pairs corresponding to each cpu pstate */
	double total_energy;					/*< Total energy consumed by the host */
	double last_updated;					/*< Timestamp of the last energy update event*/
} s_energy_cpu_cas01_t, *energy_cpu_cas01_t;


typedef struct cpu_Cas01 {
  s_surf_resource_t generic_resource;
  s_xbt_swag_hookup_t modified_cpu_hookup;
  double power_peak;
  double power_scale;
  tmgr_trace_event_t power_event;
  int core;
  e_surf_resource_state_t state_current;
  tmgr_trace_event_t state_event;
  lmm_constraint_t constraint;

  xbt_dynar_t power_peak_list;				/*< List of supported CPU capacities */
  int pstate;								/*< Current pstate (index in the power_peak_list)*/
  energy_cpu_cas01_t energy;				/*< Structure with energy-consumption data */

} s_cpu_Cas01_t, *cpu_Cas01_t;

xbt_dynar_t cpu_get_watts_range_list(cpu_Cas01_t cpu_model);
void cpu_update_energy(cpu_Cas01_t cpu_model, double cpu_load);

#endif                          /* _SURF_CPU_CAS01_PRIVATE_H */
