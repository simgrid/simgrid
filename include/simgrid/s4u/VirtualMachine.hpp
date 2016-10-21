/* Copyright (c) 2015-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_VM_HPP
#define SIMGRID_S4U_VM_HPP

#include <simgrid/s4u/forward.hpp>
#include <simgrid/s4u/host.hpp>
#include <xbt/base.h>

namespace simgrid {

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
  void parameters(vm_params_t params) override;
  void setParameters(vm_params_t params) override;
};
}
} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_HOST_HPP */
