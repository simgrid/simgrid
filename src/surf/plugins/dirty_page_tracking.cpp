/* Copyright (c) 2017-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/live_migration.h"
#include "src/plugins/vm/VirtualMachineImpl.hpp"

namespace simgrid {
namespace vm {
class VmDirtyPageTrackingExt {
  bool dp_tracking = false;
  std::map<kernel::activity::ExecImplPtr, double> dp_objs;
  double dp_updated_by_deleted_tasks = 0.0;
  // Percentage of pages that get dirty compared to netspeed [0;1] bytes per 1 flop execution
  double dp_intensity          = 0.0;
  sg_size_t working_set_memory = 0.0;
  double max_downtime          = 0.03;
  double mig_speed             = 0.0;

public:
  void start_tracking();
  void stop_tracking() { dp_tracking = false; }
  bool is_tracking() { return dp_tracking; }
  void track(kernel::activity::ExecImplPtr exec, double amount) { dp_objs.insert({exec, amount}); }
  void untrack(kernel::activity::ExecImplPtr exec) { dp_objs.erase(exec); }
  double get_stored_remains(kernel::activity::ExecImplPtr exec) { return dp_objs.at(exec); }
  void update_dirty_page_count(double delta) { dp_updated_by_deleted_tasks += delta; }
  double computed_flops_lookup();
  double get_intensity() { return dp_intensity; }
  void set_intensity(double intensity) { dp_intensity = intensity; }
  double get_working_set_memory() { return working_set_memory; }
  void set_working_set_memory(sg_size_t size) { working_set_memory = size; }
  void set_migration_speed(double speed) { mig_speed = speed; }
  double get_migration_speed() { return mig_speed; }
  double get_max_downtime() { return max_downtime; }

  static simgrid::xbt::Extension<VirtualMachineImpl, VmDirtyPageTrackingExt> EXTENSION_ID;
  virtual ~VmDirtyPageTrackingExt() = default;
  VmDirtyPageTrackingExt()          = default;
};

simgrid::xbt::Extension<VirtualMachineImpl, VmDirtyPageTrackingExt> VmDirtyPageTrackingExt::EXTENSION_ID;

void VmDirtyPageTrackingExt::start_tracking()
{
  dp_tracking = true;
  for (auto const& elm : dp_objs)
    dp_objs[elm.first] = elm.first->remains();
}

double VmDirtyPageTrackingExt::computed_flops_lookup()
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

  if (vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->is_tracking()) {
    vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->track(exec, exec->remains());
  } else {
    vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->track(exec, 0.0);
  }
}

static void onExecCompletion(simgrid::kernel::activity::ExecImplPtr exec)
{
  simgrid::s4u::VirtualMachine* vm = dynamic_cast<simgrid::s4u::VirtualMachine*>(exec->host_);
  if (vm == nullptr)
    return;

  /* If we are in the middle of dirty page tracking, we record how much computation has been done until now, and keep
   * the information for the lookup_() function that will called soon. */
  if (vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->is_tracking()) {
    double delta =
        vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->get_stored_remains(exec) - exec->remains();
    vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->update_dirty_page_count(delta);
  }
  vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->untrack(exec);
}

void sg_vm_dirty_page_tracking_init()
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
  vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->start_tracking();
}

void sg_vm_stop_dirty_page_tracking(sg_vm_t vm)
{
  vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->stop_tracking();
}

double sg_vm_lookup_computed_flops(sg_vm_t vm)
{
  return vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->computed_flops_lookup();
}

void sg_vm_set_dirty_page_intensity(sg_vm_t vm, double intensity)
{
  vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->set_intensity(intensity);
}

double sg_vm_get_dirty_page_intensity(sg_vm_t vm)
{
  return vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->get_intensity();
}

void sg_vm_set_working_set_memory(sg_vm_t vm, sg_size_t size)
{
  vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->set_working_set_memory(size);
}

sg_size_t sg_vm_get_working_set_memory(sg_vm_t vm)
{
  return vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->get_working_set_memory();
}

void sg_vm_set_migration_speed(sg_vm_t vm, double speed)
{
  vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->set_migration_speed(speed);
}

double sg_vm_get_migration_speed(sg_vm_t vm)
{
  return vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->get_migration_speed();
}

double sg_vm_get_max_downtime(sg_vm_t vm)
{
  return vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->get_max_downtime();
}
