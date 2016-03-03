/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smx_private.h"
#include "mc/mc.h"
#include "src/surf/virtual_machine.hpp"
#include "src/surf/HostImpl.hpp"

//If you need to log some stuffs, just uncomment these two lines and uses XBT_DEBUG for instance
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_vm, simix, "Logging specific to SIMIX (vms)");

/* **** create a VM **** */

/**
 * \brief Internal function to create a SIMIX host.
 * \param name name of the host to create
 * \param data some user data (may be NULL)
 */
sg_host_t SIMIX_vm_create(const char *name, sg_host_t ind_phys_host)
{
  /* Create surf associated resource */
  surf_vm_model_create(name, ind_phys_host);
  sg_host_t host = sg_host_by_name(name);
  SIMIX_host_create(host);

  /* We will be able to register the VM to its physical host, so that we can promptly
   * retrieve the list VMs on the physical host. */

  return host;
}


/* works for VMs and PMs */
static long host_get_ramsize(sg_host_t vm, int *overcommit)
{
  s_vm_params_t params;
  vm->extension<simgrid::surf::HostImpl>()->getParams(&params);

  if (overcommit)
    *overcommit = params.overcommit;

  return params.ramsize;
}

/* **** start a VM **** */
static int __can_be_started(sg_host_t vm)
{
  sg_host_t pm = surf_vm_get_pm(vm);

  int pm_overcommit = 0;
  long pm_ramsize = host_get_ramsize(pm, &pm_overcommit);
  long vm_ramsize = host_get_ramsize(vm, NULL);

  if (!pm_ramsize) {
    /* We assume users do not want to care about ramsize. */
    return 1;
  }

  if (pm_overcommit) {
    XBT_VERB("%s allows memory overcommit.", sg_host_get_name(pm));
    return 1;
  }

  long total_ramsize_of_vms = 0;
  xbt_dynar_t dyn_vms = pm->extension<simgrid::surf::HostImpl>()->getVms();
  {
    unsigned int cursor = 0;
    sg_host_t another_vm;
    xbt_dynar_foreach(dyn_vms, cursor, another_vm) {
      long another_vm_ramsize = host_get_ramsize(vm, NULL);
      total_ramsize_of_vms += another_vm_ramsize;
    }
  }

  if (vm_ramsize > pm_ramsize - total_ramsize_of_vms) {
    XBT_WARN("cannnot start %s@%s due to memory shortage: vm_ramsize %ld, free %ld, pm_ramsize %ld (bytes).",
        sg_host_get_name(vm), sg_host_get_name(pm),
        vm_ramsize, pm_ramsize - total_ramsize_of_vms, pm_ramsize);
    xbt_dynar_free(&dyn_vms);
    return 0;
  }

  return 1;
}

void SIMIX_vm_start(sg_host_t ind_vm)
{
  if (__can_be_started(ind_vm))
    static_cast<simgrid::surf::VirtualMachine*>(
      ind_vm->extension<simgrid::surf::HostImpl>()
    )->setState(SURF_VM_STATE_RUNNING);
  else
    THROWF(vm_error, 0, "The VM %s cannot be started", sg_host_get_name(ind_vm));
}


e_surf_vm_state_t SIMIX_vm_get_state(sg_host_t ind_vm)
{
  return static_cast<simgrid::surf::VirtualMachine*>(
    ind_vm->extension<simgrid::surf::HostImpl>()
  )->getState();
}

/**
 * \brief Function to migrate a SIMIX VM host.
 *
 * \param host the vm host to migrate (a sg_host_t)
 */
void SIMIX_vm_migrate(sg_host_t ind_vm, sg_host_t ind_dst_pm)
{
  /* precopy migration makes the VM temporally paused */
  xbt_assert(SIMIX_vm_get_state(ind_vm) == SURF_VM_STATE_SUSPENDED);

  /* jump to vm_ws_xigrate(). this will update the vm location. */
  surf_vm_migrate(ind_vm, ind_dst_pm);
}

/**
 * \brief Encompassing simcall to prevent the removal of the src or the dst node at the end of a VM migration
 *  The simcall actually invokes the following calls: 
 *     simcall_vm_set_affinity(vm, src_pm, 0); 
 *     simcall_vm_migrate(vm, dst_pm); 
 *     simcall_vm_resume(vm);
 *
 * It is called at the end of the migration_rx_fun function from msg/msg_vm.c
 *
 * \param vm VM to migrate
 * \param src_pm  Source physical host
 * \param dst_pmt Destination physical host
 */
