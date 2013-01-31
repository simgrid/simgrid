/* Copyright (c) 2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/instr_private.h"
#include "private.h"
#include "simdag/datatypes.h"

#ifdef HAVE_TRACING

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_sd, instr, "SD");

void TRACE_sd_set_task_category(SD_task_t task, const char *category){

  //if user provides a NULL category, task is no longer traced
  if (category == NULL) {
    xbt_free (task->category);
    task->category = NULL;
    XBT_DEBUG("SD task %p(%s), category removed", task, task->name);
    return;
  }

  //set task category
  task->category = xbt_strdup (category);
  XBT_DEBUG("SD task %p(%s), category %s", task, task->name, task->category);
}

void TRACE_sd_task_create(SD_task_t task)
{
  static long long counter = 0;
  task->counter = counter++;
  task->category = NULL;

  XBT_DEBUG("CREATE %p, %lld", task, task->counter);
}

void TRACE_sd_task_execute_start(SD_task_t task)
{
  if (task->kind == SD_TASK_COMP_PAR_AMDAHL || task->kind == SD_TASK_COMP_SEQ){
    XBT_DEBUG("EXEC,in %p, %lld, %s", task, task->counter, task->category);
//    int i;
//    for (i = 0; i < task->workstation_nb; i++){
//      container_t workstation_container =
//          PJ_container_get (SD_workstation_get_name(task->workstation_list[i]));
//      char name[1024];
//      type_t type = PJ_type_get ("power_used", workstation_container->type);
//      sprintf(name, "%s_ws_%d", SD_task_get_name(task), i);
//      val_t value = PJ_value_new(name, "1 0 1", type);
//      new_pajePushState (SD_get_clock(), workstation_container, type, value);
//    }
  }
}

void TRACE_sd_task_execute_end(SD_task_t task)
{
  if (task->kind == SD_TASK_COMP_PAR_AMDAHL || task->kind == SD_TASK_COMP_SEQ){
    XBT_DEBUG("EXEC,out %p, %lld, %s", task, task->counter, task->category);
//    int i;
//    for (i = 0; i < task->workstation_nb; i++){
//      container_t workstation_container =
//          PJ_container_get (SD_workstation_get_name(task->workstation_list[i]));
//      type_t type = PJ_type_get ("power_used", workstation_container->type);
//      new_pajePopState (SD_get_clock(), workstation_container, type);
//    }
  }
}

void TRACE_sd_task_destroy(SD_task_t task)
{
  XBT_DEBUG("DESTROY %p, %lld, %s", task, task->counter, task->category);

  //free category
  xbt_free(task->category);
  task->category = NULL;
  return;
}

#endif /* HAVE_TRACING */
