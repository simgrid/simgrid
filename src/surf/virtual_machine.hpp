/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <deque>
#include <boost/intrusive/list.hpp>

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
extern XBT_PRIVATE simgrid::xbt::signal<void(simgrid::surf::VirtualMachine*)> onVmCreation;

/** @ingroup SURF_callbacks
 * @brief Callbacks fired after VM destruction. Signature: `void(VirtualMachine*)`
 */
extern XBT_PRIVATE simgrid::xbt::signal<void(simgrid::surf::VirtualMachine*)> onVmDestruction;

/** @ingroup SURF_callbacks
 * @brief Callbacks after VM State changes. Signature: `void(VirtualMachine*)`
 */
extern XBT_PRIVATE simgrid::xbt::signal<void(simgrid::surf::VirtualMachine*)> onVmStateChange;

/************
 * Resource *
 ************/

/** @ingroup SURF_vm_interface
 * @brief SURF VM interface class
 * @details A VM represent a virtual machine
 */
class VirtualMachine : public HostImpl {
public:
  VirtualMachine(simgrid::surf::HostModel *model, const char *name, simgrid::s4u::Host *host);
  ~VirtualMachine();

  /** @brief Suspend the VM */
  virtual void suspend();

  /** @brief Resume the VM */
  virtual void resume();

  /** @brief Save the VM (Not yet implemented) */
  virtual void save();

  /** @brief Restore the VM (Not yet implemented) */
  virtual void restore();

  /** @brief Migrate the VM to the destination host */
  virtual void migrate(sg_host_t dest_PM);

  /** @brief Get the physical machine hosting the VM */
  sg_host_t getPm();

  virtual void setBound(double bound);

  /* The vm object of the lower layer */
  CpuAction *action_ = nullptr;
protected:
  simgrid::s4u::Host *hostPM_;

public:
  void turnOn() override;
  void turnOff() override;

  e_surf_vm_state_t getState();
  void setState(e_surf_vm_state_t state);
  static std::deque<VirtualMachine*> allVms_;

protected:
  e_surf_vm_state_t vmState_ = SURF_VM_STATE_CREATED;
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
  VMModel() :HostModel() {}
  ~VMModel() = default;

  /**
   * @brief Create a new VM
   *
   * @param name The name of the new VM
   * @param host_PM The real machine hosting the VM
   */
  s4u::Host *createVM(const char *name, sg_host_t host_PM);
  void adjustWeightOfDummyCpuActions() {};

  double next_occuring_event(double now) override;
  void updateActionsState(double /*now*/, double /*delta*/) {};

};

}
}

#endif /* VM_INTERFACE_HPP_ */
