/* Copyright (c) 2015-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_VM_HPP
#define SIMGRID_S4U_VM_HPP

#include <simgrid/s4u/Host.hpp>
#include <simgrid/s4u/forward.hpp>

enum e_surf_vm_state_t {
  SURF_VM_STATE_CREATED, /**< created, but not yet started */
  SURF_VM_STATE_RUNNING,
  SURF_VM_STATE_SUSPENDED, /**< Suspend/resume does not involve disk I/O, so we assume there is no transition states. */
  SURF_VM_STATE_DESTROYED
};

namespace simgrid {
namespace vm {
class VirtualMachineImpl;
};

namespace s4u {

/** @ingroup s4u_api
 *
 * @tableofcontents
 *
 * A VM represents a virtual machine (or a container) that hosts actors.
 * The total computing power that the contained actors can get is constrained to the virtual machine size.
 *
 */
class XBT_PUBLIC VirtualMachine : public s4u::Host {
  simgrid::vm::VirtualMachineImpl* pimpl_vm_ = nullptr;
  virtual ~VirtualMachine();

public:
  explicit VirtualMachine(const char* name, s4u::Host* hostPm, int coreAmount);
  explicit VirtualMachine(const char* name, s4u::Host* hostPm, int coreAmount, size_t ramsize);

  // No copy/move
  VirtualMachine(VirtualMachine const&) = delete;
  VirtualMachine& operator=(VirtualMachine const&) = delete;

  simgrid::vm::VirtualMachineImpl* getImpl() { return pimpl_vm_; }
  void start();
  void suspend();
  void resume();
  void shutdown();
  void destroy();

  simgrid::s4u::Host* getPm();
  void setPm(simgrid::s4u::Host * pm);
  size_t getRamsize();
  void setRamsize(size_t ramsize);
  void setBound(double bound);

  e_surf_vm_state_t getState();
  static simgrid::xbt::signal<void(simgrid::s4u::VirtualMachine*)> onVmShutdown;
};
}
} // namespace simgrid::s4u

#endif
