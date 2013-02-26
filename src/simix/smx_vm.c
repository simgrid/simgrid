/* Copyright (c) 2007-2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smx_private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dict.h"
#include "mc/mc.h"

//If you need to log some stuffs, just uncomment these two lines and uses XBT_DEBUG for instance
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_vm, simix, "Logging specific to SIMIX (vms)");

/* **** create a VM **** */

/**
 * \brief Internal function to create a SIMIX host.
 * \param name name of the host to create
 * \param data some user data (may be NULL)
 */
smx_host_t SIMIX_vm_create(const char *name, smx_host_t ind_phys_host)
{

  smx_host_priv_t smx_host = xbt_new0(s_smx_host_priv_t, 1);
  s_smx_process_t proc;

  // TODO check why we do not have any VM here and why we have the host_proc_hookup  ?

  /* Host structure */
  smx_host->data = NULL;
  smx_host->process_list =
      xbt_swag_new(xbt_swag_offset(proc, host_proc_hookup));

  /* Update global variables */
  xbt_lib_set(host_lib,name,SIMIX_HOST_LEVEL,smx_host);

  /* Create surf associated resource */
  // TODO change phys_host into the right workstation surf model
  surf_vm_workstation_model->extension.vm_workstation.create(name, ind_phys_host);

  return xbt_lib_get_elm_or_null(host_lib, name);
}


smx_host_t SIMIX_pre_vm_create(smx_simcall_t simcall, const char *name, smx_host_t ind_phys_host)
{
  return SIMIX_vm_create(name, ind_phys_host);
}


static int get_host_property_as_integer(smx_host_t host, const char *name)
{
  xbt_dict_t dict = SIMIX_host_get_properties(host);

  char *value = xbt_dict_get_or_null(dict, name);
  return atoi(value);
}



/* **** start a VM **** */
static int __can_be_started(smx_host_t vm)
{
	// TODO add checking code related to overcommitment or not.

#if 0
  int overcommit = get_host_property_as_integer(vm, "OverCommit");
  int core_nb = get_host_property_as_integer(vm, "CORE_NB");
  int mem_cap = get_host_property_as_integer(vm, "MEM_CAP");
  int net_cap = get_host_property_as_integer(vm, "NET_CAP");
#endif

  /* we need to get other VM objects on this physical host. */



	return 1;
}

void SIMIX_vm_start(smx_host_t ind_vm)
{
  //TODO only start the VM if you can
  if (__can_be_started(ind_vm))
    SIMIX_vm_set_state(ind_vm, SURF_VM_STATE_RUNNING);
  else
    THROWF(vm_error, 0, "The VM %s cannot be started", SIMIX_host_get_name(ind_vm));
}

void SIMIX_pre_vm_start(smx_simcall_t simcall, smx_host_t ind_vm)
{
  SIMIX_vm_start(ind_vm);
}

/* ***** set/get state of a VM ***** */
void SIMIX_vm_set_state(smx_host_t ind_vm, int state)
{
  surf_vm_workstation_model->extension.vm_workstation.set_state(ind_vm, state);
}

void SIMIX_pre_vm_set_state(smx_simcall_t simcall, smx_host_t ind_vm, int state)
{
  SIMIX_vm_set_state(ind_vm, state);
}

int SIMIX_vm_get_state(smx_host_t ind_vm)
{
  return surf_vm_workstation_model->extension.vm_workstation.get_state(ind_vm);
}

int SIMIX_pre_vm_get_state(smx_simcall_t simcall, smx_host_t ind_vm)
{
  return SIMIX_vm_get_state(ind_vm);
}


/**
 * \brief Function to migrate a SIMIX VM host. 
 *
 * \param host the vm host to migrate (a smx_host_t)
 */
void SIMIX_vm_migrate(smx_host_t ind_vm, smx_host_t ind_dst_pm)
{
  const char *name = SIMIX_host_get_name(ind_vm);

  if (SIMIX_vm_get_state(ind_vm) != SURF_VM_STATE_RUNNING)
    THROWF(vm_error, 0, "VM(%s) is not running", name);

  /* jump to vm_ws_migrate(). this will update the vm location. */
  surf_vm_workstation_model->extension.vm_workstation.migrate(ind_vm, ind_dst_pm);
}

void SIMIX_pre_vm_migrate(smx_simcall_t simcall, smx_host_t ind_vm, smx_host_t ind_dst_pm)
{
  SIMIX_vm_migrate(ind_vm, ind_dst_pm);
}