void SIMIX_vm_migratefrom_resumeto(sg_host_t vm, sg_host_t src_pm, sg_host_t dst_pm)
{
  /* deinstall the current affinity setting for the CPU */
  SIMIX_vm_set_affinity(vm, src_pm, 0);

  /* Update the vm location */
  SIMIX_vm_migrate(vm, dst_pm);
 
  /* Resume the VM */
  smx_process_t self = SIMIX_process_self(); 
  SIMIX_vm_resume(vm, self->simcall.issuer);
} 

/**
 * \brief Function to get the physical host of the given SIMIX VM host.
 *
 * \param host the vm host to get_phys_host (a sg_host_t)
 */
void *SIMIX_vm_get_pm(sg_host_t ind_vm)
{
  /* jump to vm_ws_get_pm(). this will return the vm name. */
  return surf_vm_get_pm(ind_vm);
}

/**
 * \brief Function to set the CPU bound of the given SIMIX VM host.
 *
 * \param host the vm host (a sg_host_t)
 * \param bound bound (a double)
 */
void SIMIX_vm_set_bound(sg_host_t ind_vm, double bound)
{
  /* jump to vm_ws_set_vm_bound(). */
  surf_vm_set_bound(ind_vm, bound);
}

/**
 * \brief Function to set the CPU affinity of the given SIMIX VM host.
 *
 * \param host the vm host (a sg_host_t)
 * \param host the pm host (a sg_host_t)
 * \param mask affinity mask (a unsigned long)
 */
void SIMIX_vm_set_affinity(sg_host_t ind_vm, sg_host_t ind_pm, unsigned long mask)
{
  /* make sure this at the MSG layer. */
  xbt_assert(SIMIX_vm_get_pm(ind_vm) == ind_pm);

  /* jump to vm_ws_set_vm_affinity(). */
  surf_vm_set_affinity(ind_vm, ind_pm, mask);
}


/**
 * \brief Function to suspend a SIMIX VM host. This function stops the execution of the
 * VM. All the processes on this VM will pause. The state of the VM is
 * preserved on memory. We can later resume it again.
 *
 * \param host the vm host to suspend (a sg_host_t)
 */
void SIMIX_vm_suspend(sg_host_t ind_vm, smx_process_t issuer)
{
  const char *name = sg_host_get_name(ind_vm);

  if (SIMIX_vm_get_state(ind_vm) != SURF_VM_STATE_RUNNING)
    THROWF(vm_error, 0, "VM(%s) is not running", name);

  XBT_DEBUG("suspend VM(%s), where %d processes exist", name, xbt_swag_size(sg_host_simix(ind_vm)->process_list));

  /* jump to vm_ws_suspend. The state will be set. */
  surf_vm_suspend(ind_vm);

  smx_process_t smx_process, smx_process_safe;
  xbt_swag_foreach_safe(smx_process, smx_process_safe, sg_host_simix(ind_vm)->process_list) {
    XBT_DEBUG("suspend %s", smx_process->name);
    SIMIX_process_suspend(smx_process, issuer);
  }

  XBT_DEBUG("suspend all processes on the VM done done");
}

void simcall_HANDLER_vm_suspend(smx_simcall_t simcall, sg_host_t ind_vm)
{
  if (simcall->issuer->host == ind_vm) {
    XBT_ERROR("cannot suspend the VM where I run");
    DIE_IMPOSSIBLE;
  }

  SIMIX_vm_suspend(ind_vm, simcall->issuer);

  XBT_DEBUG("simcall_HANDLER_vm_suspend done");
}


/**
 * \brief Function to resume a SIMIX VM host. This function restart the execution of the
 * VM. All the processes on this VM will run again.
 *
 * \param host the vm host to resume (a sg_host_t)
 */
void SIMIX_vm_resume(sg_host_t ind_vm, smx_process_t issuer)
{
  const char *name = sg_host_get_name(ind_vm);

  if (SIMIX_vm_get_state(ind_vm) != SURF_VM_STATE_SUSPENDED)
    THROWF(vm_error, 0, "VM(%s) was not suspended", name);

  XBT_DEBUG("resume VM(%s), where %d processes exist", name, xbt_swag_size(sg_host_simix(ind_vm)->process_list));

  /* jump to vm_ws_resume() */
  surf_vm_resume(ind_vm);

  smx_process_t smx_process, smx_process_safe;
  xbt_swag_foreach_safe(smx_process, smx_process_safe, sg_host_simix(ind_vm)->process_list) {
    XBT_DEBUG("resume %s", smx_process->name);
    SIMIX_process_resume(smx_process, issuer);
  }
}

void simcall_HANDLER_vm_resume(smx_simcall_t simcall, sg_host_t ind_vm)
{
  SIMIX_vm_resume(ind_vm, simcall->issuer);
}


