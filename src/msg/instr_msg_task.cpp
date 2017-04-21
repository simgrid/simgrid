/* Copyright (c) 2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc/mc.h"
#include "src/instr/instr_private.h"
#include "src/msg/msg_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_msg, instr, "MSG instrumentation");

void TRACE_msg_set_task_category(msg_task_t task, const char *category)
{
  xbt_assert(task->category == nullptr, "Task %p(%s) already has a category (%s).",
      task, task->name, task->category);

  //if user provides a nullptr category, task is no longer traced
  if (category == nullptr) {
    xbt_free (task->category);
    task->category = nullptr;
    XBT_DEBUG("MSG task %p(%s), category removed", task, task->name);
    return;
  }

  //set task category
  task->category = xbt_strdup (category);
  XBT_DEBUG("MSG task %p(%s), category %s", task, task->name, task->category);
}

/* MSG_task_create related function*/
void TRACE_msg_task_create(msg_task_t task)
{
  static long long counter = 0;
  task->counter = counter++;
  task->category = nullptr;
  
  if(MC_is_active())
    MC_ignore_heap(&(task->counter), sizeof(task->counter));

  XBT_DEBUG("CREATE %p, %lld", task, task->counter);
}

/* MSG_task_execute related functions */
void TRACE_msg_task_execute_start(msg_task_t task)
{
  XBT_DEBUG("EXEC,in %p, %lld, %s", task, task->counter, task->category);

  if (TRACE_msg_process_is_enabled()){
    int len = INSTR_DEFAULT_STR_SIZE;
    char str[INSTR_DEFAULT_STR_SIZE];

    container_t process_container = PJ_container_get (instr_process_id(MSG_process_self(), str, len));
    type_t type = PJ_type_get ("MSG_PROCESS_STATE", process_container->type);
    val_t value = PJ_value_get ("task_execute", type);
    PushStateEvent (MSG_get_clock(), process_container, type, value);
  }
}

void TRACE_msg_task_execute_end(msg_task_t task)
{
  XBT_DEBUG("EXEC,out %p, %lld, %s", task, task->counter, task->category);

  if (TRACE_msg_process_is_enabled()){
    int len = INSTR_DEFAULT_STR_SIZE;
    char str[INSTR_DEFAULT_STR_SIZE];

    container_t process_container = PJ_container_get (instr_process_id(MSG_process_self(), str, len));
    type_t type = PJ_type_get ("MSG_PROCESS_STATE", process_container->type);
    PopStateEvent (MSG_get_clock(), process_container, type);
  }
}

/* MSG_task_destroy related functions */
void TRACE_msg_task_destroy(msg_task_t task)
{
  XBT_DEBUG("DESTROY %p, %lld, %s", task, task->counter, task->category);

  //free category
  xbt_free(task->category);
  task->category = nullptr;
}

/* MSG_task_get related functions */
void TRACE_msg_task_get_start()
{
  XBT_DEBUG("GET,in");

  if (TRACE_msg_process_is_enabled()){
    int len = INSTR_DEFAULT_STR_SIZE;
    char str[INSTR_DEFAULT_STR_SIZE];

    container_t process_container = PJ_container_get (instr_process_id(MSG_process_self(), str, len));
    type_t type = PJ_type_get ("MSG_PROCESS_STATE", process_container->type);
    val_t value = PJ_value_get ("receive", type);
    PushStateEvent (MSG_get_clock(), process_container, type, value);
  }
}

void TRACE_msg_task_get_end(double start_time, msg_task_t task)
{
  XBT_DEBUG("GET,out %p, %lld, %s", task, task->counter, task->category);

  if (TRACE_msg_process_is_enabled()){
    int len = INSTR_DEFAULT_STR_SIZE;
    char str[INSTR_DEFAULT_STR_SIZE];

    container_t process_container = PJ_container_get (instr_process_id(MSG_process_self(), str, len));
    type_t type = PJ_type_get ("MSG_PROCESS_STATE", process_container->type);
    PopStateEvent (MSG_get_clock(), process_container, type);

    char key[INSTR_DEFAULT_STR_SIZE];
    snprintf (key, INSTR_DEFAULT_STR_SIZE, "p%lld", task->counter);
    type = PJ_type_get ("MSG_PROCESS_TASK_LINK", PJ_type_get_root());
    EndLinkEvent(MSG_get_clock(), PJ_container_get_root(), type, process_container, "SR", key);
  }
}

/* MSG_task_put related functions */
int TRACE_msg_task_put_start(msg_task_t task)
{
  XBT_DEBUG("PUT,in %p, %lld, %s", task, task->counter, task->category);

  if (TRACE_msg_process_is_enabled()){
    int len = INSTR_DEFAULT_STR_SIZE;
    char str[INSTR_DEFAULT_STR_SIZE];

    container_t process_container = PJ_container_get (instr_process_id(MSG_process_self(), str, len));
    type_t type = PJ_type_get ("MSG_PROCESS_STATE", process_container->type);
    val_t value = PJ_value_get ("send", type);
    PushStateEvent (MSG_get_clock(), process_container, type, value);

    char key[INSTR_DEFAULT_STR_SIZE];
    snprintf (key, INSTR_DEFAULT_STR_SIZE, "p%lld", task->counter);
    type = PJ_type_get ("MSG_PROCESS_TASK_LINK", PJ_type_get_root());
    StartLinkEvent(MSG_get_clock(), PJ_container_get_root(), type, process_container, "SR", key);
  }

  return 1;
}

void TRACE_msg_task_put_end()
{
  XBT_DEBUG("PUT,out");

  if (TRACE_msg_process_is_enabled()){
    int len = INSTR_DEFAULT_STR_SIZE;
    char str[INSTR_DEFAULT_STR_SIZE];

    container_t process_container = PJ_container_get (instr_process_id(MSG_process_self(), str, len));
    type_t type = PJ_type_get ("MSG_PROCESS_STATE", process_container->type);
    PopStateEvent (MSG_get_clock(), process_container, type);
  }
}