/**
 * \brief Function to get the physical host of the given the SIMIX VM host.
 *
 * \param host the vm host to get_phys_host (a smx_host_t)
 */
void *SIMIX_vm_get_pm(smx_host_t ind_vm)
{
  /* jump to vm_ws_get_pm(). this will return the vm name. */
  return surf_vm_workstation_model->extension.vm_workstation.get_pm(ind_vm);
}

void *SIMIX_pre_vm_get_pm(smx_simcall_t simcall, smx_host_t ind_vm)
{
  return SIMIX_vm_get_pm(ind_vm);
}


/**
 * \brief Function to suspend a SIMIX VM host. This function stops the exection of the
 * VM. All the processes on this VM will pause. The state of the VM is
 * preserved on memory. We can later resume it again.
 *
 * \param host the vm host to suspend (a smx_host_t)
 */
void SIMIX_vm_suspend(smx_host_t ind_vm, smx_process_t issuer)
{
  const char *name = SIMIX_host_get_name(ind_vm);

  if (SIMIX_vm_get_state(ind_vm) != SURF_VM_STATE_RUNNING)
    THROWF(vm_error, 0, "VM(%s) is not running", name);

  XBT_DEBUG("suspend VM(%s), where %d processes exist", name, xbt_swag_size(SIMIX_host_priv(ind_vm)->process_list));

  /* jump to vm_ws_suspend. The state will be set. */
  surf_vm_workstation_model->extension.vm_workstation.suspend(ind_vm);

  smx_process_t smx_process, smx_process_safe;
  xbt_swag_foreach_safe(smx_process, smx_process_safe, SIMIX_host_priv(ind_vm)->process_list) {
    XBT_DEBUG("suspend %s", smx_process->name);
    SIMIX_process_suspend(smx_process, issuer);
  }

  XBT_DEBUG("suspend all processes on the VM done done");
}

void SIMIX_pre_vm_suspend(smx_simcall_t simcall, smx_host_t ind_vm)
{
  if (simcall->issuer->smx_host == ind_vm) {
    XBT_ERROR("cannot suspend the VM where I run");
    DIE_IMPOSSIBLE;
  }

  SIMIX_vm_suspend(ind_vm, simcall->issuer);

  /* without this, simcall_vm_suspend() does not return to the userland. why? */
  SIMIX_simcall_answer(simcall);

  XBT_DEBUG("SIMIX_pre_vm_suspend done");
}


/**
 * \brief Function to resume a SIMIX VM host. This function restart the execution of the
 * VM. All the processes on this VM will run again. 
 *
 * \param host the vm host to resume (a smx_host_t)
 */
void SIMIX_vm_resume(smx_host_t ind_vm, smx_process_t issuer)
{
  const char *name = SIMIX_host_get_name(ind_vm);

  if (SIMIX_vm_get_state(ind_vm) != SURF_VM_STATE_SUSPENDED)
    THROWF(vm_error, 0, "VM(%s) was not suspended", name);

  XBT_DEBUG("resume VM(%s), where %d processes exist", name, xbt_swag_size(SIMIX_host_priv(ind_vm)->process_list));

  /* jump to vm_ws_resume() */
  surf_vm_workstation_model->extension.vm_workstation.resume(ind_vm);

  smx_process_t smx_process, smx_process_safe;
  xbt_swag_foreach_safe(smx_process, smx_process_safe, SIMIX_host_priv(ind_vm)->process_list) {
    XBT_DEBUG("resume %s", smx_process->name);
    SIMIX_process_resume(smx_process, issuer);
  }
}

void SIMIX_pre_vm_resume(smx_simcall_t simcall, smx_host_t ind_vm)
{
  SIMIX_vm_resume(ind_vm, simcall->issuer);
  SIMIX_simcall_answer(simcall);
}


/**
 * \brief Function to save a SIMIX VM host.
 * This function is the same as vm_suspend, but the state of the VM is saved to the disk, and not preserved on memory.
 * We can later restore it again.
 *
 * \param host the vm host to save (a smx_host_t)
 */
void SIMIX_vm_save(smx_host_t ind_vm, smx_process_t issuer)
{
  const char *name = SIMIX_host_get_name(ind_vm);

  if (SIMIX_vm_get_state(ind_vm) != SURF_VM_STATE_RUNNING)
    THROWF(vm_error, 0, "VM(%s) is not running", name);


  XBT_DEBUG("save VM(%s), where %d processes exist", name, xbt_swag_size(SIMIX_host_priv(ind_vm)->process_list));

  /* jump to vm_ws_save() */
  surf_vm_workstation_model->extension.vm_workstation.save(ind_vm);

  smx_process_t smx_process, smx_process_safe;
  xbt_swag_foreach_safe(smx_process, smx_process_safe, SIMIX_host_priv(ind_vm)->process_list) {
    XBT_DEBUG("suspend %s", smx_process->name);
    SIMIX_process_suspend(smx_process, issuer);
  }
}

