
/* Copyright (c) 2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_CPU_CAS01_PRIVATE_H
#define _SURF_CPU_CAS01_PRIVATE_H

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
} s_cpu_Cas01_t, *cpu_Cas01_t;

void *cpu_cas01_create_resource(const char *name,
    double power_peak,
    double power_scale,
    tmgr_trace_t power_trace,
    int core,
    e_surf_resource_state_t state_initial,
    tmgr_trace_t state_trace,
    xbt_dict_t cpu_properties,
    surf_model_t cpu_model);

#endif                          /* _SURF_CPU_CAS01_PRIVATE_H */