/**
 * \brief Function to save a SIMIX VM host.
 * This function is the same as vm_suspend, but the state of the VM is saved to the disk, and not preserved on memory.
 * We can later restore it again.
 *
 * \param host the vm host to save (a sg_host_t)
 */
void SIMIX_vm_save(sg_host_t ind_vm, smx_process_t issuer)
{
  const char *name = sg_host_get_name(ind_vm);

  if (SIMIX_vm_get_state(ind_vm) != SURF_VM_STATE_RUNNING)
    THROWF(vm_error, 0, "VM(%s) is not running", name);


  XBT_DEBUG("save VM(%s), where %d processes exist", name, xbt_swag_size(sg_host_simix(ind_vm)->process_list));

  /* jump to vm_ws_save() */
  surf_vm_save(ind_vm);

  smx_process_t smx_process, smx_process_safe;
  xbt_swag_foreach_safe(smx_process, smx_process_safe, sg_host_simix(ind_vm)->process_list) {
    XBT_DEBUG("suspend %s", smx_process->name);
    SIMIX_process_suspend(smx_process, issuer);
  }
}

void simcall_HANDLER_vm_save(smx_simcall_t simcall, sg_host_t ind_vm)
{
  SIMIX_vm_save(ind_vm, simcall->issuer);
}


/**
 * \brief Function to restore a SIMIX VM host. This function restart the execution of the
 * VM. All the processes on this VM will run again.
 *
 * \param host the vm host to restore (a sg_host_t)
 */
void SIMIX_vm_restore(sg_host_t ind_vm, smx_process_t issuer)
{
  const char *name = sg_host_get_name(ind_vm);

  if (SIMIX_vm_get_state(ind_vm) != SURF_VM_STATE_SAVED)
    THROWF(vm_error, 0, "VM(%s) was not saved", name);

  XBT_DEBUG("restore VM(%s), where %d processes exist", name, xbt_swag_size(sg_host_simix(ind_vm)->process_list));

  /* jump to vm_ws_restore() */
  surf_vm_resume(ind_vm);

  smx_process_t smx_process, smx_process_safe;
  xbt_swag_foreach_safe(smx_process, smx_process_safe, sg_host_simix(ind_vm)->process_list) {
    XBT_DEBUG("resume %s", smx_process->name);
    SIMIX_process_resume(smx_process, issuer);
  }
}

void simcall_HANDLER_vm_restore(smx_simcall_t simcall, sg_host_t ind_vm)
{
  SIMIX_vm_restore(ind_vm, simcall->issuer);
}


/**
 * \brief Function to shutdown a SIMIX VM host. This function powers off the
 * VM. All the processes on this VM will be killed. But, the state of the VM is
 * preserved on memory. We can later start it again.
 *
 * \param host the vm host to shutdown (a sg_host_t)
 */
void SIMIX_vm_shutdown(sg_host_t ind_vm, smx_process_t issuer)
{
  const char *name = sg_host_get_name(ind_vm);

  if (SIMIX_vm_get_state(ind_vm) != SURF_VM_STATE_RUNNING)
    THROWF(vm_error, 0, "VM(%s) is not running", name);

  XBT_DEBUG("shutdown %s", name);
  XBT_DEBUG("%d processes in the VM", xbt_swag_size(sg_host_simix(ind_vm)->process_list));

  smx_process_t smx_process, smx_process_safe;
  xbt_swag_foreach_safe(smx_process, smx_process_safe, sg_host_simix(ind_vm)->process_list) {
    XBT_DEBUG("kill %s", smx_process->name);
    SIMIX_process_kill(smx_process, issuer);
  }

  /* FIXME: we may have to do something at the surf layer, e.g., vcpu action */
  static_cast<simgrid::surf::VirtualMachine*>(
    ind_vm->extension<simgrid::surf::HostImpl>()
  )->setState(SURF_VM_STATE_CREATED);
}

void simcall_HANDLER_vm_shutdown(smx_simcall_t simcall, sg_host_t ind_vm)
{
  SIMIX_vm_shutdown(ind_vm, simcall->issuer);
}


/**
 * \brief Function to destroy a SIMIX VM host.
 *
 * \param host the vm host to destroy (a sg_host_t)
 */
void SIMIX_vm_destroy(sg_host_t ind_vm)
{
  /* this code basically performs a similar thing like SIMIX_host_destroy() */

  const char *hostname = sg_host_get_name(ind_vm);

  XBT_DEBUG("destroy %s", hostname);

  /* this will call the registered callback function, i.e., SIMIX_host_destroy().  */
  sg_host_simix_destroy(ind_vm);

  /* jump to vm_ws_destroy(). The surf level resource will be freed. */
  surf_vm_destroy(ind_vm);
}
