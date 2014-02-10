/* Copyright (c) 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "vm_workstation_interface.hpp"
#include "cpu_cas01.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_vm_workstation, surf,
                                "Logging specific to the SURF VM workstation module");

WorkstationVMModelPtr surf_vm_workstation_model = NULL;

/*************
 * Callbacks *
 *************/

surf_callback(void, WorkstationVMPtr) workstationVMCreatedCallbacks;
surf_callback(void, WorkstationVMPtr) workstationVMDestructedCallbacks;
surf_callback(void, WorkstationVMPtr) workstationVMStateChangedCallbacks;

/*********
 * Model *
 *********/

WorkstationVMModel::WorkstationVMModel() : WorkstationModel("Virtual Workstation") {
  p_cpuModel = surf_cpu_model_vm;
}

WorkstationVMModel::vm_list_t WorkstationVMModel::ws_vms;

/************
 * Resource *
 ************/

WorkstationVM::WorkstationVM(ModelPtr model, const char *name, xbt_dict_t props,
		        RoutingEdgePtr netElm, CpuPtr cpu)
: Workstation(model, name, props, NULL, netElm, cpu)
{
  WorkstationVMModel::ws_vms.push_back(*this);
  surf_callback_emit(workstationVMCreatedCallbacks, this);
}

/*
 * A physical host does not disapper in the current SimGrid code, but a VM may
 * disapper during a simulation.
 */
WorkstationVM::~WorkstationVM()
{
  surf_callback_emit(workstationVMDestructedCallbacks, this);
  WorkstationVMModel::ws_vms.erase(WorkstationVMModel::
                                   vm_list_t::s_iterator_to(*this));
}

void WorkstationVM::setState(e_surf_resource_state_t state){
  Resource::setState(state);
  surf_callback_emit(workstationVMStateChangedCallbacks, this);
}

/*
 * A surf level object will be useless in the upper layer. Returing the
 * dict_elm of the host.
 **/
surf_resource_t WorkstationVM::getPm()
{
  return xbt_lib_get_elm_or_null(host_lib, p_subWs->getName());
}

/**********
 * Action *
 **********/

//FIME:: handle action cancel

