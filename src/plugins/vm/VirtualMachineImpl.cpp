/* Copyright (c) 2013-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/plugins/vm/VirtualMachineImpl.hpp"
#include "src/simix/ActorImpl.hpp"
#include "src/simix/smx_host_private.hpp"

#include <xbt/asserts.h> // xbt_log_no_loc

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_vm, surf, "Logging specific to the SURF VM module");

simgrid::vm::VMModel* surf_vm_model = nullptr;

void surf_vm_model_init_HL13()
{
  if (surf_cpu_model_vm) {
    surf_vm_model = new simgrid::vm::VMModel();
    all_existing_models->push_back(surf_vm_model);
  }
}


/*************
 * Callbacks *
 *************/

simgrid::xbt::signal<void(simgrid::vm::VirtualMachineImpl*)> simgrid::vm::VirtualMachineImpl::onVmCreation;
simgrid::xbt::signal<void(simgrid::vm::VirtualMachineImpl*)> simgrid::vm::VirtualMachineImpl::onVmDestruction;
simgrid::xbt::signal<void(simgrid::vm::VirtualMachineImpl*)> simgrid::vm::VirtualMachineImpl::onVmStateChange;

namespace simgrid {
namespace vm {
/*********
 * Model *
 *********/

std::deque<s4u::VirtualMachine*> VirtualMachineImpl::allVms_;

/* In the real world, processes on the guest operating system will be somewhat degraded due to virtualization overhead.
 * The total CPU share these processes get is smaller than that of the VM process gets on a host operating system.
 * FIXME: add a configuration flag for this
 */
const double virt_overhead = 1; // 0.95

static void hostStateChange(s4u::Host& host)
{
  if (host.isOff()) { // just turned off.
    std::vector<s4u::VirtualMachine*> trash;
    /* Find all VMs living on that host */
    for (s4u::VirtualMachine* const& vm : VirtualMachineImpl::allVms_)
      if (vm->getPm() == &host)
        trash.push_back(vm);
    for (s4u::VirtualMachine* vm : trash)
      vm->pimpl_vm_->shutdown(SIMIX_process_self());
  }
}
VMModel::VMModel()
{
  s4u::Host::onStateChange.connect(hostStateChange);
}

double VMModel::nextOccuringEvent(double now)
{
  /* TODO: update action's cost with the total cost of processes on the VM. */

  /* 1. Now we know how many resource should be assigned to each virtual
   * machine. We update constraints of the virtual machine layer.
   *
   * If we have two virtual machine (VM1 and VM2) on a physical machine (PM1).
   *     X1 + X2 = C       (Equation 1)
   * where
   *    the resource share of VM1: X1
   *    the resource share of VM2: X2
   *    the capacity of PM1: C
   *
   * Then, if we have two process (P1 and P2) on VM1.
   *     X1_1 + X1_2 = X1  (Equation 2)
   * where
   *    the resource share of P1: X1_1
   *    the resource share of P2: X1_2
   *    the capacity of VM1: X1
   *
   * Equation 1 was solved in the physical machine layer.
   * Equation 2 is solved in the virtual machine layer (here).
   * X1 must be passed to the virtual machine layer as a constraint value.
   **/

  /* iterate for all virtual machines */
  for (s4u::VirtualMachine* const& ws_vm : VirtualMachineImpl::allVms_) {
    surf::Cpu* cpu = ws_vm->pimpl_cpu;
    xbt_assert(cpu, "cpu-less host");

    double solved_value = ws_vm->pimpl_vm_->action_->getVariable()->get_value(); // this is X1 in comment above, what
                                                                                 // this VM got in the sharing on the PM
    XBT_DEBUG("assign %f to vm %s @ pm %s", solved_value, ws_vm->getCname(), ws_vm->pimpl_vm_->getPm()->getCname());

    xbt_assert(cpu->model() == surf_cpu_model_vm);
    lmm_system_t vcpu_system = cpu->model()->getMaxminSystem();
    vcpu_system->update_constraint_bound(cpu->constraint(), virt_overhead * solved_value);
  }

  /* 2. Calculate resource share at the virtual machine layer. */
  ignoreEmptyVmInPmLMM();

  /* 3. Ready. Get the next occurring event */
  return surf_cpu_model_vm->nextOccuringEvent(now);
}

/************
 * Resource *
 ************/

VirtualMachineImpl::VirtualMachineImpl(simgrid::s4u::VirtualMachine* piface, simgrid::s4u::Host* host_PM,
                                       int coreAmount, size_t ramsize)
    : HostImpl(piface), hostPM_(host_PM), coreAmount_(coreAmount), ramsize_(ramsize)
{
  /* Register this VM to the list of all VMs */
  allVms_.push_back(piface);

  /* We create cpu_action corresponding to a VM process on the host operating system. */
  /* TODO: we have to periodically input GUESTOS_NOISE to the system? how ? */
  action_ = host_PM->pimpl_cpu->execution_start(0, coreAmount);

  XBT_VERB("Create VM(%s)@PM(%s)", piface->getCname(), hostPM_->getCname());
  onVmCreation(this);
}

/** @brief A physical host does not disappear in the current SimGrid code, but a VM may disappear during a simulation */
VirtualMachineImpl::~VirtualMachineImpl()
{
  onVmDestruction(this);
  /* I was already removed from the allVms set if the VM was destroyed cleanly */
  auto iter = find(allVms_.begin(), allVms_.end(), piface_);
  if (iter != allVms_.end())
    allVms_.erase(iter);

  /* Free the cpu_action of the VM. */
  XBT_ATTRIB_UNUSED int ret = action_->unref();
  xbt_assert(ret == 1, "Bug: some resource still remains");
}

e_surf_vm_state_t VirtualMachineImpl::getState()
{
  return vmState_;
}

void VirtualMachineImpl::setState(e_surf_vm_state_t state)
{
  vmState_ = state;
}
void VirtualMachineImpl::suspend(smx_actor_t issuer)
{
  if (isMigrating)
    THROWF(vm_error, 0, "Cannot suspend VM '%s': it is migrating", piface_->getCname());
  if (getState() != SURF_VM_STATE_RUNNING)
    THROWF(vm_error, 0, "Cannot suspend VM %s: it is not running.", piface_->getCname());
  if (issuer->host == piface_)
    THROWF(vm_error, 0, "Actor %s cannot suspend the VM %s in which it runs", issuer->getCname(), piface_->getCname());

  auto& process_list = piface_->extension<simgrid::simix::Host>()->process_list;
  XBT_DEBUG("suspend VM(%s), where %zu processes exist", piface_->getCname(), process_list.size());

  action_->suspend();

  for (auto& smx_process : process_list) {
    XBT_DEBUG("suspend %s", smx_process.name.c_str());
    smx_process.suspend(issuer);
  }

  XBT_DEBUG("suspend all processes on the VM done done");

  vmState_ = SURF_VM_STATE_SUSPENDED;
}

void VirtualMachineImpl::resume()
{
  if (getState() != SURF_VM_STATE_SUSPENDED)
    THROWF(vm_error, 0, "Cannot resume VM %s: it was not suspended", piface_->getCname());

  auto& process_list = piface_->extension<simgrid::simix::Host>()->process_list;
  XBT_DEBUG("Resume VM %s, containing %zu processes.", piface_->getCname(), process_list.size());

  action_->resume();

  for (auto& smx_process : process_list) {
    XBT_DEBUG("resume %s", smx_process.getCname());
    smx_process.resume();
  }

  vmState_ = SURF_VM_STATE_RUNNING;
}

/** @brief Power off a VM.
 *
 * All hosted processes will be killed, but the VM state is preserved on memory.
 * It can later be restarted.
 *
 * @param issuer the actor requesting the shutdown
 */
void VirtualMachineImpl::shutdown(smx_actor_t issuer)
{
  if (getState() != SURF_VM_STATE_RUNNING) {
    const char* stateName = "(unknown state)";
    switch (getState()) {
      case SURF_VM_STATE_CREATED:
        stateName = "created, but not yet started";
        break;
      case SURF_VM_STATE_SUSPENDED:
        stateName = "suspended";
        break;
      case SURF_VM_STATE_DESTROYED:
        stateName = "destroyed";
        break;
      default: /* SURF_VM_STATE_RUNNING or unexpected values */
        THROW_IMPOSSIBLE;
        break;
    }
    XBT_VERB("Shutting down the VM %s even if it's not running but %s", piface_->getCname(), stateName);
  }

  auto& process_list = piface_->extension<simgrid::simix::Host>()->process_list;
  XBT_DEBUG("shutdown VM %s, that contains %zu processes", piface_->getCname(), process_list.size());

  for (auto& smx_process : process_list) {
    XBT_DEBUG("kill %s@%s on behalf of %s which shutdown that VM.", smx_process.getCname(),
              smx_process.host->getCname(), issuer->getCname());
    SIMIX_process_kill(&smx_process, issuer);
  }

  setState(SURF_VM_STATE_DESTROYED);

  /* FIXME: we may have to do something at the surf layer, e.g., vcpu action */
}

/** @brief returns the physical machine on which the VM is running **/
s4u::Host* VirtualMachineImpl::getPm()
{
  return hostPM_;
}

/** @brief Change the physical host on which the given VM is running
 *
 * This is an instantaneous migration.
 */
void VirtualMachineImpl::setPm(s4u::Host* destination)
{
  const char* vm_name     = piface_->getCname();
  const char* pm_name_src = hostPM_->getCname();
  const char* pm_name_dst = destination->getCname();

  /* update net_elm with that of the destination physical host */
  piface_->pimpl_netpoint = destination->pimpl_netpoint;

  hostPM_ = destination;

  /* Update vcpu's action for the new pm */
  /* create a cpu action bound to the pm model at the destination. */
  surf::CpuAction* new_cpu_action =
      static_cast<surf::CpuAction*>(destination->pimpl_cpu->execution_start(0, this->coreAmount_));

  if (action_->getRemainsNoUpdate() > 0)
    XBT_CRITICAL("FIXME: need copy the state(?), %f", action_->getRemainsNoUpdate());

  /* keep the bound value of the cpu action of the VM. */
  double old_bound = action_->getBound();
  if (old_bound > 0) {
    XBT_DEBUG("migrate VM(%s): set bound (%f) at %s", vm_name, old_bound, pm_name_dst);
    new_cpu_action->setBound(old_bound);
  }

  XBT_ATTRIB_UNUSED int ret = action_->unref();
  xbt_assert(ret == 1, "Bug: some resource still remains");

  action_ = new_cpu_action;

  XBT_DEBUG("migrate VM(%s): change PM (%s to %s)", vm_name, pm_name_src, pm_name_dst);
}

void VirtualMachineImpl::setBound(double bound)
{
  action_->setBound(bound);
}

void VirtualMachineImpl::getParams(vm_params_t params)
{
  *params = params_;
}

void VirtualMachineImpl::setParams(vm_params_t params)
{
  /* may check something here. */
  params_ = *params;
}
}
}
