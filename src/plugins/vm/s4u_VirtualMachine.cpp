/* Copyright (c) 2015-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Actor.hpp"
#include "src/instr/instr_private.hpp"
#include "src/plugins/vm/VirtualMachineImpl.hpp"
#include "src/plugins/vm/VmHostExt.hpp"
#include "src/simix/smx_host_private.hpp"
#include "src/surf/cpu_cas01.hpp"
#include <simgrid/vm.h>
#include <src/plugins/vm/VmLiveMigration.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_vm, "S4U virtual machines");

simgrid::xbt::signal<void(simgrid::s4u::VirtualMachine*)> simgrid::s4u::VirtualMachine::onVmShutdown;

namespace simgrid {
namespace s4u {

VirtualMachine::VirtualMachine(const char* name, s4u::Host* pm, int coreAmount)
    : VirtualMachine(name, pm, coreAmount, 1024)
{
}

VirtualMachine::VirtualMachine(const char* name, s4u::Host* pm, int coreAmount, size_t ramsize)
    : Host(name), pimpl_vm_(new vm::VirtualMachineImpl(this, pm, coreAmount, ramsize))
{
  XBT_DEBUG("Create VM %s", name);

  /* Currently, a VM uses the network resource of its physical host */
  pimpl_netpoint = pm->pimpl_netpoint;
  // Create a VCPU for this VM
  surf::CpuCas01* sub_cpu = dynamic_cast<surf::CpuCas01*>(pm->pimpl_cpu);

  pimpl_cpu = surf_cpu_model_vm->createCpu(this, sub_cpu->getSpeedPeakList(), coreAmount);
  if (sub_cpu->getPState() != 0)
    pimpl_cpu->setPState(sub_cpu->getPState());

  /* Make a process container */
  extension_set<simgrid::simix::Host>(new simgrid::simix::Host());

  if (TRACE_msg_vm_is_enabled()) {
    container_t host_container = instr::Container::byName(pm->getName());
    new instr::Container(name, "MSG_VM", host_container);
    instr::Container::byName(getName())->getState("MSG_VM_STATE")->addEntityValue("start", "0 0 1");   // start is blue
    instr::Container::byName(getName())->getState("MSG_VM_STATE")->addEntityValue("suspend", "1 0 0"); // suspend is red
  }
}

VirtualMachine::~VirtualMachine()
{
  onDestruction(*this);

  XBT_DEBUG("destroy %s", getCname());

  /* FIXME: this is really strange that everything fails if the next line is removed.
   * This is as if we shared these data with the PM, which definitely should not be the case...
   *
   * We need to test that suspending a VM does not suspends the processes running on its PM, for example.
   * Or we need to simplify this code enough to make it actually readable (but this sounds harder than testing)
   */
  extension_set<simgrid::simix::Host>(nullptr);

  /* Don't free these things twice: they are the ones of my physical host */
  pimpl_netpoint = nullptr;

  if (TRACE_msg_vm_is_enabled()) {
    container_t container = simgrid::instr::Container::byName(getName());
    container->removeFromParent();
    delete container;
  }
}

