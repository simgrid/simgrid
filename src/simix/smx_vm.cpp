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
    THROWF(vm_error, 0, "VM(%s) is not running", vm->cname());

  XBT_DEBUG("shutdown VM %s, that contains %d processes", vm->cname(), xbt_swag_size(sg_host_simix(vm)->process_list));

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
