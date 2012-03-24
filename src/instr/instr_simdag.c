/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/instr_private.h"

#ifdef HAVE_TRACING

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_simdag, instr, "Tracing SimDAG");

void TRACE_sd_set_task_category(SD_task_t task, const char *category)
{
  if (!TRACE_is_enabled())
    return;
  task->category = xbt_strdup (category);
}

#endif /* HAVE_TRACING */
