/* Copyright (c) 2015-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.h"
#include "src/plugins/vm/VirtualMachineImpl.hpp"
#include "src/plugins/vm/VmHostExt.hpp"
#include "src/simix/smx_host_private.h"
#include "src/surf/cpu_cas01.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_vm, "S4U virtual machines");

namespace simgrid {
namespace s4u {

VirtualMachine::VirtualMachine(const char* name, s4u::Host* pm, int coreAmount)
    : Host(name), pimpl_vm_(new vm::VirtualMachineImpl(this, pm, coreAmount))
{
  XBT_DEBUG("Create VM %s", name);

  /* Currently, a VM uses the network resource of its physical host */
  pimpl_netpoint = pm->pimpl_netpoint;
  // Create a VCPU for this VM
  surf::CpuCas01* sub_cpu = dynamic_cast<surf::CpuCas01*>(pm->pimpl_cpu);

  pimpl_cpu = surf_cpu_model_vm->createCpu(this, sub_cpu->getSpeedPeakList(), coreAmount);
  if (sub_cpu->getPState() != 0)
    pimpl_cpu->setPState(sub_cpu->getPState());

  /* Make a process container */
  extension_set<simgrid::simix::Host>(new simgrid::simix::Host());

  if (TRACE_msg_vm_is_enabled()) {
    Container* host_container = Container::s_container_get(pm->getCname());
    new Container(name, INSTR_MSG_VM, host_container);
  }
}

VirtualMachine::~VirtualMachine()
{
  onDestruction(*this);

  XBT_DEBUG("destroy %s", getCname());

  /* FIXME: this is really strange that everything fails if the next line is removed.
   * This is as if we shared these data with the PM, which definitely should not be the case...
   *
   * We need to test that suspending a VM does not suspends the processes running on its PM, for example.
   * Or we need to simplify this code enough to make it actually readable (but this sounds harder than testing)
   */
  extension_set<simgrid::simix::Host>(nullptr);

  /* Don't free these things twice: they are the ones of my physical host */
  pimpl_netpoint = nullptr;
}

void VirtualMachine::start()
{
  simgrid::simix::kernelImmediate([this]() {
    simgrid::vm::VmHostExt::ensureVmExtInstalled();

    simgrid::s4u::Host* pm = this->pimpl_vm_->getPm();
    if (pm->extension<simgrid::vm::VmHostExt>() == nullptr)
      pm->extension_set(new simgrid::vm::VmHostExt());

    long pm_ramsize   = pm->extension<simgrid::vm::VmHostExt>()->ramsize;
    int pm_overcommit = pm->extension<simgrid::vm::VmHostExt>()->overcommit;
    long vm_ramsize   = this->getRamsize();

    if (pm_ramsize && not pm_overcommit) { /* Only verify that we don't overcommit on need */
      /* Retrieve the memory occupied by the VMs on that host. Yep, we have to traverse all VMs of all hosts for that */
      long total_ramsize_of_vms = 0;
      for (simgrid::s4u::VirtualMachine* ws_vm : simgrid::vm::VirtualMachineImpl::allVms_)
        if (pm == ws_vm->pimpl_vm_->getPm())
          total_ramsize_of_vms += ws_vm->pimpl_vm_->getRamsize();

      if (vm_ramsize > pm_ramsize - total_ramsize_of_vms) {
        XBT_WARN("cannnot start %s@%s due to memory shortage: vm_ramsize %ld, free %ld, pm_ramsize %ld (bytes).",
                 this->getCname(), pm->getCname(), vm_ramsize, pm_ramsize - total_ramsize_of_vms, pm_ramsize);
        THROWF(vm_error, 0, "Memory shortage on host '%s', VM '%s' cannot be started", pm->getCname(),
               this->getCname());
      }
    }

    this->pimpl_vm_->setState(SURF_VM_STATE_RUNNING);
  });
}

bool VirtualMachine::isMigrating()
{
  return pimpl_vm_ && pimpl_vm_->isMigrating;
}
double VirtualMachine::getRamsize()
{
  return pimpl_vm_->params_.ramsize;
}
simgrid::s4u::Host* VirtualMachine::getPm()
{
  return pimpl_vm_->getPm();
}
e_surf_vm_state_t VirtualMachine::getState()
{
  return pimpl_vm_->getState();
}

/** @brief Retrieve a copy of the parameters of that VM/PM
 *  @details The ramsize and overcommit fields are used on the PM too */
void VirtualMachine::getParameters(vm_params_t params)
{
  pimpl_vm_->getParams(params);
}
/** @brief Sets the params of that VM/PM */
void VirtualMachine::setParameters(vm_params_t params)
{
  simgrid::simix::kernelImmediate([this, params] { pimpl_vm_->setParams(params); });
}

} // namespace simgrid
} // namespace s4u
