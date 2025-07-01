/* Copyright (c) 2015-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Engine.hpp"
#include "simgrid/simcall.hpp"
#include <simgrid/Exception.hpp>
#include <simgrid/kernel/routing/NetPoint.hpp>
#include <simgrid/vm.h>

#include "src/kernel/resource/VirtualMachineImpl.hpp"
#include "src/kernel/resource/models/cpu_cas01.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_vm, s4u, "S4U virtual machines");

namespace simgrid::s4u {
xbt::signal<void(VirtualMachine&)> VirtualMachine::on_vm_creation;
xbt::signal<void(VirtualMachine const&)> VirtualMachine::on_start;
xbt::signal<void(VirtualMachine const&)> VirtualMachine::on_started;
xbt::signal<void(VirtualMachine const&)> VirtualMachine::on_shutdown;
xbt::signal<void(VirtualMachine const&)> VirtualMachine::on_suspend;
xbt::signal<void(VirtualMachine const&)> VirtualMachine::on_resume;
xbt::signal<void(VirtualMachine const&)> VirtualMachine::on_migration_start;
xbt::signal<void(VirtualMachine const&)> VirtualMachine::on_migration_end;
xbt::signal<void(VirtualMachine const&)> VirtualMachine::on_vm_destruction;

xbt::Extension<Host, VmHostExt> VmHostExt::EXTENSION_ID;

void VmHostExt::ensureVmExtInstalled()
{
  if (not EXTENSION_ID.valid())
    EXTENSION_ID = Host::extension_create<VmHostExt>();
}

VirtualMachine::VirtualMachine(kernel::resource::VirtualMachineImpl* impl)
    : Host(impl), pimpl_vm_(dynamic_cast<kernel::resource::VirtualMachineImpl*>(Host::get_impl()))
{
}

void VirtualMachine::start()
{
  kernel::actor::simcall_answered([this]() { pimpl_vm_->start(); });
}

void VirtualMachine::suspend()
{
  const kernel::actor::ActorImpl* issuer = kernel::actor::ActorImpl::self();
  kernel::actor::simcall_answered([this, issuer]() { pimpl_vm_->suspend(issuer); });
}

void VirtualMachine::resume()
{
  pimpl_vm_->resume();
}

void VirtualMachine::shutdown()
{
  kernel::actor::ActorImpl* issuer = kernel::actor::ActorImpl::self();
  kernel::actor::simcall_answered([this, issuer]() { pimpl_vm_->shutdown(issuer); });
}

void VirtualMachine::destroy()
{
  auto destroy_code = [this]() {
    /* First, terminate all processes on the VM if necessary */
    shutdown();

    XBT_DEBUG("destroy %s", get_cname());
    on_vm_destruction(*this);
    on_this_vm_destruction(*this);
    /* Then, destroy the VM object */
    kernel::actor::simcall_answered(
        [this]() { get_vm_impl()->get_physical_host()->get_impl()->destroy_vm(get_name()); });
  };

  if (not this_actor::is_maestro() && this_actor::get_host() == this) {
    XBT_VERB("Launch another actor on physical host %s to destroy my own VM: %s", get_pm()->get_cname(), get_cname());
    get_pm()->add_actor(get_name() + "-vm_destroy",  destroy_code);
    simgrid::s4u::this_actor::yield();
    XBT_CRITICAL("I should be dead now!");
    DIE_IMPOSSIBLE;
  }

  destroy_code();
}

simgrid::s4u::Host* VirtualMachine::get_pm() const
{
  return pimpl_vm_->get_physical_host();
}

VirtualMachine* VirtualMachine::set_pm(simgrid::s4u::Host* pm)
{
  kernel::actor::simcall_answered([this, pm]() { pimpl_vm_->set_physical_host(pm); });
  return this;
}

VirtualMachine::State VirtualMachine::get_state() const
{
  return kernel::actor::simcall_answered([this]() { return pimpl_vm_->get_state(); });
}

size_t VirtualMachine::get_ramsize() const
{
  return pimpl_vm_->get_ramsize();
}

VirtualMachine* VirtualMachine::set_ramsize(size_t ramsize)
{
  pimpl_vm_->set_ramsize(ramsize);
  return this;
}
/** @brief Set a CPU bound for a given VM.
 *  @ingroup msg_VMs
 *
 * 1. Note that in some cases sg_exec_set_bound() may not intuitively work for VMs.
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
 * The current solution is to use set_bound(), which allows us to directly set the bound of the dummy CPU action.
 *
 * 2. Note that bound == 0 means no bound (i.e., unlimited). But, if a host has multiple CPU cores, the CPU share of a
 *    computation task (or a VM) never exceeds the capacity of a CPU core.
 */
VirtualMachine* VirtualMachine::set_bound(double bound)
{
  kernel::actor::simcall_answered([this, bound]() { pimpl_vm_->set_bound(bound); });
  return this;
}

void VirtualMachine::start_migration() const
{
  pimpl_vm_->start_migration();
}

void VirtualMachine::end_migration() const
{
  pimpl_vm_->end_migration();
}

} // namespace simgrid::s4u

/* **************************** Public C interface *************************** */

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
  return pm->create_vm(name, coreAmount);
}

const char* sg_vm_get_name(const_sg_vm_t vm)
{
  return vm->get_cname();
}

/** @brief Get the physical host of a given VM. */
sg_host_t sg_vm_get_pm(const_sg_vm_t vm)
{
  return vm->get_pm();
}

void sg_vm_set_ramsize(sg_vm_t vm, size_t size)
{
  vm->set_ramsize(size);
}

size_t sg_vm_get_ramsize(const_sg_vm_t vm)
{
  return vm->get_ramsize();
}

void sg_vm_set_bound(sg_vm_t vm, double bound)
{
  vm->set_bound(bound);
}

/** @brief Returns whether the given VM has just created, not running. */
int sg_vm_is_created(const_sg_vm_t vm)
{
  return vm->get_state() == simgrid::s4u::VirtualMachine::State::CREATED;
}

/** @brief Returns whether the given VM is currently running */
int sg_vm_is_running(const_sg_vm_t vm)
{
  return vm->get_state() == simgrid::s4u::VirtualMachine::State::RUNNING;
}

/** @brief Returns whether the given VM is currently suspended, not running. */
int sg_vm_is_suspended(const_sg_vm_t vm)
{
  return vm->get_state() == simgrid::s4u::VirtualMachine::State::SUSPENDED;
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
 *
 @beginrst

 The memory allocated by these actors is leaked, unless you used :cpp:func:`simgrid::s4u::Actor::on_exit`.

 @endrst
 *
 * No extra delay occurs by default. You may let your actor sleep by a specific amount to simulate any extra delay that
 you want.
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
