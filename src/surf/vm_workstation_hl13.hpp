/* Copyright (c) 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

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

class WorkstationVMHL13;
typedef WorkstationVMHL13 *WorkstationVMHL13Ptr;

/*********
 * Tools *
 *********/

/*********
 * Model *
 *********/
class WorkstationVMHL13Model : public WorkstationVMModel {
public:
  WorkstationVMHL13Model();
  ~WorkstationVMHL13Model(){};
  void createResource(const char *name, void *ind_phys_workstation);
  double shareResources(double now);
  void adjustWeightOfDummyCpuActions() {};
  xbt_dynar_t getRoute(WorkstationPtr src, WorkstationPtr dst);
  ActionPtr communicate(WorkstationPtr src, WorkstationPtr dst, double size, double rate);
  ActionPtr executeParallelTask(int workstation_nb,
                                          void **workstation_list,
                                          double *computation_amount,
                                          double *communication_amount,
                                          double rate);
  void updateActionsState(double /*now*/, double /*delta*/);
};

/************
 * Resource *
 ************/

class WorkstationVMHL13 : public WorkstationVM {
public:
  WorkstationVMHL13(WorkstationVMModelPtr model, const char* name, xbt_dict_t props, surf_resource_t ind_phys_workstation);
  ~WorkstationVMHL13();

  void suspend();
  void resume();

  void save();
  void restore();

  void migrate(surf_resource_t ind_dst_pm);

  e_surf_resource_state_t getState();
  void setState(e_surf_resource_state_t state);

  surf_resource_t getPm(); // will be vm_ws_get_pm()

  void setBound(double bound);
  void setAffinity(CpuPtr cpu, unsigned long mask);

  //FIXME: remove
  void updateState(tmgr_trace_event_t event_type, double value, double date);
  bool isUsed();

  ActionPtr execute(double size);
  ActionPtr sleep(double duration);
};

/**********
 * Action *
 **********/

#endif /* VM_WORKSTATION_HPP_ */
