/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/private.h"

#ifdef HAVE_TRACING

void TRACE_sd_task_create(SD_task_t task)
{
  if (!IS_TRACING)
    return;
  task->category = NULL;
}

void TRACE_sd_task_destroy(SD_task_t task)
{
  if (!IS_TRACING)
    return;
  if (task->category)
    xbt_free(task->category);
}

void TRACE_sd_set_task_category(SD_task_t task, const char *category)
{
  if (!IS_TRACING)
    return;
  task->category = xbt_new(char, strlen(category) + 1);
  strncpy(task->category, category, strlen(category) + 1);
}

#endif
