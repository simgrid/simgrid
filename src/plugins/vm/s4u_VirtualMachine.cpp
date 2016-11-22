/* Copyright (c) 2015-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/datatypes.h"
#include "simgrid/s4u/VirtualMachine.hpp"
#include "simgrid/s4u/host.hpp"
#include "simgrid/simix.hpp"
#include "src/instr/instr_private.h"
#include "src/plugins/vm/VirtualMachineImpl.hpp"
#include "src/simix/smx_host_private.h"
#include "src/surf/HostImpl.hpp"
#include "src/surf/cpu_cas01.hpp"
#include "xbt/asserts.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_vm, "S4U virtual machines");

namespace simgrid {
namespace s4u {

VirtualMachine::VirtualMachine(const char* name, s4u::Host* pm) : Host(name)
{
  XBT_DEBUG("Create VM %s", name);

  pimpl_vm_ = new vm::VirtualMachineImpl(this, pm);
  /* Currently, a VM uses the network resource of its physical host */
  pimpl_netcard = pm->pimpl_netcard;
  // Create a VCPU for this VM
  surf::CpuCas01* sub_cpu = dynamic_cast<surf::CpuCas01*>(pm->pimpl_cpu);

  pimpl_cpu = surf_cpu_model_vm->createCpu(this, sub_cpu->getSpeedPeakList(), 1 /*cores*/);
  if (sub_cpu->getPState() != 0)
    pimpl_cpu->setPState(sub_cpu->getPState());

  /* Make a process container */
  extension_set<simgrid::simix::Host>(new simgrid::simix::Host());

  if (TRACE_msg_vm_is_enabled()) {
    container_t host_container = PJ_container_get(pm->cname());
    PJ_container_new(name, INSTR_MSG_VM, host_container);
  }
}

VirtualMachine::~VirtualMachine()
{
  onDestruction(*this);

  XBT_DEBUG("destroy %s", name().c_str());

  /* FIXME: this is really strange that everything fails if the next line is removed.
   * This is as if we shared these data with the PM, which definitely should not be the case...
   *
   * We need to test that suspending a VM does not suspends the processes running on its PM, for example.
   * Or we need to simplify this code enough to make it actually readable (but this sounds harder than testing)
   */
  extension_set<simgrid::simix::Host>(nullptr);

  /* Don't free these things twice: they are the ones of my physical host */
  pimpl_netcard = nullptr;
}

bool VirtualMachine::isMigrating()
{
  return pimpl_vm_->isMigrating;
}
double VirtualMachine::getRamsize()
{
  return pimpl_vm_->params_.ramsize;
}

/** @brief Retrieve a copy of the parameters of that VM/PM
 *  @details The ramsize and overcommit fields are used on the PM too */
void VirtualMachine::parameters(vm_params_t params)
{
  pimpl_vm_->getParams(params);
}
/** @brief Sets the params of that VM/PM */
void VirtualMachine::setParameters(vm_params_t params)
{
  simgrid::simix::kernelImmediate([&]() { pimpl_vm_->setParams(params); });
}

} // namespace simgrid
} // namespace s4u
