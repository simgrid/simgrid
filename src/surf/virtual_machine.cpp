/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/signal.hpp>

#include "cpu_cas01.hpp"
#include "virtual_machine.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_vm, surf, "Logging specific to the SURF VM module");

simgrid::surf::VMModel *surf_vm_model = nullptr;

void surf_vm_model_init_HL13(){
  if (surf_cpu_model_vm) {
    surf_vm_model = new simgrid::surf::VMModel();
    all_existing_models->push_back(surf_vm_model);
  }
}

namespace simgrid {
namespace surf {


/*************
 * Callbacks *
 *************/

simgrid::xbt::signal<void(simgrid::surf::VirtualMachine*)> onVmCreation;
simgrid::xbt::signal<void(simgrid::surf::VirtualMachine*)> onVmDestruction;
simgrid::xbt::signal<void(simgrid::surf::VirtualMachine*)> onVmStateChange;

/*********
 * Model *
 *********/

std::deque<VirtualMachine*> VirtualMachine::allVms_;

s4u::Host *VMModel::createVM(const char *name, sg_host_t host_PM)
{
  VirtualMachine* vm = new VirtualMachine(this, name, host_PM);
  onVmCreation(vm);
  return vm->piface_;
}

/* In the real world, processes on the guest operating system will be somewhat degraded due to virtualization overhead.
 * The total CPU share these processes get is smaller than that of the VM process gets on a host operating system. */
// const double virt_overhead = 0.95;
const double virt_overhead = 1;

double VMModel::next_occuring_event(double now)
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
   * X1 must be passed to the virtual machine laye as a constraint value.
   **/

  /* iterate for all virtual machines */
  for (VirtualMachine *ws_vm : VirtualMachine::allVms_) {
    Cpu *cpu = ws_vm->cpu_;
    xbt_assert(cpu, "cpu-less host");

    double solved_value = ws_vm->action_->getVariable()->value;
    XBT_DEBUG("assign %f to vm %s @ pm %s", solved_value, ws_vm->getName(), ws_vm->getPm()->name().c_str());

    // TODO: check lmm_update_constraint_bound() works fine instead of the below manual substitution.
    // cpu_cas01->constraint->bound = solved_value;
    xbt_assert(cpu->getModel() == surf_cpu_model_vm);
    lmm_system_t vcpu_system = cpu->getModel()->getMaxminSystem();
    lmm_update_constraint_bound(vcpu_system, cpu->getConstraint(), virt_overhead * solved_value);
  }

  /* 2. Calculate resource share at the virtual machine layer. */
  adjustWeightOfDummyCpuActions();

