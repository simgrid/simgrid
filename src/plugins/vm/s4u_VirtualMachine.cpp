/* Copyright (c) 2015-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "simgrid/s4u/Actor.hpp"
#include "simgrid/vm.h"
#include "src/include/surf/surf.hpp"
#include "src/plugins/vm/VirtualMachineImpl.hpp"
#include "src/plugins/vm/VmHostExt.hpp"
#include "src/surf/cpu_cas01.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_vm, s4u, "S4U virtual machines");

namespace simgrid {
namespace s4u {
simgrid::xbt::signal<void(VirtualMachine const&)> VirtualMachine::on_start;
simgrid::xbt::signal<void(VirtualMachine const&)> VirtualMachine::on_started;
simgrid::xbt::signal<void(VirtualMachine const&)> VirtualMachine::on_shutdown;
simgrid::xbt::signal<void(VirtualMachine const&)> VirtualMachine::on_suspend;
simgrid::xbt::signal<void(VirtualMachine const&)> VirtualMachine::on_resume;
simgrid::xbt::signal<void(VirtualMachine const&)> VirtualMachine::on_migration_start;
simgrid::xbt::signal<void(VirtualMachine const&)> VirtualMachine::on_migration_end;

VirtualMachine::VirtualMachine(const std::string& name, s4u::Host* physical_host, int core_amount)
    : VirtualMachine(name, physical_host, core_amount, 1024)
{
}

VirtualMachine::VirtualMachine(const std::string& name, s4u::Host* physical_host, int core_amount, size_t ramsize)
    : Host(name), pimpl_vm_(new vm::VirtualMachineImpl(this, physical_host, core_amount, ramsize))
{
  XBT_DEBUG("Create VM %s", get_cname());

  /* Currently, a VM uses the network resource of its physical host */
  set_netpoint(physical_host->get_netpoint());

  // Create a VCPU for this VM
  std::vector<double> speeds;
  for (int i = 0; i < physical_host->get_pstate_count(); i++)
    speeds.push_back(physical_host->get_pstate_speed(i));

  surf_cpu_model_vm->create_cpu(this, speeds, core_amount);
  if (physical_host->get_pstate() != 0)
    set_pstate(physical_host->get_pstate());

  // Real hosts are (only) created through NetZone::create_host(), and this where the on_creation signal is fired.
  // VMs are created directly, thus firing the signal here. The right solution is probably to separate Host and VM.
  simgrid::s4u::Host::on_creation(*this);
}

VirtualMachine::~VirtualMachine()
{
  on_destruction(*this);

  XBT_DEBUG("destroy %s", get_cname());

  /* Don't free these things twice: they are the ones of my physical host */
  set_netpoint(nullptr);
}

void VirtualMachine::start()
{
  on_start(*this);

  kernel::actor::simcall([this]() {
    vm::VmHostExt::ensureVmExtInstalled();

    Host* pm = this->pimpl_vm_->get_physical_host();
    if (pm->extension<vm::VmHostExt>() == nullptr)
      pm->extension_set(new vm::VmHostExt());

    long pm_ramsize   = pm->extension<vm::VmHostExt>()->ramsize;
    int pm_overcommit = pm->extension<vm::VmHostExt>()->overcommit;
    long vm_ramsize   = this->get_ramsize();

    if (pm_ramsize && not pm_overcommit) { /* Only verify that we don't overcommit on need */
      /* Retrieve the memory occupied by the VMs on that host. Yep, we have to traverse all VMs of all hosts for that */
      long total_ramsize_of_vms = 0;
      for (VirtualMachine* const& ws_vm : vm::VirtualMachineImpl::allVms_)
        if (pm == ws_vm->get_pm())
          total_ramsize_of_vms += ws_vm->get_ramsize();

      if (vm_ramsize > pm_ramsize - total_ramsize_of_vms) {
        XBT_WARN("cannot start %s@%s due to memory shortage: vm_ramsize %ld, free %ld, pm_ramsize %ld (bytes).",
                 this->get_cname(), pm->get_cname(), vm_ramsize, pm_ramsize - total_ramsize_of_vms, pm_ramsize);
        throw VmFailureException(XBT_THROW_POINT,
                                 xbt::string_printf("Memory shortage on host '%s', VM '%s' cannot be started",
                                                    pm->get_cname(), this->get_cname()));
      }
    }

    this->pimpl_vm_->set_state(VirtualMachine::state::RUNNING);
  });

  on_started(*this);
}

void VirtualMachine::suspend()
{
  on_suspend(*this);
  kernel::actor::ActorImpl* issuer = kernel::actor::ActorImpl::self();
  kernel::actor::simcall([this, issuer]() { pimpl_vm_->suspend(issuer); });
}

void VirtualMachine::resume()
{
  pimpl_vm_->resume();
  on_resume(*this);
}

void VirtualMachine::shutdown()
{
  kernel::actor::ActorImpl* issuer = kernel::actor::ActorImpl::self();
  kernel::actor::simcall([this, issuer]() { pimpl_vm_->shutdown(issuer); });
  on_shutdown(*this);
}

void VirtualMachine::destroy()
{
  /* First, terminate all processes on the VM if necessary */
  shutdown();

  /* Then, destroy the VM object */
  Host::destroy();
}

simgrid::s4u::Host* VirtualMachine::get_pm() const
{
  return pimpl_vm_->get_physical_host();
}

VirtualMachine* VirtualMachine::set_pm(simgrid::s4u::Host* pm)
{
  kernel::actor::simcall([this, pm]() { pimpl_vm_->set_physical_host(pm); });
  return this;
}

VirtualMachine::state VirtualMachine::get_state() const
{
  return kernel::actor::simcall([this]() { return pimpl_vm_->get_state(); });
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
VirtualMachine* VirtualMachine::set_bound(double bound)
{
  kernel::actor::simcall([this, bound]() { pimpl_vm_->set_bound(bound); });
  return this;
}

} // namespace simgrid
} // namespace s4u

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
  return new simgrid::s4u::VirtualMachine(name, pm, coreAmount);
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
int sg_vm_is_created(sg_vm_t vm)
{
  return vm->get_state() == simgrid::s4u::VirtualMachine::state::CREATED;
}

/** @brief Returns whether the given VM is currently running */
int sg_vm_is_running(sg_vm_t vm)
{
  return vm->get_state() == simgrid::s4u::VirtualMachine::state::RUNNING;
}

/** @brief Returns whether the given VM is currently suspended, not running. */
int sg_vm_is_suspended(sg_vm_t vm)
{
  return vm->get_state() == simgrid::s4u::VirtualMachine::state::SUSPENDED;
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
 * No extra delay occurs by default. You may let your actor sleep by a specific amount to simulate any extra delay that you want.
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
