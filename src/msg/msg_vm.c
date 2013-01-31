/* Copyright (c) 2012. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg_private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_vm, msg,
                                "Cloud-oriented parts of the MSG API");

/** @brief Create a new VM (the VM is just attached to the location but it is not started yet).
 *  @ingroup msg_VMs
 */

msg_vm_t MSG_vm_create(msg_host_t location, const char *name,
	                                     int core_nb, int mem_cap, int net_cap){

  // Note new and vm_workstation refer to the same area (due to the lib/dict appraoch)
  msg_vm_t new = NULL;
  void *vm_workstation =  NULL;
  // Ask simix to create the surf vm resource
  vm_workstation = simcall_vm_create(name,location);
  new = (msg_vm_t) __MSG_host_create(vm_workstation);


  MSG_vm_set_property_value(new, "CORE_NB", bprintf("%d", core_nb), free);
  MSG_vm_set_property_value(new, "MEM_CAP", bprintf("%d", core_nb), free);
  MSG_vm_set_property_value(new, "NET_CAP", bprintf("%d", core_nb), free);

  #ifdef HAVE_TRACING
  TRACE_msg_vm_create(name, location);
  #endif
  return new;
}

/** \ingroup m_host_management
 * \brief Returns the value of a given host property
 *
 * \param host a host
 * \param name a property name
 * \return value of a property (or NULL if property not set)
 */
const char *MSG_host_get_property_value(msg_host_t host, const char *name)
{
  return xbt_dict_get_or_null(MSG_host_get_properties(host), name);
}

/** \ingroup m_host_management
 * \brief Returns a xbt_dict_t consisting of the list of properties assigned to this host
 *
 * \param host a host
 * \return a dict containing the properties
 */
xbt_dict_t MSG_host_get_properties(msg_host_t host)
{
  xbt_assert((host != NULL), "Invalid parameters (host is NULL)");

  return (simcall_host_get_properties(host));
}

/** \ingroup m_host_management
 * \brief Change the value of a given host property
 *
 * \param host a host
 * \param name a property name
 * \param value what to change the property to
 * \param free_ctn the freeing function to use to kill the value on need
 */
void MSG_vm_set_property_value(msg_vm_t vm, const char *name, void *value,void_f_pvoid_t free_ctn) {

  xbt_dict_set(MSG_host_get_properties(vm), name, value,free_ctn);
}

/** @brief Immediately suspend the execution of all processes within the given VM.
 *  @ingroup msg_VMs
 *  return wheter the VM has been correctly started (0) or not (<0)
 *
 */
int MSG_vm_start(msg_vm_t vm) {
 // TODO Please complete the code

  #ifdef HAVE_TRACING
  TRACE_msg_vm_start(vm);
  #endif
}

/** @brief Returns a newly constructed dynar containing all existing VMs in the system.
 *  @ingroup msg_VMs
 *
 * Don't forget to free the dynar after use.
 */
xbt_dynar_t MSG_vms_as_dynar(void) {
  xbt_dynar_t res = xbt_dynar_new(sizeof(msg_vm_t),NULL);
  msg_vm_t vm;
  xbt_swag_foreach(vm,msg_global->vms) {
    xbt_dynar_push(res,&vm);
  }
  return res;
}

/** @brief Returns whether the given VM is currently suspended
 *  @ingroup msg_VMs
 */
int MSG_vm_is_suspended(msg_vm_t vm) {
  return vm->state == msg_vm_state_suspended;
}
/** @brief Returns whether the given VM is currently running
 *  @ingroup msg_VMs
 */
int MSG_vm_is_running(msg_vm_t vm) {
  return vm->state == msg_vm_state_running;
}
/** @brief Add the given process into the VM.
 *  @ingroup msg_VMs
 *
 * Afterward, when the VM is migrated or suspended or whatever, the process will have the corresponding handling, too.
 *
 */
void MSG_vm_bind(msg_vm_t vm, msg_process_t process) {
  /* check if the process is already in a VM */
  simdata_process_t simdata = simcall_process_get_data(process);
  if (simdata->vm) {
    msg_vm_t old_vm = simdata->vm;
    int pos = xbt_dynar_search(old_vm->processes,&process);
    xbt_dynar_remove_at(old_vm->processes,pos, NULL);
  }
  /* check if the host is in the right host */
  if (simdata->m_host != vm->location) {
    MSG_process_migrate(process,vm->location);
  }
  simdata->vm = vm;

  XBT_DEBUG("binding Process %s to %p",MSG_process_get_name(process),vm);

  xbt_dynar_push_as(vm->processes,msg_process_t,process);
}
/** @brief Removes the given process from the given VM, and kill it
 *  @ingroup msg_VMs
 *
 *  Will raise a not_found exception if the process were not binded to that VM
 */
