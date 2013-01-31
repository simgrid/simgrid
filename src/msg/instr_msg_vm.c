/* Copyright (c) 2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg_private.h"

#ifdef HAVE_TRACING

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_msg_vm, instr, "MSG VM");


char *instr_vm_id (msg_vm_t vm, char *str, int len)
{
	return instr_vm_id_2 (MSG_get_vm_name(vm), str, len);
}

char *instr_vm_id_2 (const char *vm_name, char *str, int len)
{
  snprintf (str, len, "%s", vm_name);
  return str;
}

/*
 * Instrumentation functions to trace MSG VMs (msg_vm_t)
 */
void TRACE_msg_vm_change_host(msg_vm_t vm, msg_host_t old_host, msg_host_t new_host)
{
  if (TRACE_msg_vm_is_enabled()){
    static long long int counter = 0;
    char key[INSTR_DEFAULT_STR_SIZE];
    snprintf (key, INSTR_DEFAULT_STR_SIZE, "%lld", counter++);

    int len = INSTR_DEFAULT_STR_SIZE;
    char str[INSTR_DEFAULT_STR_SIZE];

    //start link
    container_t msg = PJ_container_get (instr_vm_id(vm, str, len));
    type_t type = PJ_type_get ("MSG_VM_LINK", PJ_type_get_root());
    new_pajeStartLink (MSG_get_clock(), PJ_container_get_root(), type, msg, "M", key);

    //destroy existing container of this vm
    container_t existing_container = PJ_container_get(instr_vm_id(vm, str, len));
    PJ_container_remove_from_parent (existing_container);
    PJ_container_free(existing_container);

    //create new container on the new_host location
    msg = PJ_container_new(instr_vm_id(vm, str, len), INSTR_MSG_VM, PJ_container_get(SIMIX_host_get_name(new_host)));

    //end link
    msg = PJ_container_get(instr_vm_id(vm, str, len));
    type = PJ_type_get ("MSG_VM_LINK", PJ_type_get_root());
    new_pajeEndLink (MSG_get_clock(), PJ_container_get_root(), type, msg, "M", key);
  }
}

void TRACE_msg_vm_create (const char *vm_name, msg_host_t host)
{
  if (TRACE_msg_vm_is_enabled()){
    int len = INSTR_DEFAULT_STR_SIZE;
    char str[INSTR_DEFAULT_STR_SIZE];

    container_t host_container = PJ_container_get (SIMIX_host_get_name(host));
    PJ_container_new(instr_vm_id_2(vm_name, str, len), INSTR_MSG_VM, host_container);
  }
}

void TRACE_msg_vm_kill(msg_vm_t vm) {
  if (TRACE_msg_vm_is_enabled()) {
    int len = INSTR_DEFAULT_STR_SIZE;
    char str[INSTR_DEFAULT_STR_SIZE];

    //kill means that this vm no longer exists, let's destroy it
    container_t process = PJ_container_get (instr_vm_id(vm, str, len));
    PJ_container_remove_from_parent (process);
    PJ_container_free (process);
  }
}

void TRACE_msg_vm_suspend(msg_vm_t vm)
{
  if (TRACE_msg_vm_is_enabled()){
    int len = INSTR_DEFAULT_STR_SIZE;
    char str[INSTR_DEFAULT_STR_SIZE];

    container_t vm_container = PJ_container_get (instr_vm_id(vm, str, len));
    type_t type = PJ_type_get ("MSG_VM_STATE", vm_container->type);
    val_t value = PJ_value_get ("suspend", type);
    new_pajePushState (MSG_get_clock(), vm_container, type, value);
  }
}

void TRACE_msg_vm_resume(msg_vm_t vm)
{
  if (TRACE_msg_vm_is_enabled()){
    int len = INSTR_DEFAULT_STR_SIZE;
    char str[INSTR_DEFAULT_STR_SIZE];

    container_t vm_container = PJ_container_get (instr_vm_id(vm, str, len));
    type_t type = PJ_type_get ("MSG_VM_STATE", vm_container->type);
    new_pajePopState (MSG_get_clock(), vm_container, type);
  }
}

void TRACE_msg_vm_sleep_in(msg_vm_t vm)
{
  if (TRACE_msg_vm_is_enabled()){
    int len = INSTR_DEFAULT_STR_SIZE;
    char str[INSTR_DEFAULT_STR_SIZE];

    container_t vm_container = PJ_container_get (instr_vm_id(vm, str, len));
    type_t type = PJ_type_get ("MSG_VM_STATE", vm_container->type);
    val_t value = PJ_value_get ("sleep", type);
    new_pajePushState (MSG_get_clock(), vm_container, type, value);
  }
}

void TRACE_msg_vm_sleep_out(msg_vm_t vm)
{
  if (TRACE_msg_vm_is_enabled()){
    int len = INSTR_DEFAULT_STR_SIZE;
    char str[INSTR_DEFAULT_STR_SIZE];

    container_t vm_container = PJ_container_get (instr_vm_id(vm, str, len));
    type_t type = PJ_type_get ("MSG_VM_STATE", vm_container->type);
    new_pajePopState (MSG_get_clock(), vm_container, type);
  }
}

void TRACE_msg_vm_end(msg_vm_t vm)
{
  if (TRACE_msg_vm_is_enabled()) {
    int len = INSTR_DEFAULT_STR_SIZE;
    char str[INSTR_DEFAULT_STR_SIZE];

    //that's the end, let's destroy it
    container_t container = PJ_container_get (instr_vm_id(vm, str, len));
    PJ_container_remove_from_parent (container);
    PJ_container_free (container);
  }
}

#endif /* HAVE_TRACING */