void VirtualMachine::start()
{
  if (TRACE_msg_vm_is_enabled())
    simgrid::instr::Container::byName(getName())->getState("MSG_VM_STATE")->pushEvent("start");

  simgrid::simix::kernelImmediate([this]() {
    simgrid::vm::VmHostExt::ensureVmExtInstalled();

    simgrid::s4u::Host* pm = this->pimpl_vm_->getPm();
    if (pm->extension<simgrid::vm::VmHostExt>() == nullptr)
      pm->extension_set(new simgrid::vm::VmHostExt());

    long pm_ramsize   = pm->extension<simgrid::vm::VmHostExt>()->ramsize;
    int pm_overcommit = pm->extension<simgrid::vm::VmHostExt>()->overcommit;
    long vm_ramsize   = this->getRamsize();

    if (pm_ramsize && not pm_overcommit) { /* Only verify that we don't overcommit on need */
      /* Retrieve the memory occupied by the VMs on that host. Yep, we have to traverse all VMs of all hosts for that */
      long total_ramsize_of_vms = 0;
      for (simgrid::s4u::VirtualMachine* const& ws_vm : simgrid::vm::VirtualMachineImpl::allVms_)
        if (pm == ws_vm->getPm())
          total_ramsize_of_vms += ws_vm->getRamsize();

      if (vm_ramsize > pm_ramsize - total_ramsize_of_vms) {
        XBT_WARN("cannnot start %s@%s due to memory shortage: vm_ramsize %ld, free %ld, pm_ramsize %ld (bytes).",
                 this->getCname(), pm->getCname(), vm_ramsize, pm_ramsize - total_ramsize_of_vms, pm_ramsize);
        THROWF(vm_error, 0, "Memory shortage on host '%s', VM '%s' cannot be started", pm->getCname(),
               this->getCname());
      }
    }

    this->pimpl_vm_->setState(SURF_VM_STATE_RUNNING);
  });

  if (TRACE_msg_vm_is_enabled())
    simgrid::instr::Container::byName(getName())->getState("MSG_VM_STATE")->popEvent();
}

void VirtualMachine::suspend()
{
  smx_actor_t issuer = SIMIX_process_self();
  simgrid::simix::kernelImmediate([this, issuer]() { pimpl_vm_->suspend(issuer); });
  if (TRACE_msg_vm_is_enabled())
    simgrid::instr::Container::byName(getName())->getState("MSG_VM_STATE")->pushEvent("suspend");
  XBT_DEBUG("vm_suspend done");
}

void VirtualMachine::resume()
{
  pimpl_vm_->resume();
  if (TRACE_msg_vm_is_enabled())
    simgrid::instr::Container::byName(getName())->getState("MSG_VM_STATE")->popEvent();
}

void VirtualMachine::shutdown()
{
  smx_actor_t issuer = SIMIX_process_self();
  simgrid::simix::kernelImmediate([this, issuer]() { pimpl_vm_->shutdown(issuer); });
  onVmShutdown(this);
}

void VirtualMachine::destroy()
{
  /* First, terminate all processes on the VM if necessary */
  shutdown();

  /* Then, destroy the VM object */
  Host::destroy();
}

simgrid::s4u::Host* VirtualMachine::getPm()
{
  return pimpl_vm_->getPm();
}

void VirtualMachine::setPm(simgrid::s4u::Host* pm)
{
  simgrid::simix::kernelImmediate([this, pm]() { pimpl_vm_->setPm(pm); });
}

e_surf_vm_state_t VirtualMachine::getState()
{
  return simgrid::simix::kernelImmediate([this]() { return pimpl_vm_->getState(); });
}

size_t VirtualMachine::getRamsize()
{
  return pimpl_vm_->getRamsize();
}

void VirtualMachine::setRamsize(size_t ramsize)
{
  pimpl_vm_->setRamsize(ramsize);
}
/** @brief Set a CPU bound for a given VM.
 *  @ingroup msg_VMs
 *
 * 1. Note that in some cases MSG_task_set_bound() may not intuitively work for VMs.
 *
 * For example,
 *  On PM0, there are Task1 and VM0.
 *  On VM0, there is Task2.
 * Now we bound 75% to Task1\@PM0 and bound 25% to Task2\@VM0.
 * Then,
 *  Task1\@PM0 gets 50%.
 *  Task2\@VM0 gets 25%.
 * This is NOT 75% for Task1\@PM0 and 25% for Task2\@VM0, respectively.
 *
 * This is because a VM has the dummy CPU action in the PM layer. Putting a task on the VM does not affect the bound of
 * the dummy CPU action. The bound of the dummy CPU action is unlimited.
 *
 * There are some solutions for this problem. One option is to update the bound of the dummy CPU action automatically.
 * It should be the sum of all tasks on the VM. But, this solution might be costly, because we have to scan all tasks
 * on the VM in share_resource() or we have to trap both the start and end of task execution.
 *
 * The current solution is to use setBound(), which allows us to directly set the bound of the dummy CPU action.
 *
 * 2. Note that bound == 0 means no bound (i.e., unlimited). But, if a host has multiple CPU cores, the CPU share of a
 *    computation task (or a VM) never exceeds the capacity of a CPU core.
 */
