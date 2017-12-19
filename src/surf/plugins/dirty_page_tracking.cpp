/* Copyright (c) 2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/live_migration.h"
#include "simgrid/s4u.hpp"
#include "src/plugins/vm/VirtualMachineImpl.hpp"
#include <map>

namespace simgrid {
namespace vm {
class VmDirtyPageTrackingExt {
  bool dp_tracking = false;
  std::map<kernel::activity::ExecImplPtr, double> dp_objs;
  double dp_updated_by_deleted_tasks = 0;

public:
  void startTracking();
  void stopTracking() { dp_tracking = false; }
  bool isTracking() { return dp_tracking; }
  void track(kernel::activity::ExecImplPtr exec, double amount) { dp_objs.insert({exec, amount}); }
  void untrack(kernel::activity::ExecImplPtr exec) { dp_objs.erase(exec); }
  double getStoredRemains(kernel::activity::ExecImplPtr exec) { return dp_objs.at(exec); }
  void updateDirtyPageCount(double delta) { dp_updated_by_deleted_tasks += delta; }
  double computedFlopsLookup();

  static simgrid::xbt::Extension<VirtualMachineImpl, VmDirtyPageTrackingExt> EXTENSION_ID;
  virtual ~VmDirtyPageTrackingExt() = default;
  VmDirtyPageTrackingExt()          = default;
};

simgrid::xbt::Extension<VirtualMachineImpl, VmDirtyPageTrackingExt> VmDirtyPageTrackingExt::EXTENSION_ID;

void VmDirtyPageTrackingExt::startTracking()
{
  dp_tracking = true;
  for (auto const& elm : dp_objs)
    dp_objs[elm.first] = elm.first->remains();
}

double VmDirtyPageTrackingExt::computedFlopsLookup()
{
  double total = 0;

  for (auto const& elm : dp_objs) {
    total += elm.second - elm.first->remains();
    dp_objs[elm.first] = elm.first->remains();
  }
  total += dp_updated_by_deleted_tasks;

  dp_updated_by_deleted_tasks = 0;

  return total;
}
}
}

static void onVirtualMachineCreation(simgrid::vm::VirtualMachineImpl* vm)
{
  vm->extension_set<simgrid::vm::VmDirtyPageTrackingExt>(new simgrid::vm::VmDirtyPageTrackingExt());
}

static void onExecCreation(simgrid::kernel::activity::ExecImplPtr exec)
{
  simgrid::s4u::VirtualMachine* vm = dynamic_cast<simgrid::s4u::VirtualMachine*>(exec->host_);
  if (vm == nullptr)
    return;

  if (vm->pimpl_vm_->extension<simgrid::vm::VmDirtyPageTrackingExt>()->isTracking()) {
    vm->pimpl_vm_->extension<simgrid::vm::VmDirtyPageTrackingExt>()->track(exec, exec->remains());
  } else {
    vm->pimpl_vm_->extension<simgrid::vm::VmDirtyPageTrackingExt>()->track(exec, 0.0);
  }
}

static void onExecCompletion(simgrid::kernel::activity::ExecImplPtr exec)
{
  simgrid::s4u::VirtualMachine* vm = dynamic_cast<simgrid::s4u::VirtualMachine*>(exec->host_);
  if (vm == nullptr)
    return;

  /* If we are in the middle of dirty page tracking, we record how much computation has been done until now, and keep
   * the information for the lookup_() function that will called soon. */
  if (vm->pimpl_vm_->extension<simgrid::vm::VmDirtyPageTrackingExt>()->isTracking()) {
    double delta =
        vm->pimpl_vm_->extension<simgrid::vm::VmDirtyPageTrackingExt>()->getStoredRemains(exec) - exec->remains();
    vm->pimpl_vm_->extension<simgrid::vm::VmDirtyPageTrackingExt>()->updateDirtyPageCount(delta);
  }
  vm->pimpl_vm_->extension<simgrid::vm::VmDirtyPageTrackingExt>()->untrack(exec);
}

SG_BEGIN_DECL()

void sg_vm_live_migration_plugin_init()
{
  if (not simgrid::vm::VmDirtyPageTrackingExt::EXTENSION_ID.valid()) {
    simgrid::vm::VmDirtyPageTrackingExt::EXTENSION_ID =
        simgrid::vm::VirtualMachineImpl::extension_create<simgrid::vm::VmDirtyPageTrackingExt>();
    simgrid::vm::VirtualMachineImpl::onVmCreation.connect(&onVirtualMachineCreation);
    simgrid::kernel::activity::ExecImpl::onCreation.connect(&onExecCreation);
    simgrid::kernel::activity::ExecImpl::onCompletion.connect(&onExecCompletion);
  }
}

void sg_vm_start_dirty_page_tracking(sg_vm_t vm)
{
  vm->pimpl_vm_->extension<simgrid::vm::VmDirtyPageTrackingExt>()->startTracking();
}

void sg_vm_stop_dirty_page_tracking(sg_vm_t vm)
{
  vm->pimpl_vm_->extension<simgrid::vm::VmDirtyPageTrackingExt>()->stopTracking();
}

double sg_vm_lookup_computed_flops(sg_vm_t vm)
{
  return vm->pimpl_vm_->extension<simgrid::vm::VmDirtyPageTrackingExt>()->computedFlopsLookup();
}
SG_END_DECL()
