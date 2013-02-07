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


smx_host_t SIMIX_pre_vm_create(smx_simcall_t simcall, const char *name, smx_host_t ind_phys_host){
   return SIMIX_vm_create(name, ind_phys_host);
}


/* **** start a VM **** */
static int __can_be_started(smx_host_t vm){
	// TODO add checking code related to overcommitment or not.
	return 1;
}
void SIMIX_vm_start(smx_host_t ind_vm){

  //TODO only start the VM if you can
  if (__can_be_started(ind_vm))
	  SIMIX_vm_set_state(ind_vm, msg_vm_state_running);
  else
	  THROWF(vm_error, 0, "The VM %s cannot be started", SIMIX_host_get_name(ind_vm));
}

void SIMIX_pre_vm_start(smx_simcall_t simcall, smx_host_t ind_vm){
   SIMIX_vm_start(ind_vm);
}

/* ***** set/get state of a VM ***** */
void SIMIX_vm_set_state(smx_host_t ind_vm, int state){
	surf_vm_workstation_model->extension.vm_workstation.set_state(ind_vm, state);
}
void SIMIX_pre_vm_set_state(smx_host_t ind_vm, int state){
	SIMIX_vm_set_state(ind_vm, state);
}

int SIMIX_vm_get_state(smx_host_t ind_vm){
 return surf_vm_workstation_model->extension.vm_workstation.get_state(ind_vm);
}
int SIMIX_pre_vm_get_state(smx_host_t ind_vm){
	return SIMIX_vm_get_state(ind_vm);
}


/**
 * \brief Function to migrate a SIMIX VM host. 
 *
 * \param host the vm host to migrate (a smx_host_t)
 */
void SIMIX_vm_migrate(smx_host_t ind_vm, smx_host_t ind_dst_pm)
{
  /* TODO: check state */

  /* TODO: Using the variable of the MSG layer is not clean. */
  SIMIX_vm_set_state(ind_vm, msg_vm_state_migrating);

  /* jump to vm_ws_migrate(). this will update the vm location. */
  surf_vm_workstation_model->extension.vm_workstation.migrate(ind_vm, ind_dst_pm);

  SIMIX_vm_set_state(ind_vm, msg_vm_state_running);
}

void SIMIX_pre_vm_migrate(smx_simcall_t simcall, smx_host_t ind_vm, smx_host_t ind_dst_pm){
   SIMIX_vm_migrate(ind_vm, ind_dst_pm);
}


/**
 * \brief Function to get the physical host of the given the SIMIX VM host.
 *
 * \param host the vm host to get_phys_host (a smx_host_t)
 */
const char *SIMIX_vm_get_phys_host(smx_host_t ind_vm)
{
  /* jump to vm_ws_get_phys_host(). this will return the vm name. */
  return surf_vm_workstation_model->extension.vm_workstation.get_phys_host(ind_vm);
}

const char *SIMIX_pre_vm_get_phys_host(smx_simcall_t simcall, smx_host_t ind_vm){
  return SIMIX_vm_get_phys_host(ind_vm);
}


/**
 * \brief Function to suspend a SIMIX VM host. This function stops the exection of the
 * VM. All the processes on this VM will pause. The state of the VM is
 * preserved on memory. We can later resume it again.
 *
 * \param host the vm host to suspend (a smx_host_t)
 */
void SIMIX_vm_suspend(smx_host_t ind_vm)
{
  /* TODO: check state */

  XBT_DEBUG("%lu processes in the VM", xbt_swag_size(SIMIX_host_priv(ind_vm)->process_list));

  smx_process_t smx_process, smx_process_safe;
  xbt_swag_foreach_safe(smx_process, smx_process_safe, SIMIX_host_priv(ind_vm)->process_list) {
         XBT_DEBUG("suspend %s", SIMIX_host_get_name(ind_vm));
	 /* FIXME: calling a simcall from the SIMIX layer is strange. */
         simcall_process_suspend(smx_process);
  }

  /* TODO: Using the variable of the MSG layer is not clean. */
  SIMIX_vm_set_state(ind_vm, msg_vm_state_suspended);
}

void SIMIX_pre_vm_suspend(smx_simcall_t simcall, smx_host_t ind_vm){
   SIMIX_vm_suspend(ind_vm);
}


/**
 * \brief Function to resume a SIMIX VM host. This function restart the execution of the
 * VM. All the processes on this VM will run again. 
 *
 * \param host the vm host to resume (a smx_host_t)
 */
