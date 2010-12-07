/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/instr_private.h"

#ifdef HAVE_TRACING

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_msg, instr, "MSG");

static xbt_dict_t task_containers = NULL;

static char *TRACE_task_alias_container(m_task_t task, m_process_t process,
                                 m_host_t host, char *output, int len)
{
  if (output) {
    snprintf(output, len, "%p-%lld-%p-%p", task, task->counter, process,
             host);
    return output;
  } else {
    return NULL;
  }
}

static char *TRACE_host_container(m_host_t host, char *output, int len)
{
  if (output) {
    snprintf(output, len, "%s", MSG_host_get_name(host));
    return output;
  } else {
    return NULL;
  }
}

char *TRACE_task_container(m_task_t task, char *output, int len)
{
  if (output) {
    snprintf(output, len, "%p-%lld", task, task->counter);
    return output;
  } else {
    return NULL;
  }
}

void TRACE_msg_task_alloc(void)
{
  task_containers = xbt_dict_new();
}

void TRACE_msg_task_release(void)
{
  xbt_dict_free(&task_containers);
}

static void TRACE_task_location(m_task_t task)
{
  char container[200];
  char name[200], alias[200];
  char *val_one = NULL;
  m_process_t process = NULL;
  m_host_t host = NULL;
  if (!TRACE_msg_task_is_enabled())
    return;
  process = MSG_process_self();
  host = MSG_process_get_host(process);

  //tasks are grouped by host
  TRACE_host_container(host, container, 200);
  TRACE_task_container(task, name, 200);
  TRACE_task_alias_container(task, process, host, alias, 200);
  //check if task container is already created
  if (!xbt_dict_get_or_null(task_containers, alias)) {
    pajeCreateContainer(MSG_get_clock(), alias, "TASK", container, name);
    pajeSetState(MSG_get_clock(), "category", alias, task->category);
    val_one = xbt_strdup("1");
    xbt_dict_set(task_containers, alias, val_one, xbt_free);
  }
}

static void TRACE_task_location_present(m_task_t task)
{
  char alias[200];
  m_process_t process = NULL;
  m_host_t host = NULL;
  if (!TRACE_msg_task_is_enabled())
    return;
  //updating presence state of this task location
  process = MSG_process_self();
  host = MSG_process_get_host(process);

  TRACE_task_alias_container(task, process, host, alias, 200);
  pajePushState(MSG_get_clock(), "presence", alias, "presence");
}

static void TRACE_task_location_not_present(m_task_t task)
{
  char alias[200];
  m_process_t process = NULL;
  m_host_t host = NULL;
  if (!TRACE_msg_task_is_enabled())
    return;
  //updating presence state of this task location
  process = MSG_process_self();
  host = MSG_process_get_host(process);

  TRACE_task_alias_container(task, process, host, alias, 200);
  pajePopState(MSG_get_clock(), "presence", alias);
}

/*
 * TRACE_msg_set_task_category: tracing interface function
 */
void TRACE_msg_set_task_category(m_task_t task, const char *category)
{
  char name[200];
  if (!TRACE_is_active())
    return;

  xbt_assert3(task->category == NULL, "Task %p(%s) already has a category (%s).",
      task, task->name, task->category);

  //set task category
  task->category = xbt_strdup (category);
  DEBUG3("MSG task %p(%s), category %s", task, task->name, task->category);

  //tracing task location based on host
  TRACE_task_location(task);
  TRACE_task_location_present(task);

  TRACE_task_container(task, name, 200);
  //create container of type "task" to indicate behavior
  if (TRACE_msg_task_is_enabled())
    pajeCreateContainer(MSG_get_clock(), name, "task", category, name);
  if (TRACE_msg_task_is_enabled())
    pajePushState(MSG_get_clock(), "task-state", name, "created");
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
  char name[200];
  if (!TRACE_is_active())
    return;

  if (!task->category)
    return;

  DEBUG3("EXEC,in %p, %lld, %s", task, task->counter, task->category);

  TRACE_task_container(task, name, 200);
  if (TRACE_msg_task_is_enabled())
    pajePushState(MSG_get_clock(), "task-state", name, "execute");
}

void TRACE_msg_task_execute_end(m_task_t task)
{
  char name[200];
  if (!TRACE_is_active())
    return;

  if (!task->category)
    return;

  TRACE_task_container(task, name, 200);
  if (TRACE_msg_task_is_enabled())
    pajePopState(MSG_get_clock(), "task-state", name);

  DEBUG3("EXEC,out %p, %lld, %s", task, task->counter, task->category);
}

/* MSG_task_destroy related functions */
void TRACE_msg_task_destroy(m_task_t task)
{
  char name[200];
  if (!TRACE_is_active())
    return;

  if (!task->category)
    return;

  TRACE_task_container(task, name, 200);
  if (TRACE_msg_task_is_enabled())
    pajeDestroyContainer(MSG_get_clock(), "task", name);

  //finish the location of this task
  TRACE_task_location_not_present(task);

  DEBUG3("DESTROY %p, %lld, %s", task, task->counter, task->category);

  //free category
  xbt_free(task->category);
  return;
}

/* MSG_task_get related functions */
void TRACE_msg_task_get_start(void)
{
  if (!TRACE_is_active())
    return;

  DEBUG0("GET,in");
}

void TRACE_msg_task_get_end(double start_time, m_task_t task)
{
  char name[200];
  if (!TRACE_is_active())
    return;

  if (!task->category)
    return;

  TRACE_task_container(task, name, 200);
  if (TRACE_msg_task_is_enabled())
    pajePopState(MSG_get_clock(), "task-state", name);

  TRACE_msg_volume_finish(task);

  TRACE_task_location(task);
  TRACE_task_location_present(task);

  DEBUG3("GET,out %p, %lld, %s", task, task->counter, task->category);
}

/* MSG_task_put related functions */
int TRACE_msg_task_put_start(m_task_t task)
{
  char name[200];
  if (!TRACE_is_active())
    return 0;

  if (!task->category)
    return 0;

  DEBUG3("PUT,in %p, %lld, %s", task, task->counter, task->category);

  TRACE_task_container(task, name, 200);
  if (TRACE_msg_task_is_enabled())
    pajePopState(MSG_get_clock(), "task-state", name);
  if (TRACE_msg_task_is_enabled())
    pajePushState(MSG_get_clock(), "task-state", name, "communicate");

  TRACE_msg_volume_start(task);

  //trace task location grouped by host
  TRACE_task_location_not_present(task);
  return 1;
}

void TRACE_msg_task_put_end(void)
{
  if (!TRACE_is_active())
    return;
  DEBUG0("PUT,in");
}

#endif /* HAVE_TRACING */
