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
#include "src/plugins/vm/VmHostExt.hpp"

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

/* **** Check state of a VM **** */
void MSG_vm_set_bound(msg_vm_t vm, double bound)
{
  vm->setBound(bound);
}
static inline int __MSG_vm_is_state(msg_vm_t vm, e_surf_vm_state_t state)
{
  return vm->pimpl_vm_ != nullptr && vm->getState() == state;
}

/** @brief Returns whether the given VM has just created, not running.
 *  @ingroup msg_VMs
 */
int MSG_vm_is_created(msg_vm_t vm)
{
  return __MSG_vm_is_state(vm, SURF_VM_STATE_CREATED);
}

/** @brief Returns whether the given VM is currently running
 *  @ingroup msg_VMs
 */
int MSG_vm_is_running(msg_vm_t vm)
{
  return __MSG_vm_is_state(vm, SURF_VM_STATE_RUNNING);
}

/** @brief Returns whether the given VM is currently migrating
 *  @ingroup msg_VMs
 */
int MSG_vm_is_migrating(msg_vm_t vm)
{
  return vm->isMigrating();
}

/** @brief Returns whether the given VM is currently suspended, not running.
 *  @ingroup msg_VMs
 */
int MSG_vm_is_suspended(msg_vm_t vm)
{
  return __MSG_vm_is_state(vm, SURF_VM_STATE_SUSPENDED);
}

/* **** ******** MSG vm actions ********* **** */
/** @brief Create a new VM with specified parameters.
 *  @ingroup msg_VMs*
 *  @param pm        Physical machine that will host the VM
 *  @param name      Must be unique
 *  @param coreAmount Must be >= 1
 *  @param ramsize   [TODO]
 *  @param mig_netspeed Amount of Mbyte/s allocated to the migration (cannot be larger than net_cap). Use 0 if unsure.
 *  @param dp_intensity Dirty page percentage according to migNetSpeed, [0-100]. Use 0 if unsure.
 */
msg_vm_t MSG_vm_create(msg_host_t pm, const char* name, int coreAmount, int ramsize, int mig_netspeed, int dp_intensity)
{
  simgrid::vm::VmHostExt::ensureVmExtInstalled();

  /* For the moment, intensity_rate is the percentage against the migration bandwidth */

  msg_vm_t vm = new simgrid::s4u::VirtualMachine(name, pm, coreAmount, static_cast<sg_size_t>(ramsize) * 1024 * 1024);
  if (not sg_vm_is_migratable(vm)) {
    if (mig_netspeed != 0 || dp_intensity != 0)
      XBT_WARN("The live migration is not enabled. dp_intensity and mig_netspeed can't be used");
  } else {
    sg_vm_set_dirty_page_intensity(vm, dp_intensity / 100.0);
    sg_vm_set_working_set_memory(vm, vm->getRamsize() * 0.9); // assume working set memory is 90% of ramsize
    sg_vm_set_migration_speed(vm, mig_netspeed * 1024 * 1024.0);

    XBT_DEBUG("migspeed : %f intensity mem : %d", mig_netspeed * 1024 * 1024.0, dp_intensity);
  }

  return vm;
}

/** @brief Create a new VM object with the default parameters
 *  @ingroup msg_VMs*
 *
 * A VM is treated as a host. The name of the VM must be unique among all hosts.
 */
msg_vm_t MSG_vm_create_core(msg_host_t pm, const char* name)
{
  xbt_assert(sg_host_by_name(name) == nullptr,
             "Cannot create a VM named %s: this name is already used by an host or a VM", name);

  msg_vm_t vm = new simgrid::s4u::VirtualMachine(name, pm, 1);
  return vm;
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

  msg_vm_t vm = new simgrid::s4u::VirtualMachine(name, pm, coreAmount);
  return vm;
}

/** @brief Start a vm (i.e., boot the guest operating system)
 *  @ingroup msg_VMs
 *
 *  If the VM cannot be started (because of memory over-provisioning), an exception is generated.
 */
void MSG_vm_start(msg_vm_t vm)
{
  vm->start();
  if (TRACE_msg_vm_is_enabled()) {
    simgrid::instr::StateType* state = simgrid::instr::Container::byName(vm->getName())->getState("MSG_VM_STATE");
    state->addEntityValue("start", "0 0 1"); // start is blue
    state->pushEvent("start");
  }
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
  if (TRACE_msg_vm_is_enabled()) {
    simgrid::instr::StateType* state = simgrid::instr::Container::byName(vm->getName())->getState("MSG_VM_STATE");
    state->addEntityValue("suspend", "1 0 0"); // suspend is red
    state->pushEvent("suspend");
  }
}

/** @brief Resume the execution of the VM. All processes on the VM run again.
 *  @ingroup msg_VMs
 *
 * No resume cost occurs.
 */
void MSG_vm_resume(msg_vm_t vm)
{
  vm->resume();
  if (TRACE_msg_vm_is_enabled())
    simgrid::instr::Container::byName(vm->getName())->getState("MSG_VM_STATE")->popEvent();
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

/** @brief Destroy a VM. Destroy the VM object from the simulation.
 *  @ingroup msg_VMs
 */
void MSG_vm_destroy(msg_vm_t vm)
{
  if (vm->isMigrating())
    THROWF(vm_error, 0, "Cannot destroy VM '%s', which is migrating.", vm->getCname());

  /* First, terminate all processes on the VM if necessary */
  vm->shutdown();

  /* Then, destroy the VM object */
  vm->destroy();

  if (TRACE_msg_vm_is_enabled()) {
    container_t container = simgrid::instr::Container::byName(vm->getName());
    container->removeFromParent();
    delete container;
  }
}

}
