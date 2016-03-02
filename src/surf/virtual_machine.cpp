/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "cpu_cas01.hpp"
#include "virtual_machine.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_vm, surf,
                                "Logging specific to the SURF VM module");

simgrid::surf::VMModel *surf_vm_model = NULL;

namespace simgrid {
namespace surf {

/*************
 * Callbacks *
 *************/

simgrid::xbt::signal<void(simgrid::surf::VirtualMachine*)> VMCreatedCallbacks;
simgrid::xbt::signal<void(simgrid::surf::VirtualMachine*)> VMDestructedCallbacks;
simgrid::xbt::signal<void(simgrid::surf::VirtualMachine*)> VMStateChangedCallbacks;

/*********
 * Model *
 *********/

VMModel::vm_list_t VMModel::ws_vms;

/************
 * Resource *
 ************/

VirtualMachine::VirtualMachine(HostModel *model, const char *name, xbt_dict_t props, simgrid::s4u::Host *hostPM)
: HostImpl(model, name, props, NULL, NULL, NULL)
, p_hostPM(hostPM)
{
  VMModel::ws_vms.push_back(*this);
  simgrid::s4u::Host::by_name_or_create(name)->extension_set<simgrid::surf::HostImpl>(this);
}

/*
 * A physical host does not disappear in the current SimGrid code, but a VM may
 * disappear during a simulation.
 */
VirtualMachine::~VirtualMachine()
{
  VMDestructedCallbacks(this);
  VMModel::ws_vms.erase(VMModel::vm_list_t::s_iterator_to(*this));
  /* Free the cpu_action of the VM. */
  XBT_ATTRIB_UNUSED int ret = p_action->unref();
  xbt_assert(ret == 1, "Bug: some resource still remains");
}

e_surf_vm_state_t VirtualMachine::getState() {
  return p_vm_state;
}

void VirtualMachine::setState(e_surf_vm_state_t state) {
  p_vm_state = state;
}
void VirtualMachine::turnOn() {
  if (isOff()) {
    Resource::turnOn();
    VMStateChangedCallbacks(this);
  }
}
void VirtualMachine::turnOff() {
  if (isOn()) {
    Resource::turnOff();
    VMStateChangedCallbacks(this);
  }
}

/** @brief returns the physical machine on which the VM is running **/
sg_host_t VirtualMachine::getPm() {
  return p_hostPM;
}

}
}
