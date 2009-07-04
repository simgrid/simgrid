
/* Copyright (c) 2009 The SimGrid Team. All rights reserved.                */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_RESOURCE_LMM_H
#define SURF_RESOURCE_LMM_H
#include "surf/surf.h"
#include "surf/trace_mgr.h"

static XBT_INLINE
surf_resource_lmm_t surf_resource_lmm_new(size_t childsize,
    /* for superclass */ surf_model_t model, char *name, xbt_dict_t props,
    lmm_system_t system, double constraint_value,
    tmgr_history_t history,
    int state_init, tmgr_trace_t state_trace,
    double metric_init, tmgr_trace_t metric_trace) {

  surf_resource_lmm_t res = (surf_resource_lmm_t)surf_resource_new(childsize,model,name,props);

  res->constraint = lmm_constraint_new(system, res, constraint_value);
  res->state_current = state_init;
  if (state_trace)
    res->state_event = tmgr_history_add_trace(history, state_trace, 0.0, 0, res);

  res->power.current = metric_init;
  if (metric_trace)
    res->power.event = tmgr_history_add_trace(history, metric_trace, 0.0, 0, res);
  return res;
}


static XBT_INLINE e_surf_resource_state_t surf_resource_lmm_get_state(void *r) {
  surf_resource_lmm_t resource = (surf_resource_lmm_t)r;
  return (resource)->state_current;
}
#endif /* SURF_RESOURCE_LMM_H */
