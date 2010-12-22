/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/instr_private.h"

#ifdef HAVE_TRACING

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_msg_process, instr, "MSG process");

/*
 * TRACE_msg_set_process_category: tracing interface function
 */
void TRACE_msg_set_process_category(m_process_t process, const char *category, const char *color)
{
  if (!(TRACE_is_enabled() &&
      TRACE_msg_process_is_enabled())) return;

  xbt_assert3(process->category == NULL,
      "Process %p(%s) already has a category (%s).",
      process, process->name, process->category);
  xbt_assert2(process->name != NULL,
      "Process %p(%s) must have a unique name in order to be traced.",
      process, process->name);
  xbt_assert3(getContainer(process->name)==NULL,
      "Process %p(%s). Tracing already knows a process with name %s."
      "The name of each process must be unique.", process, process->name, process->name);

  if (category == NULL) {
    //if user provides a NULL category, process is no longer traced
    xbt_free (process->category);
    process->category = NULL;
    return;
  }

  //set process category
  process->category = xbt_strdup(category);
  DEBUG3("MSG process %p(%s), category %s", process, process->name, process->category);

  m_host_t host = MSG_process_get_host(process);
  container_t host_container = getContainer(host->name);
  container_t msg = newContainer(process->name, INSTR_MSG_PROCESS, host_container);
  type_t type = getType (category);
  if (!type){
    type = getVariableType(category, color, msg->type);
  }
  pajeSetVariable(SIMIX_get_clock(), type->id, msg->id, "1");

  type = getType ("MSG_PROCESS_STATE");
  pajeSetState (MSG_get_clock(), type->id, msg->id, "executing");
}

/*
 * Instrumentation functions to trace MSG processes (m_process_t)
 */
void TRACE_msg_process_change_host(m_process_t process, m_host_t old_host, m_host_t new_host)
{
  if (!(TRACE_is_enabled() &&
      TRACE_msg_process_is_enabled() &&
      process->category)) return;

  static long long int counter = 0;
  char key[INSTR_DEFAULT_STR_SIZE];
  snprintf (key, INSTR_DEFAULT_STR_SIZE, "%lld", counter++);

  //start link
  container_t msg = getContainer(process->name);
  type_t type = getType ("MSG_PROCESS_LINK");
  pajeStartLink (MSG_get_clock(), type->id, "0", "M", msg->id, key);

  //destroy existing container of this process
  destroyContainer(getContainer(process->name));

  //create new container on the new_host location
  msg = newContainer(process->name, INSTR_MSG_PROCESS, getContainer(new_host->name));
  type = getType (process->category);
  pajeSetVariable(SIMIX_get_clock(), type->id, msg->id, "1");

  //end link
  msg = getContainer(process->name);
  type = getType ("MSG_PROCESS_LINK");
  pajeEndLink (MSG_get_clock(), type->id, "0", "M", msg->id, key);
}

void TRACE_msg_process_kill(m_process_t process)
{
  if (!(TRACE_is_enabled() &&
      TRACE_msg_process_is_enabled() &&
      process->category)) return;

  //kill means that this process no longer exists, let's destroy it
  destroyContainer (getContainer(process->name));
}

void TRACE_msg_process_suspend(m_process_t process)
{
  if (!(TRACE_is_enabled() &&
      TRACE_msg_process_is_enabled() &&
      process->category)) return;

  container_t process_container = getContainer (process->name);
  type_t type = getType ("MSG_PROCESS_STATE");
  pajePushState (MSG_get_clock(), type->id, process_container->id, "suspend");
}

void TRACE_msg_process_resume(m_process_t process)
{
  if (!(TRACE_is_enabled() &&
      TRACE_msg_process_is_enabled() &&
      process->category)) return;

  container_t process_container = getContainer (process->name);
  type_t type = getType ("MSG_PROCESS_STATE");
  pajePopState (MSG_get_clock(), type->id, process_container->id);
}

void TRACE_msg_process_sleep_in(m_process_t process)
{
  if (!(TRACE_is_enabled() &&
      TRACE_msg_process_is_enabled() &&
      process->category)) return;

  container_t process_container = getContainer (process->name);
  type_t type = getType ("MSG_PROCESS_STATE");
  pajePushState (MSG_get_clock(), type->id, process_container->id, "sleep");
}

void TRACE_msg_process_sleep_out(m_process_t process)
{
  if (!(TRACE_is_enabled() &&
      TRACE_msg_process_is_enabled() &&
      process->category)) return;

  container_t process_container = getContainer (process->name);
  type_t type = getType ("MSG_PROCESS_STATE");
  pajePopState (MSG_get_clock(), type->id, process_container->id);
}

void TRACE_msg_process_end(m_process_t process)
{
  if (!(TRACE_is_enabled() &&
      TRACE_msg_process_is_enabled() &&
      process->category)) return;

  //that's the end, let's destroy it
  destroyContainer (getContainer(process->name));
}

#endif /* HAVE_TRACING */
