/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/private.h"

#ifdef HAVE_TRACING

static xbt_dict_t task_containers = NULL;

void TRACE_msg_task_alloc (void)
{
  task_containers = xbt_dict_new();
}

void TRACE_msg_task_release (void)
{
  xbt_dict_free (&task_containers);
}

static void TRACE_task_location (m_task_t task)
{
	char container[200];
	char name[200], alias[200];
	char *val_one = NULL;
	m_process_t process = NULL;
	m_host_t host = NULL;
  if (!IS_TRACING_TASKS) return;
  process = MSG_process_self();
  host = MSG_process_get_host (process);

  //tasks are grouped by host
  TRACE_host_container (host, container, 200);
  TRACE_task_container (task, name, 200);
  TRACE_task_alias_container (task, process, host, alias, 200);
  //check if task container is already created
  if (!xbt_dict_get_or_null (task_containers, alias)){
    pajeCreateContainer (MSG_get_clock(), alias, "TASK", container, name);
    pajeSetState (MSG_get_clock(), "category", alias, task->category);
    val_one = xbt_strdup ("1");
    xbt_dict_set (task_containers, alias, val_one, xbt_free);
  }
}

static void TRACE_task_location_present (m_task_t task)
{
	char alias[200];
	m_process_t process = NULL;
	m_host_t host = NULL;
  if (!IS_TRACING_TASKS) return;
  //updating presence state of this task location
  process = MSG_process_self();
  host = MSG_process_get_host (process);

  TRACE_task_alias_container (task, process, host, alias, 200);
  pajePushState (MSG_get_clock(), "presence", alias, "presence");
}

static void TRACE_task_location_not_present (m_task_t task)
{
	char alias[200];
	m_process_t process = NULL;
	m_host_t host = NULL;
  if (!IS_TRACING_TASKS) return;
  //updating presence state of this task location
  process = MSG_process_self();
  host = MSG_process_get_host (process);

  TRACE_task_alias_container (task, process, host, alias, 200);
  pajePopState (MSG_get_clock(), "presence", alias);
}

/*
 * TRACE_msg_set_task_category: tracing interface function
 */
void TRACE_msg_set_task_category(m_task_t task, const char *category)
{
	char name[200];
  if (!IS_TRACING) return;

  //set task category
  task->category = xbt_new (char, strlen (category)+1);
  strncpy(task->category, category, strlen(category)+1);

  //tracing task location based on host
  TRACE_task_location (task);
  TRACE_task_location_present (task);

  TRACE_task_container (task, name, 200);
  //create container of type "task" to indicate behavior
  if (IS_TRACING_TASKS) pajeCreateContainer (MSG_get_clock(), name, "task", category, name);
  if (IS_TRACING_TASKS) pajePushState (MSG_get_clock(), "task-state", name, "created");
}

/* MSG_task_create related function*/
void TRACE_msg_task_create (m_task_t task)
{
  static long long counter = 0;
  task->counter = counter++;
  task->category = NULL;
}

/* MSG_task_execute related functions */
void TRACE_msg_task_execute_start (m_task_t task)
{
	char name[200];
  if (!IS_TRACING || !IS_TRACED(task)) return;

  TRACE_task_container (task, name, 200);
  if (IS_TRACING_TASKS) pajePushState (MSG_get_clock(), "task-state", name, "execute");

  TRACE_msg_category_set (SIMIX_process_self(), task);
}

void TRACE_msg_task_execute_end (m_task_t task)
{
	char name[200];
  if (!IS_TRACING || !IS_TRACED(task)) return;

  TRACE_task_container (task, name, 200);
  if (IS_TRACING_TASKS) pajePopState (MSG_get_clock(), "task-state", name);

  TRACE_category_unset(SIMIX_process_self());
}

/* MSG_task_destroy related functions */
void TRACE_msg_task_destroy (m_task_t task)
{
	char name[200];
  if (!IS_TRACING || !IS_TRACED(task)) return;

  TRACE_task_container (task, name, 200);
  if (IS_TRACING_TASKS) pajeDestroyContainer (MSG_get_clock(), "task", name);

  //finish the location of this task
  TRACE_task_location_not_present (task);

  //free category
  xbt_free (task->category);
  return;
}

/* MSG_task_get related functions */
void TRACE_msg_task_get_start (void)
{
  if (!IS_TRACING) return;
}

void TRACE_msg_task_get_end (double start_time, m_task_t task)
{
	char name[200];
  if (!IS_TRACING || !IS_TRACED(task)) return;

  TRACE_task_container (task, name, 200);
  if (IS_TRACING_TASKS) pajePopState (MSG_get_clock(), "task-state", name);

  TRACE_msg_volume_finish (task);

  TRACE_task_location (task);
  TRACE_task_location_present (task);
}

/* MSG_task_put related functions */
int TRACE_msg_task_put_start (m_task_t task)
{
	char name[200];
  if (!IS_TRACING || !IS_TRACED(task)) return 0;

  TRACE_task_container (task, name, 200);
  if (IS_TRACING_TASKS) pajePopState (MSG_get_clock(), "task-state", name);
  if (IS_TRACING_TASKS) pajePushState (MSG_get_clock(), "task-state", name, "communicate");

  TRACE_msg_volume_start (task);

  //trace task location grouped by host
  TRACE_task_location_not_present (task);

  //set current category
  TRACE_msg_category_set (SIMIX_process_self(), task);
  return 1;
}

void TRACE_msg_task_put_end (void)
{
  if (!IS_TRACING) return;

  TRACE_category_unset (SIMIX_process_self());
}

#endif
