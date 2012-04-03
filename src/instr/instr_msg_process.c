/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/instr_private.h"

#ifdef HAVE_TRACING

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_msg_process, instr, "MSG process");

char *instr_process_id (m_process_t proc, char *str, int len)
{
  return instr_process_id_2 (proc->name, proc->pid, str, len);//MSG_process_get_name(proc), MSG_process_get_PID(proc), str, len);
}

char *instr_process_id_2 (const char *process_name, int process_pid, char *str, int len)
{
  snprintf (str, len, "%s-%d", process_name, process_pid);
  return str;
}

/*
 * Instrumentation functions to trace MSG processes (m_process_t)
 */
void TRACE_msg_process_change_host(m_process_t process, m_host_t old_host, m_host_t new_host)
{
  if (TRACE_msg_process_is_enabled()){
    static long long int counter = 0;
    char key[INSTR_DEFAULT_STR_SIZE];
    snprintf (key, INSTR_DEFAULT_STR_SIZE, "%lld", counter++);

    int len = INSTR_DEFAULT_STR_SIZE;
    char str[INSTR_DEFAULT_STR_SIZE];

    //start link
    container_t msg = PJ_container_get (instr_process_id(process, str, len));
    type_t type = PJ_type_get ("MSG_PROCESS_LINK", PJ_type_get_root());
    new_pajeStartLink (MSG_get_clock(), PJ_container_get_root(), type, msg, "M", key);

    //destroy existing container of this process
    container_t existing_container = PJ_container_get(instr_process_id(process, str, len));
    PJ_container_remove_from_parent (existing_container);
    PJ_container_free(existing_container);

    //create new container on the new_host location
    msg = PJ_container_new(instr_process_id(process, str, len), INSTR_MSG_PROCESS, PJ_container_get(new_host->name));

    //end link
    msg = PJ_container_get(instr_process_id(process, str, len));
    type = PJ_type_get ("MSG_PROCESS_LINK", PJ_type_get_root());
    new_pajeEndLink (MSG_get_clock(), PJ_container_get_root(), type, msg, "M", key);
  }
}

void TRACE_msg_process_create (const char *process_name, int process_pid, m_host_t host)
{
  if (TRACE_msg_process_is_enabled()){
    int len = INSTR_DEFAULT_STR_SIZE;
    char str[INSTR_DEFAULT_STR_SIZE];

    container_t host_container = PJ_container_get (host->name);
    PJ_container_new(instr_process_id_2(process_name, process_pid, str, len), INSTR_MSG_PROCESS, host_container);
  }
}

void TRACE_msg_process_kill(m_process_t process)
{
  if (TRACE_msg_process_is_enabled()){
    int len = INSTR_DEFAULT_STR_SIZE;
    char str[INSTR_DEFAULT_STR_SIZE];

    //kill means that this process no longer exists, let's destroy it
    PJ_container_free (PJ_container_get (instr_process_id(process, str, len)));
  }
}

void TRACE_msg_process_suspend(m_process_t process)
{
  if (TRACE_msg_process_is_enabled()){
    int len = INSTR_DEFAULT_STR_SIZE;
    char str[INSTR_DEFAULT_STR_SIZE];

    container_t process_container = PJ_container_get (instr_process_id(process, str, len));
    type_t type = PJ_type_get ("MSG_PROCESS_STATE", process_container->type);
    val_t value = PJ_value_get ("suspend", type);
    new_pajePushState (MSG_get_clock(), process_container, type, value);
  }
}

void TRACE_msg_process_resume(m_process_t process)
{
  if (TRACE_msg_process_is_enabled()){
    int len = INSTR_DEFAULT_STR_SIZE;
    char str[INSTR_DEFAULT_STR_SIZE];

    container_t process_container = PJ_container_get (instr_process_id(process, str, len));
    type_t type = PJ_type_get ("MSG_PROCESS_STATE", process_container->type);
    new_pajePopState (MSG_get_clock(), process_container, type);
  }
}

void TRACE_msg_process_sleep_in(m_process_t process)
{
  if (TRACE_msg_process_is_enabled()){
    int len = INSTR_DEFAULT_STR_SIZE;
    char str[INSTR_DEFAULT_STR_SIZE];

    container_t process_container = PJ_container_get (instr_process_id(process, str, len));
    type_t type = PJ_type_get ("MSG_PROCESS_STATE", process_container->type);
    val_t value = PJ_value_get ("sleep", type);
    new_pajePushState (MSG_get_clock(), process_container, type, value);
  }
}

void TRACE_msg_process_sleep_out(m_process_t process)
{
  if (TRACE_msg_process_is_enabled()){
    int len = INSTR_DEFAULT_STR_SIZE;
    char str[INSTR_DEFAULT_STR_SIZE];

    container_t process_container = PJ_container_get (instr_process_id(process, str, len));
    type_t type = PJ_type_get ("MSG_PROCESS_STATE", process_container->type);
    new_pajePopState (MSG_get_clock(), process_container, type);
  }
}

void TRACE_msg_process_end(m_process_t process)
{
  if (TRACE_msg_process_is_enabled()) {
    int len = INSTR_DEFAULT_STR_SIZE;
    char str[INSTR_DEFAULT_STR_SIZE];

    //that's the end, let's destroy it
    container_t container = PJ_container_get (instr_process_id(process, str, len));
    PJ_container_remove_from_parent (container);
    PJ_container_free (container);
  }
}

#endif /* HAVE_TRACING */
