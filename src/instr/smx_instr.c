/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/private.h"

#ifdef HAVE_TRACING

static long long int counter = 0;       /* to uniquely identify simix actions */

void TRACE_smx_action_execute(smx_action_t act)
{
  char *category = NULL;
  if (!IS_TRACING)
    return;

  act->counter = counter++;
  category = TRACE_category_get(SIMIX_process_self());
  if (category) {
    act->category = xbt_new(char, strlen(category) + 1);
    strncpy(act->category, category, strlen(category) + 1);
  }
  TRACE_surf_resource_utilization_start(act);
}

void TRACE_smx_action_communicate(smx_action_t act, smx_process_t proc)
{
  char *category = NULL;
  if (!IS_TRACING)
    return;

  act->counter = counter++;
  category = TRACE_category_get(proc);
  if (category) {
    act->category = xbt_strdup(category);
  }
  TRACE_surf_resource_utilization_start(act);
}

void TRACE_smx_action_destroy(smx_action_t act)
{
  if (!IS_TRACING || !IS_TRACED(act))
    return;

  if (act->category) {
    xbt_free(act->category);
  }
  TRACE_surf_resource_utilization_end(act);
}

#endif
