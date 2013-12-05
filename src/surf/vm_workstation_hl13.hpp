/*
 * vm_workstation.hpp
 *
 *  Created on: Nov 12, 2013
 *      Author: bedaride
 */
#include "vm_workstation_interface.hpp"
#include "workstation_clm03.hpp"

#ifndef VM_WORKSTATION_HPP_
#define VM_WORKSTATION_HPP_

#define GUESTOS_NOISE 100 // This value corresponds to the cost of the global action associated to the VM
                          // It corresponds to the cost of a VM running no tasks.

void surf_vm_workstation_model_init(void);

/***********
 * Classes *
 ***********/

class WorkstationVMHL13Model;
typedef WorkstationVMHL13Model *WorkstationVMHL13ModelPtr;

class WorkstationVMHL13Lmm;
typedef WorkstationVMHL13Lmm *WorkstationVMHL13LmmPtr;

/*********
 * Tools *
 *********/

/*********
 * Model *
 *********/
class WorkstationVMHL13Model : public WorkstationVMModel, public WorkstationCLM03Model {
public:
  WorkstationVMHL13Model();
  ~WorkstationVMHL13Model(){};
  void createResource(const char *name, void *ind_phys_workstation);
  double shareResources(double now);
  void adjustWeightOfDummyCpuActions() {};
  xbt_dynar_t getRoute(WorkstationPtr src, WorkstationPtr dst);
  ActionPtr communicate(WorkstationPtr src, WorkstationPtr dst, double size, double rate);
};

/************
 * Resource *
 ************/

class WorkstationVMHL13Lmm : public WorkstationVMLmm, public WorkstationCLM03Lmm {
public:
  WorkstationVMHL13Lmm(WorkstationVMModelPtr model, const char* name, xbt_dict_t props, surf_resource_t ind_phys_workstation);
  ~WorkstationVMHL13Lmm();

  void suspend();
  void resume();

  void save();
  void restore();

  void migrate(surf_resource_t ind_dst_pm);

  e_surf_resource_state_t getState();
  void setState(e_surf_resource_state_t state);

  surf_resource_t getPm(); // will be vm_ws_get_pm()

  void setBound(double bound);
  void setAffinity(CpuLmmPtr cpu, unsigned long mask);

  //FIXME: remove
  void updateState(tmgr_trace_event_t event_type, double value, double date) {
	WorkstationCLM03Lmm::updateState(event_type, value, date);
  }
  bool isUsed() {
    return WorkstationCLM03Lmm::isUsed();
  }
  xbt_dict_t getProperties() {
    return WorkstationCLM03Lmm::getProperties();
  }
  ActionPtr execute(double size);

};

/**********
 * Action *
 **********/

#endif /* VM_WORKSTATION_HPP_ */
