/* Copyright (c) 2015-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/datatypes.h"
#include "simgrid/s4u/VirtualMachine.hpp"
#include "simgrid/s4u/host.hpp"
#include "simgrid/simix.hpp"
#include "src/surf/HostImpl.hpp"
#include "src/surf/VirtualMachineImpl.hpp"
#include "xbt/asserts.h"

namespace simgrid {

namespace s4u {

VirtualMachine::VirtualMachine(const char* name, s4u::Host* Pm) : Host(name)
{
  pimpl_ = new surf::VirtualMachineImpl(this, Pm);
}

VirtualMachine::~VirtualMachine()
{
  onDestruction(*this);
}

void VirtualMachine::parameters(vm_params_t params)
{
  this->pimpl_->getParams(params);
}

void VirtualMachine::setParameters(vm_params_t params)
{
  simgrid::simix::kernelImmediate([&]() { this->pimpl_->setParams(params); });
}

} // namespace simgrid
} // namespace s4u
