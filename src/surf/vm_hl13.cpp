/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <algorithm>

#include <simgrid/host.h>

#include "cpu_cas01.hpp"
#include "vm_hl13.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_vm);

void surf_vm_model_init_HL13(void){
  if (surf_cpu_model_vm) {
    surf_vm_model = new simgrid::surf::VMHL13Model();
    simgrid::surf::Model *model = surf_vm_model;
    xbt_dynar_push(all_existing_models, &model);
  }
}

namespace simgrid {
namespace surf {

/*********
 * Model *
 *********/

VMHL13Model::VMHL13Model() : VMModel() {}

void VMHL13Model::updateActionsState(double /*now*/, double /*delta*/) {}

/* ind means ''indirect'' that this is a reference on the whole dict_elm
 * structure (i.e not on the surf_resource_private infos) */

VirtualMachine *VMHL13Model::createVM(const char *name, sg_host_t host_PM)
{
  VirtualMachine* vm = new VMHL13(this, name, NULL, host_PM);
  VMCreatedCallbacks(vm);
  return vm;
}

/* In the real world, processes on the guest operating system will be somewhat
 * degraded due to virtualization overhead. The total CPU share that these
 * processes get is smaller than that of the VM process gets on a host
 * operating system. */
// const double virt_overhead = 0.95;
const double virt_overhead = 1;

double VMHL13Model::next_occuring_event(double now)
{
  /* TODO: update action's cost with the total cost of processes on the VM. */

  /* 1. Now we know how many resource should be assigned to each virtual
   * machine. We update constraints of the virtual machine layer.
   *
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
   *
   **/

  /* iterate for all virtual machines */
  for (VMModel::vm_list_t::iterator iter =
         VMModel::ws_vms.begin();
       iter !=  VMModel::ws_vms.end(); ++iter) {

    VirtualMachine *ws_vm = &*iter;
    Cpu *cpu = ws_vm->p_cpu;
    xbt_assert(cpu, "cpu-less host");

    double solved_value = ws_vm->p_action->getVariable()->value;
    XBT_DEBUG("assign %f to vm %s @ pm %s", solved_value,
        ws_vm->getName(), ws_vm->p_hostPM->name().c_str());

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

VMHL13::VMHL13(VMModel *model, const char* name, xbt_dict_t props, sg_host_t host_PM)
 : VirtualMachine(model, name, props, host_PM)
{
  /* Currently, we assume a VM has no storage. */
  p_storage = NULL;

  /* Currently, a VM uses the network resource of its physical host. In
   * host_lib, this network resource object is referred from two different keys.
   * When deregistering the reference that points the network resource object
   * from the VM name, we have to make sure that the system does not call the
   * free callback for the network resource object. The network resource object
   * is still used by the physical machine. */
  sg_host_t host_VM = sg_host_by_name_or_create(name);
  host_VM->pimpl_netcard = host_PM->pimpl_netcard;

  p_vm_state = SURF_VM_STATE_CREATED;

  // //// CPU  RELATED STUFF ////
  // Roughly, create a vcpu resource by using the values of the sub_cpu one.
  CpuCas01 *sub_cpu = static_cast<CpuCas01*>(host_PM->pimpl_cpu);

  p_cpu = surf_cpu_model_vm->createCpu(host_VM, // the machine hosting the VM
      sub_cpu->getSpeedPeakList(),        // host->power_peak,
      NULL,                       // host->power_trace,
      1,                          // host->core_amount,
      NULL);                      // host->state_trace,
  if (sub_cpu->getPState() != 0)
    p_cpu->setPState(sub_cpu->getPState());

  /* We create cpu_action corresponding to a VM process on the host operating system. */
  /* FIXME: TODO: we have to periodically input GUESTOS_NOISE to the system? how ? */
  p_action = sub_cpu->execution_start(0);

  XBT_VERB("Create VM(%s)@PM(%s) with %ld mounted disks",
    name, p_hostPM->name().c_str(), xbt_dynar_length(p_storage));
}

void VMHL13::suspend()
{
  p_action->suspend();
  p_vm_state = SURF_VM_STATE_SUSPENDED;
}

void VMHL13::resume()
{
  p_action->resume();
  p_vm_state = SURF_VM_STATE_RUNNING;
}

void VMHL13::save()
{
  p_vm_state = SURF_VM_STATE_SAVING;

  /* FIXME: do something here */
  p_action->suspend();
  p_vm_state = SURF_VM_STATE_SAVED;
}

void VMHL13::restore()
{
  p_vm_state = SURF_VM_STATE_RESTORING;

  /* FIXME: do something here */
  p_action->resume();
  p_vm_state = SURF_VM_STATE_RUNNING;
}

/*
 * Update the physical host of the given VM
 */
void VMHL13::migrate(sg_host_t host_dest)
{
   HostImpl *surfHost_dst = host_dest->extension<HostImpl>();
   const char *vm_name = getName();
   const char *pm_name_src = p_hostPM->name().c_str();
   const char *pm_name_dst = surfHost_dst->getName();

   /* update net_elm with that of the destination physical host */
   sg_host_by_name(vm_name)->pimpl_netcard = sg_host_by_name(pm_name_dst)->pimpl_netcard;

   p_hostPM = host_dest;

   /* Update vcpu's action for the new pm */
   {
     /* create a cpu action bound to the pm model at the destination. */
     CpuAction *new_cpu_action = static_cast<CpuAction*>(host_dest->pimpl_cpu->execution_start(0));

     e_surf_action_state_t state = p_action->getState();
     if (state != SURF_ACTION_DONE)
       XBT_CRITICAL("FIXME: may need a proper handling, %d", state);
     if (p_action->getRemainsNoUpdate() > 0)
       XBT_CRITICAL("FIXME: need copy the state(?), %f", p_action->getRemainsNoUpdate());

     /* keep the bound value of the cpu action of the VM. */
     double old_bound = p_action->getBound();
     if (old_bound != 0) {
       XBT_DEBUG("migrate VM(%s): set bound (%f) at %s", vm_name, old_bound, pm_name_dst);
       new_cpu_action->setBound(old_bound);
     }

     XBT_ATTRIB_UNUSED int ret = p_action->unref();
     xbt_assert(ret == 1, "Bug: some resource still remains");

     p_action = new_cpu_action;
   }

   XBT_DEBUG("migrate VM(%s): change PM (%s to %s)", vm_name, pm_name_src, pm_name_dst);
}

void VMHL13::setBound(double bound){
 p_action->setBound(bound);
}

void VMHL13::setAffinity(Cpu *cpu, unsigned long mask){
 p_action->setAffinity(cpu, mask);
}


}
}
