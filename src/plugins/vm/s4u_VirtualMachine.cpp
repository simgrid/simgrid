/* Copyright (c) 2015-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/datatypes.h"
#include "simgrid/s4u/VirtualMachine.hpp"
#include "simgrid/s4u/host.hpp"
#include "simgrid/simix.hpp"
#include "src/plugins/vm/VirtualMachineImpl.hpp"
#include "src/simix/smx_host_private.h"
#include "src/surf/HostImpl.hpp"
#include "xbt/asserts.h"
#include "src/instr/instr_private.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_vm, "S4U virtual machines");

namespace simgrid {
namespace s4u {

VirtualMachine::VirtualMachine(const char* name, s4u::Host* pm) : Host(name)
{
  XBT_DEBUG("Create VM %s", name);

  pimpl_vm_ = new surf::VirtualMachineImpl(this, pm);
  extension_set<simgrid::simix::Host>(new simgrid::simix::Host());

  if (TRACE_msg_vm_is_enabled()) {
    container_t host_container = PJ_container_get(sg_host_get_name(pm));
    PJ_container_new(name, INSTR_MSG_VM, host_container);
  }
}

VirtualMachine::~VirtualMachine()
{
  onDestruction(*this);
}

bool VirtualMachine::isMigrating()
{
  return static_cast<surf::VirtualMachineImpl*>(pimpl_)->isMigrating;
}

/** @brief Retrieve a copy of the parameters of that VM/PM
 *  @details The ramsize and overcommit fields are used on the PM too */
void VirtualMachine::parameters(vm_params_t params)
{
  static_cast<surf::VirtualMachineImpl*>(pimpl_)->getParams(params);
}
/** @brief Sets the params of that VM/PM */
void VirtualMachine::setParameters(vm_params_t params)
{
  simgrid::simix::kernelImmediate([&]() { static_cast<surf::VirtualMachineImpl*>(pimpl_)->setParams(params); });
}

} // namespace simgrid
} // namespace s4u
