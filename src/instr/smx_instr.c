/*
 * smx.c
 *
 *  Created on: Nov 24, 2009
 *      Author: Lucas Schnorr
 *     License: This program is free software; you can redistribute
 *              it and/or modify it under the terms of the license
 *              (GNU LGPL) which comes with this package.
 *
 *     Copyright (c) 2009 The SimGrid team.
 */

#include "instr/private.h"
#include "instr/config.h"

#ifdef HAVE_TRACING

static long long int counter = 0; /* to uniquely identify simix actions */

void TRACE_smx_action_execute (smx_action_t act)
{
  if (!IS_TRACING) return;

  act->counter = counter++;
  char *category = __TRACE_current_category_get (SIMIX_process_self());
  if (category){
	act->category = xbt_new (char, strlen (category)+1);
	strncpy (act->category, category, strlen(category)+1);
  }
}

void TRACE_smx_action_communicate (smx_action_t act, smx_process_t proc)
{
  if (!IS_TRACING) return;

  act->counter = counter++;
  char *category = __TRACE_current_category_get (proc);
  if (category){
	act->category = xbt_new (char, strlen (category)+1);
	strncpy (act->category, category, strlen(category)+1);
  }
}

void TRACE_smx_action_destroy (smx_action_t act)
{
  if (!IS_TRACING || !IS_TRACED(act)) return;

  if (act->category){
    xbt_free (act->category);
  }
}

#endif
