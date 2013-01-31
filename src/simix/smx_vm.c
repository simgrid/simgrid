/* Copyright (c) 2007-2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smx_private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dict.h"
#include "mc/mc.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_vm, simix,
                                "Logging specific to SIMIX (vms)");
/* **** create a VM **** */

/**
 * \brief Internal function to create a SIMIX host.
 * \param name name of the host to create
 * \param data some user data (may be NULL)
 */
smx_host_t SIMIX_vm_create(const char *name, smx_host_t phys_host)
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
  surf_vm_workstation_model->extension.vm_workstation.create(name, phys_host);

  return xbt_lib_get_elm_or_null(host_lib, name);
}


smx_host_t SIMIX_pre_vm_create(smx_simcall_t simcall, const char *name, smx_host_t phys_host){
   return SIMIX_vm_create(name, phys_host);
}


/* **** start a VM **** */
int __can_be_started(smx_host_t vm){
	// TODO add checking code related to overcommitment or not.
	return 1;
}
void SIMIX_vm_start(smx_host_t vm){

  //TODO only start the VM if you can
  if (can_be_started(vm))
	  SIMIX_set_vm_state(vm, msg_vm_state_running);
  else
	  THROWF(vm_error, 0, "The VM %s cannot be started", SIMIX_host_get_name(vm));
}

void SIMIX_pre_vm_start(smx_simcall_t simcall, smx_host_t vm){
   SIMIX_vm_start(vm);
}

/* ***** set/get state of a VM ***** */
void SIMIX_set_vm_state(smx_host_t vm, int state){
	surf_vm_workstation_model->extension.vm_workstation.set_state(vm, state);
}
void SIMIX_prev_set_vm_state(smx_host_t vm, int state){
	SIMIX_set_vm_state(vm, state);
}

int SIMIX_get_vm_state(smx_host_t vm){
 return surf_vm_workstation_model->extension.vm_workstation.get_state(vm);
}
int SIMIX_pre_vm_state(smx_host_t vm){
	return SIMIX_get_vm_state(vm);
}

/**
 * \brief Function to destroy a SIMIX VM host.
 *
 * \param host the vm host to destroy (a smx_host_t)
 */
void SIMIX_vm_destroy(smx_host_t host)
{
  /* this code basically performs a similar thing like SIMIX_host_destroy() */

  xbt_assert((host != NULL), "Invalid parameters");
  char *hostname = host->key;

  smx_host_priv_t host_priv = SIMIX_host_priv(host);

  /* this will call the registered callback function, i.e., SIMIX_host_destroy().  */
  xbt_lib_unset(host_lib, hostname, SIMIX_HOST_LEVEL);

  /* jump to vm_ws_destroy(). The surf level resource will be freed. */
  surf_vm_workstation_model->extension.vm_workstation.destroy(host);
}

void SIMIX_pre_vm_destroy(smx_simcall_t simcall, smx_host_t vm){
   SIMIX_vm_destroy(vm);
}
