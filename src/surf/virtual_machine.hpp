/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/base.h>

#include "src/surf/HostImpl.hpp"

#ifndef VM_INTERFACE_HPP_
#define VM_INTERFACE_HPP_

#define GUESTOS_NOISE 100 // This value corresponds to the cost of the global action associated to the VM
                          // It corresponds to the cost of a VM running no tasks.

namespace simgrid {
namespace surf {

/***********
 * Classes *
 ***********/

class XBT_PRIVATE VMModel;
class XBT_PRIVATE VirtualMachine;

/*************
 * Callbacks *
 *************/

/** @ingroup SURF_callbacks
 * @brief Callbacks fired after VM creation. Signature: `void(VirtualMachine*)`
 */
extern XBT_PRIVATE simgrid::xbt::signal<void(simgrid::surf::VirtualMachine*)> VMCreatedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks fired after VM destruction. Signature: `void(VirtualMachine*)`
 */
extern XBT_PRIVATE simgrid::xbt::signal<void(simgrid::surf::VirtualMachine*)> VMDestructedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks after VM State changes. Signature: `void(VirtualMachine*)`
 */
extern XBT_PRIVATE simgrid::xbt::signal<void(simgrid::surf::VirtualMachine*)> VMStateChangedCallbacks;

/************
 * Resource *
 ************/

/** @ingroup SURF_vm_interface
 * @brief SURF VM interface class
 * @details A VM represent a virtual machine
 */
class VirtualMachine : public HostImpl {
public:
  /**
   * @brief Constructor
   *
   * @param model VMModel associated to this VM
   * @param name The name of the VM
   * @param props Dictionary of properties associated to this VM
   * @param host The host
   */
  VirtualMachine(simgrid::surf::HostModel *model, const char *name, xbt_dict_t props,
            simgrid::s4u::Host *host);

  /** @brief Destructor */
  ~VirtualMachine();

  /** @brief Suspend the VM */
  virtual void suspend()=0;

  /** @brief Resume the VM */
  virtual void resume()=0;

  /** @brief Save the VM (Not yet implemented) */
  virtual void save()=0;

  /** @brief Restore the VM (Not yet implemented) */
  virtual void restore()=0;

  /** @brief Migrate the VM to the destination host */
  virtual void migrate(sg_host_t dest_PM)=0;

  /** @brief Get the physical machine hosting the VM */
  sg_host_t getPm();

  virtual void setBound(double bound)=0;
  virtual void setAffinity(Cpu *cpu, unsigned long mask)=0;

  /* The vm object of the lower layer */
  CpuAction *p_action;
  simgrid::s4u::Host *p_hostPM;

  void turnOn() override;
  void turnOff() override;

public:
  e_surf_vm_state_t getState();
  void setState(e_surf_vm_state_t state);
protected:
  e_surf_vm_state_t p_vm_state = SURF_VM_STATE_CREATED;


public:
  boost::intrusive::list_member_hook<> vm_hook;
};

/*********
 * Model *
 *********/
/** @ingroup SURF_vm_interface
 * @brief SURF VM model interface class
 * @details A model is an object which handle the interactions between its Resources and its Actions
 */
class VMModel : public HostModel {
public:
  VMModel() :HostModel(){}
  ~VMModel(){};

  /**
   * @brief Create a new VM
   *
   * @param name The name of the new VM
   * @param host_PM The real machine hosting the VM
   *
   */
  virtual VirtualMachine *createVM(const char *name, sg_host_t host_PM)=0;
  void adjustWeightOfDummyCpuActions() {};

  typedef boost::intrusive::member_hook<
    VirtualMachine, boost::intrusive::list_member_hook<>, &VirtualMachine::vm_hook> VmOptions;
  typedef boost::intrusive::list<VirtualMachine, VmOptions, boost::intrusive::constant_time_size<false> > vm_list_t;
  static vm_list_t ws_vms;
};

}
}

#endif /* VM_INTERFACE_HPP_ */
