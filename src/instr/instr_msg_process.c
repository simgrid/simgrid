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
  xbt_die("deprecated");
}

char *instr_process_id (m_process_t proc, char *str, int len)
{
  return instr_process_id_2 (MSG_process_get_name(proc), MSG_process_get_PID(proc), str, len);
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
    container_t msg = getContainer(instr_process_id(process, str, len));
    type_t type = getType ("MSG_PROCESS_LINK", getRootType());
    new_pajeStartLink (MSG_get_clock(), getRootContainer(), type, msg, "M", key);

    //destroy existing container of this process
    destroyContainer(getContainer(instr_process_id(process, str, len)));

    //create new container on the new_host location
    msg = newContainer(instr_process_id(process, str, len), INSTR_MSG_PROCESS, getContainer(new_host->name));

    //set the state of this new container
    type = getType ("MSG_PROCESS_STATE", msg->type);
    val_t value = getValueByName ("executing", type);
    new_pajeSetState (MSG_get_clock(), msg, type, value);

    //end link
    msg = getContainer(instr_process_id(process, str, len));
    type = getType ("MSG_PROCESS_LINK", getRootType());
    new_pajeEndLink (MSG_get_clock(), getRootContainer(), type, msg, "M", key);
  }
}

void TRACE_msg_process_create (const char *process_name, int process_pid, m_host_t host)
{
  if (TRACE_msg_process_is_enabled()){
    int len = INSTR_DEFAULT_STR_SIZE;
    char str[INSTR_DEFAULT_STR_SIZE];

    container_t host_container = getContainer(host->name);
    container_t msg = newContainer(instr_process_id_2(process_name, process_pid, str, len), INSTR_MSG_PROCESS, host_container);

    type_t type = getType ("MSG_PROCESS_STATE", msg->type);
    val_t value = getValueByName ("executing", type);
    new_pajeSetState (MSG_get_clock(), msg, type, value);
  }
}

void TRACE_msg_process_kill(m_process_t process)
{
  if (TRACE_msg_process_is_enabled()){
    int len = INSTR_DEFAULT_STR_SIZE;
    char str[INSTR_DEFAULT_STR_SIZE];

    //kill means that this process no longer exists, let's destroy it
    destroyContainer (getContainer(instr_process_id(process, str, len)));
  }
}

void TRACE_msg_process_suspend(m_process_t process)
{
  if (TRACE_msg_process_is_enabled()){
    int len = INSTR_DEFAULT_STR_SIZE;
    char str[INSTR_DEFAULT_STR_SIZE];

    container_t process_container = getContainer (instr_process_id(process, str, len));
    type_t type = getType ("MSG_PROCESS_STATE", process_container->type);
    val_t value = getValueByName ("suspend", type);
    new_pajePushState (MSG_get_clock(), process_container, type, value);
  }
}

void TRACE_msg_process_resume(m_process_t process)
{
  if (TRACE_msg_process_is_enabled()){
    int len = INSTR_DEFAULT_STR_SIZE;
    char str[INSTR_DEFAULT_STR_SIZE];

    container_t process_container = getContainer (instr_process_id(process, str, len));
    type_t type = getType ("MSG_PROCESS_STATE", process_container->type);
    new_pajePopState (MSG_get_clock(), process_container, type);
  }
}

void TRACE_msg_process_sleep_in(m_process_t process)
{
  if (TRACE_msg_process_is_enabled()){
    int len = INSTR_DEFAULT_STR_SIZE;
    char str[INSTR_DEFAULT_STR_SIZE];

    container_t process_container = getContainer (instr_process_id(process, str, len));
    type_t type = getType ("MSG_PROCESS_STATE", process_container->type);
    val_t value = getValueByName ("sleep", type);
    new_pajePushState (MSG_get_clock(), process_container, type, value);
  }
}

void TRACE_msg_process_sleep_out(m_process_t process)
{
  if (TRACE_msg_process_is_enabled()){
    int len = INSTR_DEFAULT_STR_SIZE;
    char str[INSTR_DEFAULT_STR_SIZE];

    container_t process_container = getContainer (instr_process_id(process, str, len));
    type_t type = getType ("MSG_PROCESS_STATE", process_container->type);
    new_pajePopState (MSG_get_clock(), process_container, type);
  }
}

void TRACE_msg_process_end(m_process_t process)
{
  if (TRACE_msg_process_is_enabled()) {
    int len = INSTR_DEFAULT_STR_SIZE;
    char str[INSTR_DEFAULT_STR_SIZE];

    //that's the end, let's destroy it
    container_t container = getContainer(instr_process_id(process, str, len));
    removeContainerFromParent (container);
    destroyContainer (container);
  }
}

#endif /* HAVE_TRACING */
