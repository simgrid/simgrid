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

surf_callback(void, simgrid::surf::VirtualMachine*) VMCreatedCallbacks;
surf_callback(void, simgrid::surf::VirtualMachine*) VMDestructedCallbacks;
surf_callback(void, simgrid::surf::VirtualMachine*) VMStateChangedCallbacks;

/*********
 * Model *
 *********/

VMModel::vm_list_t VMModel::ws_vms;

/************
 * Resource *
 ************/

VirtualMachine::VirtualMachine(Model *model, const char *name, xbt_dict_t props,
		        RoutingEdge *netElm, Cpu *cpu)
: Host(model, name, props, NULL, netElm, cpu)
{
  VMModel::ws_vms.push_back(*this);
  simgrid::Host::get_host(name)->set_facet(SURF_HOST_LEVEL, this);
}

/*
 * A physical host does not disappear in the current SimGrid code, but a VM may
 * disappear during a simulation.
 */
VirtualMachine::~VirtualMachine()
{
  surf_callback_emit(VMDestructedCallbacks, this);
  VMModel::ws_vms.erase(VMModel::vm_list_t::s_iterator_to(*this));
}

void VirtualMachine::setState(e_surf_resource_state_t state){
  Resource::setState(state);
  surf_callback_emit(VMStateChangedCallbacks, this);
}

/*
 * A surf level object will be useless in the upper layer. Returning the
 * dict_elm of the host.
 **/
sg_host_t VirtualMachine::getPm()
{
  return simgrid::Host::find_host(p_subWs->getName());
}

/**********
 * Action *
 **********/

//FIME:: handle action cancel

}
}
