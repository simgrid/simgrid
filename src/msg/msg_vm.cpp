/* Copyright (c) 2012-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* TODO:
 * 1. add the support of trace
 * 2. use parallel tasks to simulate CPU overhead and remove the experimental code generating micro computation tasks
 */

#include <xbt/ex.hpp>

#include "simgrid/plugins/live_migration.h"
#include "src/instr/instr_private.hpp"
#include "src/plugins/vm/VirtualMachineImpl.hpp"

#include "simgrid/host.h"
#include "simgrid/simix.hpp"
#include "xbt/string.hpp"

extern "C" {

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_vm, msg, "Cloud-oriented parts of the MSG API");

const char* MSG_vm_get_name(msg_vm_t vm)
{
  return vm->getCname();
}

/** @brief Get the physical host of a given VM.
 *  @ingroup msg_VMs
 */
msg_host_t MSG_vm_get_pm(msg_vm_t vm)
{
  return vm->getPm();
}

void MSG_vm_set_ramsize(msg_vm_t vm, size_t size)
{
  vm->setRamsize(size);
}

size_t MSG_vm_get_ramsize(msg_vm_t vm)
{
  return vm->getRamsize();
}

void MSG_vm_set_bound(msg_vm_t vm, double bound)
{
  vm->setBound(bound);
}

/** @brief Returns whether the given VM has just created, not running.
 *  @ingroup msg_VMs
 */
int MSG_vm_is_created(msg_vm_t vm)
{
  return vm->getState() == SURF_VM_STATE_CREATED;
}

/** @brief Returns whether the given VM is currently running
 *  @ingroup msg_VMs
 */
int MSG_vm_is_running(msg_vm_t vm)
{
  return vm->getState() == SURF_VM_STATE_RUNNING;
}

/** @brief Returns whether the given VM is currently suspended, not running.
 *  @ingroup msg_VMs
 */
int MSG_vm_is_suspended(msg_vm_t vm)
{
  return vm->getState() == SURF_VM_STATE_SUSPENDED;
}

/* **** ******** MSG vm actions ********* **** */
/** @brief Create a new VM object with the default parameters
 *  @ingroup msg_VMs*
 *
 * A VM is treated as a host. The name of the VM must be unique among all hosts.
 */
msg_vm_t MSG_vm_create_core(msg_host_t pm, const char* name)
{
  return MSG_vm_create_multicore(pm, name, 1);
}
/** @brief Create a new VM object with the default parameters, but with a specified amount of cores
 *  @ingroup msg_VMs*
 *
 * A VM is treated as a host. The name of the VM must be unique among all hosts.
 */
msg_vm_t MSG_vm_create_multicore(msg_host_t pm, const char* name, int coreAmount)
{
  xbt_assert(sg_host_by_name(name) == nullptr,
             "Cannot create a VM named %s: this name is already used by an host or a VM", name);

  return new simgrid::s4u::VirtualMachine(name, pm, coreAmount);
}

/** @brief Start a vm (i.e., boot the guest operating system)
 *  @ingroup msg_VMs
 *
 *  If the VM cannot be started (because of memory over-provisioning), an exception is generated.
 */
void MSG_vm_start(msg_vm_t vm)
{
  vm->start();
}

/** @brief Immediately suspend the execution of all processes within the given VM.
 *  @ingroup msg_VMs
 *
 * This function stops the execution of the VM. All the processes on this VM
 * will pause. The state of the VM is preserved. We can later resume it again.
 *
 * No suspension cost occurs.
 */
void MSG_vm_suspend(msg_vm_t vm)
{
  vm->suspend();
}

/** @brief Resume the execution of the VM. All processes on the VM run again.
 *  @ingroup msg_VMs
 *
 * No resume cost occurs.
 */
void MSG_vm_resume(msg_vm_t vm)
{
  vm->resume();
}

/** @brief Immediately kills all processes within the given VM.
 *  @ingroup msg_VMs
 *
 * Any memory that they allocated will be leaked, unless you used #MSG_process_on_exit().
 *
 * No extra delay occurs. If you want to simulate this too, you want to use a #MSG_process_sleep().
 */
void MSG_vm_shutdown(msg_vm_t vm)
{
  vm->shutdown();
}

/* Deprecated. Please use MSG_vm_create_migratable() instead */
msg_vm_t MSG_vm_create(msg_host_t ind_pm, const char* name, int coreAmount, int ramsize, int mig_netspeed,
                       int dp_intensity)
{
  return sg_vm_create_migratable(ind_pm, name, coreAmount, ramsize, mig_netspeed, dp_intensity);
}

/** @brief Destroy a VM. Destroy the VM object from the simulation.
 *  @ingroup msg_VMs
 */
void MSG_vm_destroy(msg_vm_t vm)
{
  vm->destroy();
}
}
