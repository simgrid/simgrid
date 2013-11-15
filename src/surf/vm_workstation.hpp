/*
 * vm_workstation.hpp
 *
 *  Created on: Nov 12, 2013
 *      Author: bedaride
 */
#include "workstation.hpp"

#ifndef VM_WORKSTATION_HPP_
#define VM_WORKSTATION_HPP_

#define GUESTOS_NOISE 100 // This value corresponds to the cost of the global action associated to the VM
                          // It corresponds to the cost of a VM running no tasks.

void surf_vm_workstation_model_init(void);

/*
  void   (*create)  (const char *name, void *ind_phys_workstation); // First operation of the VM model
  void   (*destroy) (void *ind_vm_ws); // will be vm_ws_destroy(), which destroies the vm-workstation-specific data

  void   (*suspend) (void *ind_vm_ws);
  void   (*resume)  (void *ind_vm_ws);

  void   (*save)    (void *ind_vm_ws);
  void   (*restore) (void *ind_vm_ws);

  void   (*migrate) (void *ind_vm_ws, void *ind_vm_ws_dest); // will be vm_ws_migrate()

  int    (*get_state) (void *ind_vm_ws);
  void   (*set_state) (void *ind_vm_ws, int state);

  void * (*get_pm) (void *ind_vm_ws); // will be vm_ws_get_pm()

  void   (*set_vm_bound) (void *ind_vm_ws, double bound); // will be vm_ws_set_vm_bound()
  void   (*set_vm_affinity) (void *ind_vm_ws, void *ind_pm_ws, unsigned long mask); // will be vm_ws_set_vm_affinity()
*/

/***********
 * Classes *
 ***********/

class WorkstationVMModel;
typedef WorkstationVMModel *WorkstationVMModelPtr;

class WorkstationVM2013;
typedef WorkstationVM2013 *WorkstationVM2013Ptr;

class WorkstationVM2013Lmm;
typedef WorkstationVM2013Lmm *WorkstationVM2013LmmPtr;

/*********
 * Tools *
 *********/

/*********
 * Model *
 *********/
class WorkstationVMModel : public WorkstationModel {
public:
  WorkstationVMModel();
  ~WorkstationVMModel(){};
  void createResource(const char *name, void *ind_phys_workstation);
  double shareResources(double now);
  void adjustWeightOfDummyCpuActions() {};
};

/************
 * Resource *
 ************/
class WorkstationVM2013 : virtual public WorkstationCLM03 {
public:
  WorkstationVM2013(WorkstationVMModelPtr model, const char* name, xbt_dict_t props, RoutingEdgePtr netElm, CpuPtr cpu)
   : WorkstationCLM03(model, name, props, NULL, netElm, cpu) {};

  virtual void suspend()=0;
  virtual void resume()=0;

  virtual void save()=0;
  virtual void restore()=0;

  virtual void migrate(surf_resource_t ind_vm_ws_dest)=0; // will be vm_ws_migrate()

  virtual surf_resource_t getPm()=0; // will be vm_ws_get_pm()

  virtual void setBound(double bound)=0;
  virtual void setAffinity(CpuLmmPtr cpu, unsigned long mask)=0;

  /* The workstation object of the lower layer */
  WorkstationCLM03Ptr p_subWs;  // Pointer to the ''host'' OS
  e_surf_vm_state_t p_currentState;
  CpuActionLmmPtr p_action;
};

class WorkstationVM2013Lmm : public WorkstationVM2013, public WorkstationCLM03Lmm {
public:
  WorkstationVM2013Lmm(WorkstationVMModelPtr model, const char* name, xbt_dict_t props, surf_resource_t ind_phys_workstation);
  ~WorkstationVM2013Lmm();

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
  };
  bool isUsed() {WorkstationCLM03Lmm::isUsed();};
  xbt_dict_t getProperties() {WorkstationCLM03Lmm::getProperties();};
  ActionPtr execute(double size);

};

/**********
 * Action *
 **********/

#endif /* VM_WORKSTATION_HPP_ */