void VirtualMachine::setBound(double bound)
{
  simgrid::simix::kernelImmediate([this, bound]() { pimpl_vm_->setBound(bound); });
}

} // namespace simgrid
} // namespace s4u

/* **************************** Public C interface *************************** */

SG_BEGIN_DECL()
/** @brief Create a new VM object with the default parameters
 * A VM is treated as a host. The name of the VM must be unique among all hosts.
 */
sg_vm_t sg_vm_create_core(sg_host_t pm, const char* name)
{
  return sg_vm_create_multicore(pm, name, 1);
}
/** @brief Create a new VM object with the default parameters, but with a specified amount of cores
 * A VM is treated as a host. The name of the VM must be unique among all hosts.
 */
sg_vm_t sg_vm_create_multicore(sg_host_t pm, const char* name, int coreAmount)
{
  xbt_assert(sg_host_by_name(name) == nullptr,
             "Cannot create a VM named %s: this name is already used by an host or a VM", name);

  return new simgrid::s4u::VirtualMachine(name, pm, coreAmount);
}

const char* sg_vm_get_name(sg_vm_t vm)
{
  return vm->getCname();
}

/** @brief Get the physical host of a given VM. */
sg_host_t sg_vm_get_pm(sg_vm_t vm)
{
  return vm->getPm();
}

void sg_vm_set_ramsize(sg_vm_t vm, size_t size)
{
  vm->setRamsize(size);
}

size_t sg_vm_get_ramsize(sg_vm_t vm)
{
  return vm->getRamsize();
}

void sg_vm_set_bound(sg_vm_t vm, double bound)
{
  vm->setBound(bound);
}

/** @brief Returns whether the given VM has just created, not running. */
int sg_vm_is_created(sg_vm_t vm)
{
  return vm->getState() == SURF_VM_STATE_CREATED;
}

/** @brief Returns whether the given VM is currently running */
int sg_vm_is_running(sg_vm_t vm)
{
  return vm->getState() == SURF_VM_STATE_RUNNING;
}

/** @brief Returns whether the given VM is currently suspended, not running. */
int sg_vm_is_suspended(sg_vm_t vm)
{
  return vm->getState() == SURF_VM_STATE_SUSPENDED;
}

/** @brief Start a vm (i.e., boot the guest operating system)
 *  If the VM cannot be started (because of memory over-provisioning), an exception is generated.
 */
void sg_vm_start(sg_vm_t vm)
{
  vm->start();
}

/** @brief Immediately suspend the execution of all processes within the given VM.
 *
 * This function stops the execution of the VM. All the processes on this VM
 * will pause. The state of the VM is preserved. We can later resume it again.
 *
 * No suspension cost occurs.
 */
void sg_vm_suspend(sg_vm_t vm)
{
  vm->suspend();
}

/** @brief Resume the execution of the VM. All processes on the VM run again.
 * No resume cost occurs.
 */
void sg_vm_resume(sg_vm_t vm)
{
  vm->resume();
}

/** @brief Immediately kills all processes within the given VM.
 * Any memory that they allocated will be leaked, unless you used #MSG_process_on_exit().
 *
 * No extra delay occurs. If you want to simulate this too, you want to use a #MSG_process_sleep().
 */
void sg_vm_shutdown(sg_vm_t vm)
{
  vm->shutdown();
}

/** @brief Destroy a VM. Destroy the VM object from the simulation. */
void sg_vm_destroy(sg_vm_t vm)
{
  vm->destroy();
}
SG_END_DECL()
