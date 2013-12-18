/*
 * vm_workstation.hpp
 *
 *  Created on: Nov 12, 2013
 *      Author: bedaride
 */
#include "workstation_interface.hpp"

#ifndef VM_WORKSTATION_INTERFACE_HPP_
#define VM_WORKSTATION_INTERFACE_HPP_

#define GUESTOS_NOISE 100 // This value corresponds to the cost of the global action associated to the VM
                          // It corresponds to the cost of a VM running no tasks.

void surf_vm_workstation_model_init(void);

/***********
 * Classes *
 ***********/

class WorkstationVMModel;
typedef WorkstationVMModel *WorkstationVMModelPtr;

class WorkstationVM;
typedef WorkstationVM *WorkstationVMPtr;

class WorkstationVMLmm;
typedef WorkstationVMLmm *WorkstationVMLmmPtr;

/*********
 * Model *
 *********/

class WorkstationVMModel : virtual public WorkstationModel {
public:
  WorkstationVMModel();
  ~WorkstationVMModel(){};
  virtual void createResource(const char *name, void *ind_phys_workstation)=0;
  void adjustWeightOfDummyCpuActions() {};
};

/************
 * Resource *
 ************/

class WorkstationVM : public Workstation {
public:
  WorkstationVM(ModelPtr model, const char *name, xbt_dict_t props,
		        RoutingEdgePtr netElm, CpuPtr cpu)
  : Workstation(model, name, props, NULL, netElm, cpu) {}
  ~WorkstationVM();

  virtual void suspend()=0;
  virtual void resume()=0;

  virtual void save()=0;
  virtual void restore()=0;

  virtual void migrate(surf_resource_t ind_vm_ws_dest)=0; // will be vm_ws_migrate()

  virtual surf_resource_t getPm()=0; // will be vm_ws_get_pm()

  virtual void setBound(double bound)=0;
  virtual void setAffinity(CpuPtr cpu, unsigned long mask)=0;

  /* The workstation object of the lower layer */
  CpuActionPtr p_action;
  WorkstationPtr p_subWs;  // Pointer to the ''host'' OS
  e_surf_vm_state_t p_currentState;
};


/**********
 * Action *
 **********/

#endif /* VM_WORKSTATION_INTERFACE_HPP_ */