  /* 3. Ready. Get the next occuring event */
  return surf_cpu_model_vm->next_occuring_event(now);
}


/************
 * Resource *
 ************/

VirtualMachine::VirtualMachine(HostModel *model, const char *name, simgrid::s4u::Host *host_PM)
: HostImpl(model, name, nullptr, nullptr, nullptr)
, hostPM_(host_PM)
{
  /* Register this VM to the list of all VMs */
  allVms_.push_back(this);
  piface_ = simgrid::s4u::Host::by_name_or_create(name);
  piface_->extension_set<simgrid::surf::HostImpl>(this);

  /* Currently, we assume a VM has no storage. */
  storage_ = nullptr;

  /* Currently, a VM uses the network resource of its physical host. In
   * host_lib, this network resource object is referred from two different keys.
   * When deregistering the reference that points the network resource object
   * from the VM name, we have to make sure that the system does not call the
   * free callback for the network resource object. The network resource object
   * is still used by the physical machine. */
  sg_host_t host_VM = simgrid::s4u::Host::by_name_or_create(name);
  host_VM->pimpl_netcard = host_PM->pimpl_netcard;

  vmState_ = SURF_VM_STATE_CREATED;

  // //// CPU  RELATED STUFF ////
  // Roughly, create a vcpu resource by using the values of the sub_cpu one.
  CpuCas01 *sub_cpu = dynamic_cast<CpuCas01*>(host_PM->pimpl_cpu);

  cpu_ = surf_cpu_model_vm->createCpu(host_VM, // the machine hosting the VM
      sub_cpu->getSpeedPeakList(), 1 /*cores*/);
  if (sub_cpu->getPState() != 0)
    cpu_->setPState(sub_cpu->getPState());

  /* We create cpu_action corresponding to a VM process on the host operating system. */
  /* FIXME: TODO: we have to periodically input GUESTOS_NOISE to the system? how ? */
  action_ = sub_cpu->execution_start(0);

  XBT_VERB("Create VM(%s)@PM(%s) with %ld mounted disks", name, hostPM_->name().c_str(), xbt_dynar_length(storage_));
}

/*
 * A physical host does not disappear in the current SimGrid code, but a VM may disappear during a simulation.
 */
VirtualMachine::~VirtualMachine()
{
  onVmDestruction(this);
  allVms_.erase( find(allVms_.begin(), allVms_.end(), this) );

  /* Free the cpu_action of the VM. */
  XBT_ATTRIB_UNUSED int ret = action_->unref();
  xbt_assert(ret == 1, "Bug: some resource still remains");

  delete cpu_;
}

e_surf_vm_state_t VirtualMachine::getState() {
  return vmState_;
}

void VirtualMachine::setState(e_surf_vm_state_t state) {
  vmState_ = state;
}
void VirtualMachine::turnOn() {
  if (isOff()) {
    Resource::turnOn();
    onVmStateChange(this);
  }
}
void VirtualMachine::turnOff() {
  if (isOn()) {
    Resource::turnOff();
    onVmStateChange(this);
  }
}
void VirtualMachine::suspend()
{
  action_->suspend();
  vmState_ = SURF_VM_STATE_SUSPENDED;
}

void VirtualMachine::resume()
{
  action_->resume();
  vmState_ = SURF_VM_STATE_RUNNING;
}

void VirtualMachine::save()
{
  vmState_ = SURF_VM_STATE_SAVING;
  action_->suspend();
  vmState_ = SURF_VM_STATE_SAVED;
}

void VirtualMachine::restore()
{
  vmState_ = SURF_VM_STATE_RESTORING;
  action_->resume();
  vmState_ = SURF_VM_STATE_RUNNING;
}

/** @brief returns the physical machine on which the VM is running **/
sg_host_t VirtualMachine::getPm() {
  return hostPM_;
}

/* Update the physical host of the given VM */
void VirtualMachine::migrate(sg_host_t host_dest)
{
   HostImpl *surfHost_dst = host_dest->extension<HostImpl>();
   const char *vm_name = getName();
   const char *pm_name_src = hostPM_->name().c_str();
   const char *pm_name_dst = surfHost_dst->getName();

   /* update net_elm with that of the destination physical host */
   sg_host_by_name(vm_name)->pimpl_netcard = sg_host_by_name(pm_name_dst)->pimpl_netcard;

   hostPM_ = host_dest;

   /* Update vcpu's action for the new pm */
   /* create a cpu action bound to the pm model at the destination. */
   CpuAction *new_cpu_action = static_cast<CpuAction*>(host_dest->pimpl_cpu->execution_start(0));

   Action::State state = action_->getState();
   if (state != Action::State::done)
     XBT_CRITICAL("FIXME: may need a proper handling, %d", static_cast<int>(state));
   if (action_->getRemainsNoUpdate() > 0)
     XBT_CRITICAL("FIXME: need copy the state(?), %f", action_->getRemainsNoUpdate());

   /* keep the bound value of the cpu action of the VM. */
   double old_bound = action_->getBound();
   if (old_bound != 0) {
     XBT_DEBUG("migrate VM(%s): set bound (%f) at %s", vm_name, old_bound, pm_name_dst);
     new_cpu_action->setBound(old_bound);
   }

   XBT_ATTRIB_UNUSED int ret = action_->unref();
   xbt_assert(ret == 1, "Bug: some resource still remains");

   action_ = new_cpu_action;

   XBT_DEBUG("migrate VM(%s): change PM (%s to %s)", vm_name, pm_name_src, pm_name_dst);
}

void VirtualMachine::setBound(double bound){
 action_->setBound(bound);
}


}
}
