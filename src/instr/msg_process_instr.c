/*
 * msg_process_instr.c
 *
 *  Created on: Feb 24, 2010
 *      Author: Lucas Schnorr
 *     License: This program is free software; you can redistribute
 *              it and/or modify it under the terms of the license
 *              (GNU LGPL) which comes with this package.
 *
 *     Copyright (c) 2009 The SimGrid team.
 */

#include "instr/private.h"

#ifdef HAVE_TRACING

static xbt_dict_t process_containers = NULL;

void __TRACE_msg_process_init (void)
{
  process_containers = xbt_dict_new();
}

void __TRACE_msg_process_location (m_process_t process)
{
  char name[200], alias[200];
  m_host_t host = MSG_process_get_host (process);
  TRACE_process_container (process, name, 200);
  TRACE_process_alias_container (process, host, alias, 200);

  //check if process_alias container is already created
  if (!xbt_dict_get_or_null (process_containers, alias)){
    if (IS_TRACING_PROCESSES) pajeCreateContainer (MSG_get_clock(), alias, "PROCESS", MSG_host_get_name(host), name);
    if (IS_TRACING_PROCESSES) pajePushState (MSG_get_clock(), "presence", alias, "1");
    xbt_dict_set (process_containers, xbt_strdup(alias), xbt_strdup("1"), xbt_free);
  }
}

/*
 * TRACE_msg_set_process_category: tracing interface function
 */
void TRACE_msg_set_process_category (m_process_t process, const char *category)
{
  if (!IS_TRACING) return;

  //set process category
  process->category = xbt_new (char, strlen (category)+1);
  strncpy (process->category, category, strlen(category)+1);

  //create container of type "PROCESS" to indicate location
  __TRACE_msg_process_location (process);

  //create container of type "process" to indicate behavior
  char name[200];
  TRACE_process_container (process, name, 200);
  if (IS_TRACING_PROCESSES) pajeCreateContainer (MSG_get_clock(), name, "process", category, name);
  if (IS_TRACING_PROCESSES) pajeSetState (MSG_get_clock(), "process-state", name, "executing");
}

/*
 * Instrumentation functions to trace MSG processes (m_process_t)
 */
void TRACE_msg_process_change_host (m_process_t process, m_host_t old_host, m_host_t new_host)
{
  if (!IS_TRACING_PROCESSES || !IS_TRACED(process)) return;

  char alias[200];
  TRACE_process_alias_container (process, old_host, alias, 200);
  if (IS_TRACING_PROCESSES) pajePopState (MSG_get_clock(), "presence", alias);

  __TRACE_msg_process_location (process);
  TRACE_process_alias_container (process, new_host, alias, 200);
  if (IS_TRACING_PROCESSES) pajePushState (MSG_get_clock(), "presence", alias, "1");
}

void TRACE_msg_process_kill (m_process_t process)
{
  if (!IS_TRACING_PROCESSES || !IS_TRACED(process)) return;

  char name[200];
  TRACE_process_container (process, name, 200);
  pajeDestroyContainer (MSG_get_clock(), "process", name);
}

void TRACE_msg_process_suspend (m_process_t process)
{
  if (!IS_TRACING_PROCESSES || !IS_TRACED(process)) return;

  char name[200];
  TRACE_process_container (process, name, 200);
  pajeSetState (MSG_get_clock(), "process-state", name, "suspend");
}

void TRACE_msg_process_resume (m_process_t process)
{
  if (!IS_TRACING_PROCESSES || !IS_TRACED(process)) return;

  char name[200];
  TRACE_process_container (process, name, 200);
  pajeSetState (MSG_get_clock(), "process-state", name, "executing");
}

void TRACE_msg_process_sleep_in (m_process_t process)
{
  if (!IS_TRACING_PROCESSES || !IS_TRACED(process)) return;

  char name[200];
  TRACE_process_container (process, name, 200);
  pajeSetState (MSG_get_clock(), "process-state", name, "sleep");
}

void TRACE_msg_process_sleep_out (m_process_t process)
{
  if (!IS_TRACING_PROCESSES || !IS_TRACED(process)) return;

  char name[200];
  TRACE_process_container (process, name, 200);
  pajeSetState (MSG_get_clock(), "process-state", name, "executing");
}

void TRACE_msg_process_end (m_process_t process)
{
  if (!IS_TRACED(process)) return;

  char name[200], alias[200];
  m_host_t host = MSG_process_get_host (process);
  TRACE_process_container (process, name, 200);
  TRACE_process_alias_container (process, host, alias, 200);
  if (IS_TRACING_PROCESSES) pajeDestroyContainer (MSG_get_clock(), "process", name);
  if (IS_TRACING_PROCESSES) pajeDestroyContainer (MSG_get_clock(), "PROCESS", alias);
}

#endif
