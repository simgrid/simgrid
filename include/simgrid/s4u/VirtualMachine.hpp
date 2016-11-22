/* Copyright (c) 2015-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_VM_HPP
#define SIMGRID_S4U_VM_HPP

#include <simgrid/s4u/forward.hpp>
#include <simgrid/s4u/host.hpp>
#include <xbt/base.h>

typedef enum {
  SURF_VM_STATE_CREATED, /**< created, but not yet started */
  SURF_VM_STATE_RUNNING,
  SURF_VM_STATE_SUSPENDED, /**< Suspend/resume does not involve disk I/O, so we assume there is no transition states. */

  SURF_VM_STATE_SAVING, /**< Save/restore involves disk I/O, so there should be transition states. */
  SURF_VM_STATE_SAVED,
  SURF_VM_STATE_RESTORING,
} e_surf_vm_state_t;

namespace simgrid {
namespace surf {
class VirtualMachineImpl;
};
namespace s4u {

/** @ingroup s4u_api
 *
 * @tableofcontents
 *
 * A VM is a virtual machine that contains actors. The total computing power that the contained
 * processes can get is constrained to the virtual machine size.
 *
 */
XBT_PUBLIC_CLASS VirtualMachine : public s4u::Host
{

public:
  explicit VirtualMachine(const char* name, s4u::Host* hostPm);

  // No copy/move
  VirtualMachine(VirtualMachine const&) = delete;
  VirtualMachine& operator=(VirtualMachine const&) = delete;

private:
  virtual ~VirtualMachine();

public:
  bool isMigrating();

  void parameters(vm_params_t params);
  void setParameters(vm_params_t params);
  double getRamsize();

  /* FIXME: protect me */
  simgrid::surf::VirtualMachineImpl* pimpl_vm_ = nullptr;
};
}
} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_HOST_HPP */
