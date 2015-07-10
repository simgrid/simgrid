/* Copyright (c) 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "cpu_cas01.hpp"
#include "vm_interface.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_vm, surf,
                                "Logging specific to the SURF VM module");

VMModelPtr surf_vm_model = NULL;

/*************
 * Callbacks *
 *************/

surf_callback(void, VMPtr) VMCreatedCallbacks;
surf_callback(void, VMPtr) VMDestructedCallbacks;
surf_callback(void, VMPtr) VMStateChangedCallbacks;

/*********
 * Model *
 *********/

VMModel::VMModel() : HostModel("Virtual Machine") {
  p_cpuModel = surf_cpu_model_vm;
}

VMModel::vm_list_t VMModel::ws_vms;

/************
 * Resource *
 ************/

VM::VM(ModelPtr model, const char *name, xbt_dict_t props,
		        RoutingEdgePtr netElm, CpuPtr cpu)
: Host(model, name, props, NULL, netElm, cpu)
{
  VMModel::ws_vms.push_back(*this);
  surf_callback_emit(VMCreatedCallbacks, this);
}

/*
 * A physical host does not disapper in the current SimGrid code, but a VM may
 * disapper during a simulation.
 */
VM::~VM()
{
  surf_callback_emit(VMDestructedCallbacks, this);
  VMModel::ws_vms.erase(VMModel::
                                   vm_list_t::s_iterator_to(*this));
}

void VM::setState(e_surf_resource_state_t state){
  Resource::setState(state);
  surf_callback_emit(VMStateChangedCallbacks, this);
}

/*
 * A surf level object will be useless in the upper layer. Returing the
 * dict_elm of the host.
 **/
surf_resource_t VM::getPm()
{
  return xbt_lib_get_elm_or_null(host_lib, p_subWs->getName());
}

/**********
 * Action *
 **********/

//FIME:: handle action cancel

