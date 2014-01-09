/*
 * vm_workstation.cpp
 *
 *  Created on: Nov 12, 2013
 *      Author: bedaride
 */
#include "vm_workstation_interface.hpp"
#include "cpu_cas01.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_vm_workstation, surf,
                                "Logging specific to the SURF VM workstation module");

WorkstationVMModelPtr surf_vm_workstation_model = NULL;

/*********
 * Model *
 *********/

WorkstationVMModel::WorkstationVMModel() : WorkstationModel("Virtual Workstation") {
  p_cpuModel = surf_cpu_model_vm;
}

/************
 * Resource *
 ************/

/*
 * A physical host does not disapper in the current SimGrid code, but a VM may
 * disapper during a simulation.
 */
WorkstationVM::~WorkstationVM()
{
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

