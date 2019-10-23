/* Copyright (c) 2004-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/VirtualMachine.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/surf/HostImpl.hpp"
#include <algorithm>
#include <deque>
#include <unordered_map>

#ifndef VM_INTERFACE_HPP_
#define VM_INTERFACE_HPP_

namespace simgrid {
namespace vm {

/************
 * Resource *
 ************/

/** @ingroup SURF_vm_interface
 * @brief SURF VM interface class
 * @details A VM represent a virtual machine
 */
class XBT_PUBLIC VirtualMachineImpl : public surf::HostImpl, public simgrid::xbt::Extendable<VirtualMachineImpl> {
  friend simgrid::s4u::VirtualMachine;

public:
  explicit VirtualMachineImpl(s4u::VirtualMachine* piface, s4u::Host* host, int core_amount, size_t ramsize);
  ~VirtualMachineImpl();

  /** @brief Callbacks fired after VM creation. Signature: `void(VirtualMachineImpl&)` */
  static xbt::signal<void(simgrid::vm::VirtualMachineImpl&)> on_creation;
  /** @brief Callbacks fired after VM destruction. Signature: `void(VirtualMachineImpl const&)` */
  static xbt::signal<void(simgrid::vm::VirtualMachineImpl const&)> on_destruction;

  virtual void suspend(kernel::actor::ActorImpl* issuer);
  virtual void resume();
  virtual void shutdown(kernel::actor::ActorImpl* issuer);

  /** @brief Change the physical host on which the given VM is running */
  virtual void set_physical_host(s4u::Host* dest);
  /** @brief Get the physical host on which the given VM is running */
  s4u::Host* get_physical_host() { return physical_host_; }

  sg_size_t get_ramsize() { return ramsize_; }
  void set_ramsize(sg_size_t ramsize) { ramsize_ = ramsize; }

  s4u::VirtualMachine::state get_state() { return vm_state_; }
  void set_state(s4u::VirtualMachine::state state) { vm_state_ = state; }

  int get_core_amount() { return core_amount_; }

  virtual void set_bound(double bound);

  /* The vm object of the lower layer */
  kernel::resource::Action* action_ = nullptr;
  static std::deque<s4u::VirtualMachine*> allVms_;
  bool is_migrating_ = false;
  int active_tasks_ = 0;

  void update_action_weight();

private:
  s4u::Host* physical_host_;
  int core_amount_;
  double user_bound_;
  size_t ramsize_            = 0;
  s4u::VirtualMachine::state vm_state_ = s4u::VirtualMachine::state::CREATED;
};

/*********
 * Model *
 *********/
/** @ingroup SURF_vm_interface
 * @brief SURF VM model interface class
 * @details A model is an object which handle the interactions between its Resources and its Actions
 */
class XBT_PRIVATE VMModel : public surf::HostModel {
public:
  VMModel();

  double next_occurring_event(double now) override;
  void update_actions_state(double /*now*/, double /*delta*/) override{};
};
}
}

XBT_PUBLIC_DATA simgrid::vm::VMModel* surf_vm_model;

#endif /* VM_INTERFACE_HPP_ */
