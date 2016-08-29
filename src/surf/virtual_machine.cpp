/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/signal.hpp>

#include "cpu_cas01.hpp"
#include "virtual_machine.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_vm, surf, "Logging specific to the SURF VM module");

simgrid::surf::VMModel *surf_vm_model = nullptr;

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

/************
 * Resource *
 ************/

VirtualMachine::VirtualMachine(HostModel *model, const char *name, simgrid::s4u::Host *hostPM)
: HostImpl(model, name, nullptr, nullptr, nullptr)
, hostPM_(hostPM)
{
  allVms_.push_back(this);
  piface_ = simgrid::s4u::Host::by_name_or_create(name);
  piface_->extension_set<simgrid::surf::HostImpl>(this);
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

/** @brief returns the physical machine on which the VM is running **/
sg_host_t VirtualMachine::getPm() {
  return hostPM_;
}

}
}