void SIMIX_vm_resume(smx_host_t ind_vm)
{
  /* TODO: check state */

  XBT_DEBUG("%lu processes in the VM", xbt_swag_size(SIMIX_host_priv(ind_vm)->process_list));

  smx_process_t smx_process, smx_process_safe;
  xbt_swag_foreach_safe(smx_process, smx_process_safe, SIMIX_host_priv(ind_vm)->process_list) {
         XBT_DEBUG("resume %s", SIMIX_host_get_name(ind_vm));
	 /* FIXME: calling a simcall from the SIMIX layer is strange. */
         simcall_process_resume(smx_process);
  }

  /* TODO: Using the variable of the MSG layer is not clean. */
  SIMIX_vm_set_state(ind_vm, msg_vm_state_running);
}

void SIMIX_pre_vm_resume(smx_simcall_t simcall, smx_host_t ind_vm){
   SIMIX_vm_resume(ind_vm);
}


/**
 * \brief Function to save a SIMIX VM host.
 * This function is the same as vm_suspend, but the state of the VM is saved to the disk, and not preserved on memory.
 * We can later restore it again.
 *
 * \param host the vm host to save (a smx_host_t)
 */
void SIMIX_vm_save(smx_host_t ind_vm)
{
  /* TODO: check state */

  XBT_DEBUG("%lu processes in the VM", xbt_swag_size(SIMIX_host_priv(ind_vm)->process_list));

  /* TODO: do something at the surf level */

  smx_process_t smx_process, smx_process_safe;
  xbt_swag_foreach_safe(smx_process, smx_process_safe, SIMIX_host_priv(ind_vm)->process_list) {
         XBT_DEBUG("save %s", SIMIX_host_get_name(ind_vm));
	 /* FIXME: calling a simcall from the SIMIX layer is strange. */
         simcall_process_save(smx_process);
  }

  /* TODO: Using the variable of the MSG layer is not clean. */
  SIMIX_vm_set_state(ind_vm, msg_vm_state_saved);
}

void SIMIX_pre_vm_save(smx_simcall_t simcall, smx_host_t ind_vm){
   SIMIX_vm_save(ind_vm);
}


/**
 * \brief Function to restore a SIMIX VM host. This function restart the execution of the
 * VM. All the processes on this VM will run again. 
 *
 * \param host the vm host to restore (a smx_host_t)
 */
void SIMIX_vm_restore(smx_host_t ind_vm)
{
  /* TODO: check state */

  XBT_DEBUG("%lu processes in the VM", xbt_swag_size(SIMIX_host_priv(ind_vm)->process_list));

  /* TODO: do something at the surf level */

  smx_process_t smx_process, smx_process_safe;
  xbt_swag_foreach_safe(smx_process, smx_process_safe, SIMIX_host_priv(ind_vm)->process_list) {
         XBT_DEBUG("restore %s", SIMIX_host_get_name(ind_vm));
	 /* FIXME: calling a simcall from the SIMIX layer is strange. */
         simcall_process_restore(smx_process);
  }

  /* TODO: Using the variable of the MSG layer is not clean. */
  SIMIX_vm_set_state(ind_vm, msg_vm_state_running);
}

void SIMIX_pre_vm_restore(smx_simcall_t simcall, smx_host_t ind_vm){
   SIMIX_vm_restore(ind_vm);
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
  /* TODO: check state */

  XBT_DEBUG("%lu processes in the VM", xbt_swag_size(SIMIX_host_priv(ind_vm)->process_list));

  smx_process_t smx_process, smx_process_safe;
  xbt_swag_foreach_safe(smx_process, smx_process_safe, SIMIX_host_priv(ind_vm)->process_list) {
         XBT_DEBUG("kill %s", SIMIX_host_get_name(ind_vm));

	 SIMIX_process_kill(smx_process, issuer);
  }

  /* TODO: Using the variable of the MSG layer is not clean. */
  SIMIX_vm_set_state(ind_vm, msg_vm_state_sleeping);
}

void SIMIX_pre_vm_shutdown(smx_simcall_t simcall, smx_host_t ind_vm){
   SIMIX_vm_shutdown(ind_vm, simcall->issuer);
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
  xbt_lib_unset(host_lib, hostname, SIMIX_HOST_LEVEL);

  /* jump to vm_ws_destroy(). The surf level resource will be freed. */
  surf_vm_workstation_model->extension.vm_workstation.destroy(ind_vm);
}

void SIMIX_pre_vm_destroy(smx_simcall_t simcall, smx_host_t ind_vm){
   SIMIX_vm_destroy(ind_vm);
}
