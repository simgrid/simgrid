/* Copyright (c) 2007-2015. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc/mc.h"
#include "simgrid/s4u/VirtualMachine.hpp"
#include "smx_private.h"
#include "src/plugins/vm/VirtualMachineImpl.hpp"
#include "src/plugins/vm/VmHostExt.hpp"
#include "src/surf/HostImpl.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_vm, simix, "Logging specific to SIMIX Virtual Machines");

/* works for VMs and PMs */
static long host_get_ramsize(sg_host_t vm, int *overcommit)
{
  s_vm_params_t params;
  static_cast<simgrid::s4u::VirtualMachine*>(vm)->parameters(&params);

  if (overcommit)
    *overcommit = params.overcommit;

  return params.ramsize;
}

/* **** start a VM **** */
void SIMIX_vm_start(sg_host_t vm)
{
  sg_host_t pm = static_cast<simgrid::s4u::VirtualMachine*>(vm)->pimpl_vm_->getPm();

  simgrid::vm::VmHostExt::ensureVmExtInstalled();
  if (pm->extension<simgrid::vm::VmHostExt>() == nullptr)
    pm->extension_set(new simgrid::vm::VmHostExt());

  long pm_ramsize = pm->extension<simgrid::vm::VmHostExt>()->ramsize;
  int pm_overcommit = pm->extension<simgrid::vm::VmHostExt>()->overcommit;
  long vm_ramsize = host_get_ramsize(vm, nullptr);

  if (pm_ramsize && !pm_overcommit) { /* Only verify that we don't overcommit on need */
    /* Retrieve the memory occupied by the VMs on that host. Yep, we have to traverse all VMs of all hosts for that */
    long total_ramsize_of_vms = 0;
    for (simgrid::surf::VirtualMachineImpl* ws_vm : simgrid::surf::VirtualMachineImpl::allVms_)
      if (pm == ws_vm->getPm())
        total_ramsize_of_vms += ws_vm->getRamsize();

    if (vm_ramsize > pm_ramsize - total_ramsize_of_vms) {
      XBT_WARN("cannnot start %s@%s due to memory shortage: vm_ramsize %ld, free %ld, pm_ramsize %ld (bytes).",
               sg_host_get_name(vm), sg_host_get_name(pm), vm_ramsize, pm_ramsize - total_ramsize_of_vms, pm_ramsize);
      THROWF(vm_error, 0, "The VM %s cannot be started", vm->name().c_str());
    }
  }

  static_cast<simgrid::s4u::VirtualMachine*>(vm)->pimpl_vm_->setState(SURF_VM_STATE_RUNNING);
}

/**
 * @brief Function to suspend a SIMIX VM host. This function stops the execution of the
 * VM. All the processes on this VM will pause. The state of the VM is
 * preserved on memory. We can later resume it again.
 *
 * @param vm the vm host to suspend (a sg_host_t)
 */
void SIMIX_vm_suspend(sg_host_t vm, smx_actor_t issuer)
{
  if (static_cast<simgrid::s4u::VirtualMachine*>(vm)->pimpl_vm_->getState() != SURF_VM_STATE_RUNNING)
    THROWF(vm_error, 0, "VM(%s) is not running", vm->name().c_str());

  XBT_DEBUG("suspend VM(%s), where %d processes exist", vm->name().c_str(), xbt_swag_size(sg_host_simix(vm)->process_list));

  /* jump to vm_ws_suspend. The state will be set. */
  static_cast<simgrid::s4u::VirtualMachine*>(vm)->pimpl_vm_->suspend();

  smx_actor_t smx_process, smx_process_safe;
  xbt_swag_foreach_safe(smx_process, smx_process_safe, sg_host_simix(vm)->process_list) {
    XBT_DEBUG("suspend %s", smx_process->name.c_str());
    SIMIX_process_suspend(smx_process, issuer);
  }

  XBT_DEBUG("suspend all processes on the VM done done");
}

void simcall_HANDLER_vm_suspend(smx_simcall_t simcall, sg_host_t vm)
{
  xbt_assert(simcall->issuer->host != vm, "cannot suspend the VM where I run");

  SIMIX_vm_suspend(vm, simcall->issuer);

  XBT_DEBUG("simcall_HANDLER_vm_suspend done");
}


