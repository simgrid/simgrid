/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/instr_private.h"

#ifdef HAVE_TRACING

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_category, instr, "Tracing category set/get/del of SMX processes");

static xbt_dict_t current_task_category = NULL;

void TRACE_category_alloc()
{
  current_task_category = xbt_dict_new();
}

void TRACE_category_release()
{
  xbt_dict_free(&current_task_category);
}

void TRACE_category_set(smx_process_t proc, const char *category)
{
  char processid[100];
  char *var_cpy = NULL;
  snprintf(processid, 100, "%p", proc);
  var_cpy = xbt_strdup(category);
  DEBUG2("SET process %p, category %s", proc, category);
  xbt_dict_set(current_task_category, processid, var_cpy, xbt_free);
}

char *TRACE_category_get(smx_process_t proc)
{
  char processid[100];
  snprintf(processid, 100, "%p", proc);
  char *ret = xbt_dict_get_or_null(current_task_category, processid);
  DEBUG2("GET process %p, category %s", proc, ret);
  return ret;
}

void TRACE_category_unset(smx_process_t proc)
{
  char processid[100];
  snprintf(processid, 100, "%p", proc);
  char *category = xbt_dict_get_or_null(current_task_category, processid);
  if (category != NULL) {
    DEBUG2("DEL process %p, category %s", proc, category);
    xbt_dict_remove(current_task_category, processid);
  }
}

#endif /* HAVE_TRACING */
