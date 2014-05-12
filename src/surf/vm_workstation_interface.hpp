/* Copyright (c) 2004-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "workstation_interface.hpp"

#ifndef VM_WORKSTATION_INTERFACE_HPP_
#define VM_WORKSTATION_INTERFACE_HPP_

#define GUESTOS_NOISE 100 // This value corresponds to the cost of the global action associated to the VM
                          // It corresponds to the cost of a VM running no tasks.

/***********
 * Classes *
 ***********/

class WorkstationVMModel;
typedef WorkstationVMModel *WorkstationVMModelPtr;

class WorkstationVM;
typedef WorkstationVM *WorkstationVMPtr;

class WorkstationVMLmm;
typedef WorkstationVMLmm *WorkstationVMLmmPtr;

/*************
 * Callbacks *
 *************/

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after WorkstationVM creation *
 * @details Callback functions have the following signature: `void(WorkstationVMPtr)`
 */
extern surf_callback(void, WorkstationVMPtr) workstationVMCreatedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after WorkstationVM destruction *
 * @details Callback functions have the following signature: `void(WorkstationVMPtr)`
 */
extern surf_callback(void, WorkstationVMPtr) workstationVMDestructedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after WorkstationVM State changed *
 * @details Callback functions have the following signature: `void(WorkstationVMActionPtr)`
 */
extern surf_callback(void, WorkstationVMPtr) workstationVMStateChangedCallbacks;

/*********
 * Model *
 *********/
/** @ingroup SURF_vm_workstation_interface
 * @brief SURF workstation VM model interface class
 * @details A model is an object which handle the interactions between its Resources and its Actions
 */
class WorkstationVMModel : public WorkstationModel {
public:
  /**
   * @brief WorkstationVMModel consrtuctor
   */
  WorkstationVMModel();

    /**
   * @brief WorkstationVMModel consrtuctor
   */
  ~WorkstationVMModel(){};

  WorkstationPtr createWorkstation(const char *name){DIE_IMPOSSIBLE;}

  /**
   * @brief Create a new WorkstationVM
   *
   * @param name The name of the new WorkstationVM
   * @param ind_phys_workstation The workstation hosting the VM
   *
   */
  virtual WorkstationVMPtr createWorkstationVM(const char *name, surf_resource_t ind_phys_workstation)=0;
  void adjustWeightOfDummyCpuActions() {};

  typedef boost::intrusive::list<WorkstationVM,
                                 boost::intrusive::constant_time_size<false> >
          vm_list_t;
  static vm_list_t ws_vms;
};

/************
 * Resource *
 ************/

/** @ingroup SURF_vm_workstation_interface
 * @brief SURF workstation VM interface class
 * @details A workstation VM represent an virtual machine
 */
class WorkstationVM : public Workstation,
                      public boost::intrusive::list_base_hook<> {
public:
  /**
   * @brief WorkstationVM consrtructor
   *
   * @param model WorkstationModel associated to this Workstation
   * @param name The name of the Workstation
   * @param props Dictionary of properties associated to this Workstation
   * @param netElm The RoutingEdge associated to this Workstation
   * @param cpu The Cpu associated to this Workstation
   */
  WorkstationVM(ModelPtr model, const char *name, xbt_dict_t props,
		        RoutingEdgePtr netElm, CpuPtr cpu);

  /**
   * @brief WdorkstationVM destructor
   */
  ~WorkstationVM();

  void setState(e_surf_resource_state_t state);

  /**
   * @brief Suspend the VM
   */
  virtual void suspend()=0;

  /**
   * @brief Resume the VM
   */
  virtual void resume()=0;

  /**
   * @brief Save the VM (Not yet implemented)
   */
  virtual void save()=0;

  /**
   * @brief Restore the VM (Not yet implemented)
   */
  virtual void restore()=0;

  /**
   * @brief Migrate the VM to the destination host
   *
   * @param ind_vm_ws_dest The destination host
   */
  virtual void migrate(surf_resource_t ind_vm_ws_dest)=0;

  /**
   * @brief Get the physical machine hosting the VM
   * @return The physical machine hosting the VM
   */
  virtual surf_resource_t getPm()=0;

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
