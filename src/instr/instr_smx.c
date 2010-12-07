/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/instr_private.h"

#ifdef HAVE_TRACING

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_simix, instr, "Tracing Simix");

void TRACE_smx_host_execute(smx_action_t act)
{
  if (!TRACE_is_active()) return;
  TRACE_surf_resource_utilization_start(act);
  return;
}

void TRACE_smx_action_communicate(smx_action_t act, smx_process_t proc)
{
  if (!TRACE_is_active()) return;
  TRACE_surf_resource_utilization_start(act);
  return;
}

void TRACE_smx_action_destroy(smx_action_t act)
{
  if (!TRACE_is_active()) return;
  TRACE_surf_resource_utilization_end(act);
  return;
}

#endif /* HAVE_TRACING */
