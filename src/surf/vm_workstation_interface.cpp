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


WorkstationVMLmm::WorkstationVMLmm(RoutingEdgePtr netElm, CpuPtr cpu)
  : WorkstationVM(netElm, cpu)
  , WorkstationLmm()
{}

/*
 * A physical host does not disapper in the current SimGrid code, but a VM may
 * disapper during a simulation.
 */
WorkstationVM::~WorkstationVM()
{
  /* ind_phys_workstation equals to smx_host_t */
  surf_resource_t ind_vm_workstation = xbt_lib_get_elm_or_null(host_lib, getName());

  /* Before clearing the entries in host_lib, we have to pick up resources. */
  CpuCas01LmmPtr cpu = dynamic_cast<CpuCas01LmmPtr>(
                    static_cast<ResourcePtr>(
       	              surf_cpu_resource_priv(ind_vm_workstation)));

  /* We deregister objects from host_lib, without invoking the freeing callback
   * of each level.
   *
   * Do not call xbt_lib_remove() here. It deletes all levels of the key,
   * including MSG_HOST_LEVEL and others. We should unregister only what we know.
   */
  xbt_lib_unset(host_lib, getName(), SURF_CPU_LEVEL, 0);
  xbt_lib_unset(host_lib, getName(), ROUTING_HOST_LEVEL, 0);
  xbt_lib_unset(host_lib, getName(), SURF_WKS_LEVEL, 0);

  /* TODO: comment out when VM stroage is implemented. */
  // xbt_lib_unset(host_lib, name, SURF_STORAGE_LEVEL, 0);


  /* Free the cpu_action of the VM. */
  int ret = p_action->unref();
  xbt_assert(ret == 1, "Bug: some resource still remains");

  /* Free the cpu resource of the VM. If using power_trace, we will have to */
  delete cpu;
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