void SIMIX_pre_vm_save(smx_simcall_t simcall, smx_host_t ind_vm)
{
  SIMIX_vm_save(ind_vm, simcall->issuer);

  /* without this, simcall_vm_suspend() does not return to the userland. why? */
  SIMIX_simcall_answer(simcall);
}


/**
 * \brief Function to restore a SIMIX VM host. This function restart the execution of the
 * VM. All the processes on this VM will run again. 
 *
 * \param host the vm host to restore (a smx_host_t)
 */
void SIMIX_vm_restore(smx_host_t ind_vm, smx_process_t issuer)
{
  const char *name = SIMIX_host_get_name(ind_vm);

  if (SIMIX_vm_get_state(ind_vm) != SURF_VM_STATE_SAVED)
    THROWF(vm_error, 0, "VM(%s) was not saved", name);

  XBT_DEBUG("restore VM(%s), where %d processes exist", name, xbt_swag_size(SIMIX_host_priv(ind_vm)->process_list));

  /* jump to vm_ws_restore() */
  surf_vm_workstation_model->extension.vm_workstation.resume(ind_vm);

  smx_process_t smx_process, smx_process_safe;
  xbt_swag_foreach_safe(smx_process, smx_process_safe, SIMIX_host_priv(ind_vm)->process_list) {
    XBT_DEBUG("resume %s", smx_process->name);
    SIMIX_process_resume(smx_process, issuer);
  }
}

void SIMIX_pre_vm_restore(smx_simcall_t simcall, smx_host_t ind_vm)
{
  SIMIX_vm_restore(ind_vm, simcall->issuer);
  SIMIX_simcall_answer(simcall);
}


/**
 * \brief Function to shutdown a SIMIX VM host. This function powers off the
 * VM. All the processes on this VM will be killed. But, the state of the VM is
 * preserved on memory. We can later start it again.
 *
 * \param host the vm host to shutdown (a smx_host_t)
 */
void SIMIX_vm_shutdown(smx_host_t ind_vm, smx_process_t issuer)
{
  const char *name = SIMIX_host_get_name(ind_vm);

  if (SIMIX_vm_get_state(ind_vm) != SURF_VM_STATE_RUNNING)
    THROWF(vm_error, 0, "VM(%s) is not running", name);

  XBT_DEBUG("%d processes in the VM", xbt_swag_size(SIMIX_host_priv(ind_vm)->process_list));

  smx_process_t smx_process, smx_process_safe;
  xbt_swag_foreach_safe(smx_process, smx_process_safe, SIMIX_host_priv(ind_vm)->process_list) {
    XBT_DEBUG("shutdown %s", name);
    SIMIX_process_kill(smx_process, issuer);
  }

  /* FIXME: we may have to do something at the surf layer, e.g., vcpu action */
  SIMIX_vm_set_state(ind_vm, SURF_VM_STATE_CREATED);
}

void SIMIX_pre_vm_shutdown(smx_simcall_t simcall, smx_host_t ind_vm)
{
  SIMIX_vm_shutdown(ind_vm, simcall->issuer);
  SIMIX_simcall_answer(simcall);
}


/**
 * \brief Function to destroy a SIMIX VM host.
 *
 * \param host the vm host to destroy (a smx_host_t)
 */
void SIMIX_vm_destroy(smx_host_t ind_vm)
{
  /* this code basically performs a similar thing like SIMIX_host_destroy() */

  xbt_assert((ind_vm != NULL), "Invalid parameters");
  const char *hostname = SIMIX_host_get_name(ind_vm);

  smx_host_priv_t host_priv = SIMIX_host_priv(ind_vm);

  /* this will call the registered callback function, i.e., SIMIX_host_destroy().  */
  xbt_lib_unset(host_lib, hostname, SIMIX_HOST_LEVEL, 1);

  /* jump to vm_ws_destroy(). The surf level resource will be freed. */
  surf_vm_workstation_model->extension.vm_workstation.destroy(ind_vm);
}

void SIMIX_pre_vm_destroy(smx_simcall_t simcall, smx_host_t ind_vm){
   SIMIX_vm_destroy(ind_vm);
}