/**
 * @brief Function to resume a SIMIX VM host. This function restart the execution of the
 * VM. All the processes on this VM will run again.
 *
 * @param vm the vm host to resume (a sg_host_t)
 */
void SIMIX_vm_resume(sg_host_t vm)
{
  if (static_cast<simgrid::s4u::VirtualMachine*>(vm)->pimpl_vm_->getState() != SURF_VM_STATE_SUSPENDED)
    THROWF(vm_error, 0, "VM(%s) was not suspended", vm->name().c_str());

  XBT_DEBUG("resume VM(%s), where %d processes exist",
      vm->name().c_str(), xbt_swag_size(sg_host_simix(vm)->process_list));

  /* jump to vm_ws_resume() */
  static_cast<simgrid::s4u::VirtualMachine*>(vm)->pimpl_vm_->resume();

  smx_actor_t smx_process, smx_process_safe;
  xbt_swag_foreach_safe(smx_process, smx_process_safe, sg_host_simix(vm)->process_list) {
    XBT_DEBUG("resume %s", smx_process->name.c_str());
    SIMIX_process_resume(smx_process);
  }
}

/**
 * @brief Function to save a SIMIX VM host.
 * This function is the same as vm_suspend, but the state of the VM is saved to the disk, and not preserved on memory.
 * We can later restore it again.
 *
 * @param vm the vm host to save (a sg_host_t)
 */
void SIMIX_vm_save(sg_host_t vm, smx_actor_t issuer)
{
  const char *name = sg_host_get_name(vm);

  if (static_cast<simgrid::s4u::VirtualMachine*>(vm)->pimpl_vm_->getState() != SURF_VM_STATE_RUNNING)
    THROWF(vm_error, 0, "VM(%s) is not running", name);

  XBT_DEBUG("save VM(%s), where %d processes exist", name, xbt_swag_size(sg_host_simix(vm)->process_list));

  /* jump to vm_ws_save() */
  static_cast<simgrid::s4u::VirtualMachine*>(vm)->pimpl_vm_->save();

  smx_actor_t smx_process, smx_process_safe;
  xbt_swag_foreach_safe(smx_process, smx_process_safe, sg_host_simix(vm)->process_list) {
    XBT_DEBUG("suspend %s", smx_process->name.c_str());
    SIMIX_process_suspend(smx_process, issuer);
  }
}

void simcall_HANDLER_vm_save(smx_simcall_t simcall, sg_host_t vm)
{
  SIMIX_vm_save(vm, simcall->issuer);
}

/**
 * @brief Function to shutdown a SIMIX VM host. This function powers off the
 * VM. All the processes on this VM will be killed. But, the state of the VM is
 * preserved on memory. We can later start it again.
 *
 * @param vm the VM to shutdown (a sg_host_t)
 */
void SIMIX_vm_shutdown(sg_host_t vm, smx_actor_t issuer)
{
  if (static_cast<simgrid::s4u::VirtualMachine*>(vm)->pimpl_vm_->getState() != SURF_VM_STATE_RUNNING)
    THROWF(vm_error, 0, "VM(%s) is not running", vm->name().c_str());

  XBT_DEBUG("shutdown VM %s, that contains %d processes",
      vm->name().c_str(),xbt_swag_size(sg_host_simix(vm)->process_list));

  smx_actor_t smx_process, smx_process_safe;
  xbt_swag_foreach_safe(smx_process, smx_process_safe, sg_host_simix(vm)->process_list) {
    XBT_DEBUG("kill %s", smx_process->name.c_str());
    SIMIX_process_kill(smx_process, issuer);
  }

  /* FIXME: we may have to do something at the surf layer, e.g., vcpu action */
  static_cast<simgrid::s4u::VirtualMachine*>(vm)->pimpl_vm_->setState(SURF_VM_STATE_CREATED);
}

void simcall_HANDLER_vm_shutdown(smx_simcall_t simcall, sg_host_t vm)
{
  SIMIX_vm_shutdown(vm, simcall->issuer);
}
