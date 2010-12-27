/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/instr_private.h"

#ifdef HAVE_TRACING

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_msg, instr, "MSG");

xbt_dict_t tasks_created = NULL;

/*
 * TRACE_msg_set_task_category: tracing interface function
 */
void TRACE_msg_set_task_category(m_task_t task, const char *category)
{
  if (!tasks_created) tasks_created = xbt_dict_new();
  if (!TRACE_is_active())
    return;

  xbt_assert3(task->category == NULL, "Task %p(%s) already has a category (%s).",
      task, task->name, task->category);
  if (TRACE_msg_task_is_enabled()){
    xbt_assert2(task->name != NULL,
        "Task %p(%s) must have a unique name in order to be traced, if --cfg=tracing/msg/task:1 is used.",
        task, task->name);
    xbt_assert3(getContainer(task->name)==NULL,
        "Task %p(%s). Tracing already knows a task with name %s."
        "The name of each task must be unique, if --cfg=tracing/msg/task:1 is used.", task, task->name, task->name);
  }

  if (category == NULL) {
    //if user provides a NULL category, task is no longer traced
    xbt_free (task->category);
    task->category = NULL;
    return;
  }

  //set task category
  task->category = xbt_strdup (category);
  DEBUG3("MSG task %p(%s), category %s", task, task->name, task->category);

  if (TRACE_msg_task_is_enabled()){
    m_host_t host = MSG_host_self();
    container_t host_container = getContainer(host->name);
    //check to see if there is a container with the task->name
    container_t msg = getContainer(task->name);
    xbt_assert3(xbt_dict_get_or_null (tasks_created, task->name) == NULL,
        "Task %p(%s). Tracing already knows a task with name %s."
        "The name of each task must be unique, if --cfg=tracing/msg/task:1 is used.", task, task->name, task->name);
    msg = newContainer(task->name, INSTR_MSG_TASK, host_container);
    type_t type = getType (task->category);
    if (!type){
      type = getVariableType(task->category, NULL, msg->type);
    }
    new_pajeSetVariable (SIMIX_get_clock(), msg, type, 1);

    type = getType ("MSG_TASK_STATE");
    new_pajePushState (MSG_get_clock(), msg, type, "created");

    xbt_dict_set (tasks_created, task->name, xbt_strdup("1"), xbt_free);
  }
}

/* MSG_task_create related function*/
void TRACE_msg_task_create(m_task_t task)
{
  static long long counter = 0;
  task->counter = counter++;
  task->category = NULL;
  DEBUG2("CREATE %p, %lld", task, task->counter);
}

/* MSG_task_execute related functions */
void TRACE_msg_task_execute_start(m_task_t task)
{
  if (!(TRACE_is_enabled() &&
      TRACE_msg_task_is_enabled() &&
      task->category)) return;

  DEBUG3("EXEC,in %p, %lld, %s", task, task->counter, task->category);

  container_t task_container = getContainer (task->name);
  type_t type = getType ("MSG_TASK_STATE");
  new_pajePushState (MSG_get_clock(), task_container, type, "MSG_task_execute");
}

void TRACE_msg_task_execute_end(m_task_t task)
{
  if (!(TRACE_is_enabled() &&
      TRACE_msg_task_is_enabled() &&
      task->category)) return;

  DEBUG3("EXEC,out %p, %lld, %s", task, task->counter, task->category);

  container_t task_container = getContainer (task->name);
  type_t type = getType ("MSG_TASK_STATE");
  new_pajePopState (MSG_get_clock(), task_container, type);
}

/* MSG_task_destroy related functions */
void TRACE_msg_task_destroy(m_task_t task)
{
  if (!tasks_created) tasks_created = xbt_dict_new();
  if (!(TRACE_is_enabled() &&
      TRACE_msg_task_is_enabled() &&
      task->category)) return;

  //that's the end, let's destroy it
  destroyContainer (getContainer(task->name));

  DEBUG3("DESTROY %p, %lld, %s", task, task->counter, task->category);

  //free category
  xbt_free(task->category);
  task->category = NULL;

  xbt_dict_remove (tasks_created, task->name);
  return;
}

/* MSG_task_get related functions */
void TRACE_msg_task_get_start(void)
{
  if (!(TRACE_is_enabled() &&
      TRACE_msg_task_is_enabled())) return;

  DEBUG0("GET,in");
}

void TRACE_msg_task_get_end(double start_time, m_task_t task)
{
  if (!(TRACE_is_enabled() &&
      TRACE_msg_task_is_enabled() &&
      task->category)) return;

  DEBUG3("GET,out %p, %lld, %s", task, task->counter, task->category);

  //FIXME
  //if (TRACE_msg_volume_is_enabled()){
  //  TRACE_msg_volume_end(task);
  //}

  m_host_t host = MSG_host_self();
  container_t host_container = getContainer(host->name);
  container_t msg = newContainer(task->name, INSTR_MSG_TASK, host_container);
  type_t type = getType (task->category);
  new_pajeSetVariable (SIMIX_get_clock(), msg, type, 1);

  type = getType ("MSG_TASK_STATE");
  new_pajePushState (MSG_get_clock(), msg, type, "created");

  type = getType ("MSG_TASK_LINK");
  char key[INSTR_DEFAULT_STR_SIZE];
  snprintf (key, INSTR_DEFAULT_STR_SIZE, "%lld", task->counter);
  new_pajeEndLink (MSG_get_clock(), getRootContainer(), type, msg, "SR", key);
}

/* MSG_task_put related functions */
int TRACE_msg_task_put_start(m_task_t task)
{
  if (!(TRACE_is_enabled() &&
      TRACE_msg_task_is_enabled() &&
      task->category)) return 0;

  DEBUG3("PUT,in %p, %lld, %s", task, task->counter, task->category);

  container_t msg = getContainer (task->name);
  type_t type = getType ("MSG_TASK_STATE");
  new_pajePopState (MSG_get_clock(), msg, type);

  type = getType ("MSG_TASK_LINK");
  char key[INSTR_DEFAULT_STR_SIZE];
  snprintf (key, INSTR_DEFAULT_STR_SIZE, "%lld", task->counter);
  new_pajeStartLink(MSG_get_clock(), getRootContainer(), type, msg, "SR", key);

  destroyContainer (msg);

  //FIXME
  //if (TRACE_msg_volume_is_enabled()){
  //  TRACE_msg_volume_start(task);
  //}

  return 1;
}

void TRACE_msg_task_put_end(void)
{
  if (!(TRACE_is_enabled() &&
      TRACE_msg_task_is_enabled())) return;

  DEBUG0("PUT,out");
}

#endif /* HAVE_TRACING */
