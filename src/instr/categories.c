/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/private.h"

#ifdef HAVE_TRACING

static xbt_dict_t current_task_category = NULL;

void TRACE_category_alloc ()
{
  current_task_category = xbt_dict_new();
}

void TRACE_category_release ()
{
  xbt_dict_free (&current_task_category);
}

void TRACE_category_set (smx_process_t proc, const char *category)
{
  char processid[100];
  char *var_cpy = NULL;
  snprintf (processid, 100, "%p", proc);
  var_cpy = xbt_strdup (category);
  xbt_dict_set (current_task_category, processid, var_cpy, xbt_free);
}

char *TRACE_category_get (smx_process_t proc)
{
  char processid[100];
  snprintf (processid, 100, "%p", proc);
  return xbt_dict_get_or_null (current_task_category, processid);
}

void TRACE_category_unset (smx_process_t proc)
{
  char processid[100];
  snprintf (processid, 100, "%p", proc);
  if (xbt_dict_get_or_null (current_task_category, processid) != NULL){
    xbt_dict_remove (current_task_category, processid);
  }
}

void TRACE_msg_category_set (smx_process_t proc, m_task_t task)
{
  TRACE_category_set (proc, task->category);
}


#endif