void MSG_vm_unbind(msg_vm_t vm, msg_process_t process) {
  int pos = xbt_dynar_search(vm->processes,process);
  xbt_dynar_remove_at(vm->processes,pos, NULL);
  MSG_process_kill(process);
}

/** @brief Immediately change the host on which all processes are running.
 *  @ingroup msg_VMs
 *
 * No migration cost occurs. If you want to simulate this too, you want to use a
 * MSG_task_send() before or after, depending on whether you want to do cold or hot
 * migration.
 */
void MSG_vm_migrate(msg_vm_t vm, msg_host_t destination) {
  unsigned int cpt;
  msg_process_t process;
  xbt_dynar_foreach(vm->processes,cpt,process) {
    MSG_process_migrate(process,destination);
  }
  xbt_swag_remove(vm, MSG_host_priv(vm->location)->vms);
  xbt_swag_insert_at_tail(vm, MSG_host_priv(destination)->vms);
  
  #ifdef HAVE_TRACING
  TRACE_msg_vm_change_host(vm,vm->location,destination);
  #endif

  vm->location = destination;
}

/** @brief Immediately suspend the execution of all processes within the given VM.
 *  @ingroup msg_VMs
 *
 * No suspension cost occurs. If you want to simulate this too, you want to
 * use a \ref MSG_file_write() before or after, depending on the exact semantic
 * of VM suspend to you.
 */
void MSG_vm_suspend(msg_vm_t vm) {
  unsigned int cpt;
  msg_process_t process;
  xbt_dynar_foreach(vm->processes,cpt,process) {
    XBT_DEBUG("suspend process %s of host %s",MSG_process_get_name(process),MSG_host_get_name(MSG_process_get_host(process)));
    MSG_process_suspend(process);
  }

  #ifdef HAVE_TRACING
  TRACE_msg_vm_suspend(vm);
  #endif
}


/** @brief Immediately resumes the execution of all processes within the given VM.
 *  @ingroup msg_VMs
 *
 * No resume cost occurs. If you want to simulate this too, you want to
 * use a \ref MSG_file_read() before or after, depending on the exact semantic
 * of VM resume to you.
 */
void MSG_vm_resume(msg_vm_t vm) {
  unsigned int cpt;
  msg_process_t process;
  xbt_dynar_foreach(vm->processes,cpt,process) {
    XBT_DEBUG("resume process %s of host %s",MSG_process_get_name(process),MSG_host_get_name(MSG_process_get_host(process)));
    MSG_process_resume(process);
  }

  #ifdef HAVE_TRACING
  TRACE_msg_vm_resume(vm);
  #endif
}


void MSG_vm_shutdown(msg_vm_t vm)
{
  /* msg_vm_t equals to msg_host_t */
  char *name = simcall_host_get_name(vm);
  smx_host_t smx_host = xbt_lib_get_or_null(host_lib, name, SIMIX_HOST_LEVEL);
  smx_process_t smx_process, smx_process_next;

  XBT_DEBUG("%lu processes in the VM", xbt_swag_size(SIMIX_host_priv(smx_host)->process_list));

  xbt_swag_foreach_safe(smx_process, SIMIX_host_priv(smx_host)->process_list) {
	  XBT_DEBUG("kill %s", SIMIX_host_get_name(smx_host));
	  simcall_process_kill(smx_process);
  }

  /* taka: not yet confirmed */
  #ifdef HAVE_TRACING
  TRACE_msg_vm_kill(vm);
  #endif


  /* TODO: update the state of vm */

#if 0
  while (xbt_dynar_length(vm->processes) > 0) {
    process = xbt_dynar_get_as(vm->processes,0,msg_process_t);
  }
#endif
}


/**
 * \ingroup msg_VMs
 * \brief Reboot the VM, restarting all the processes in it.
 */
void MSG_vm_reboot(msg_vm_t vm)
{
  xbt_dynar_t new_processes = xbt_dynar_new(sizeof(msg_process_t),NULL);

  msg_process_t process;
  unsigned int cpt;

  xbt_dynar_foreach(vm->processes,cpt,process) {
    msg_process_t new_process = MSG_process_restart(process);
    xbt_dynar_push_as(new_processes,msg_process_t,new_process);

  }

  xbt_dynar_foreach(new_processes, cpt, process) {
    MSG_vm_bind(vm,process);
  }

  xbt_dynar_free(&new_processes);
}

/** @brief Destroy a msg_vm_t.
 *  @ingroup msg_VMs
 */
void MSG_vm_destroy(msg_vm_t vm) {
  unsigned int cpt;
  msg_process_t process;
  xbt_dynar_foreach(vm->processes,cpt,process) {
    //FIXME: Slow ?
    simdata_process_t simdata = simcall_process_get_data(process);
    simdata->vm = NULL;
  }

  #ifdef HAVE_TRACING
  TRACE_msg_vm_end(vm);
  #endif


  xbt_dynar_free(&vm->processes);
  xbt_free(vm);
}
