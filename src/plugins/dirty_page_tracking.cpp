/* Copyright (c) 2017-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/live_migration.h"
#include "simgrid/s4u/Exec.hpp"
#include "src/kernel/activity/ExecImpl.hpp"
#include "src/plugins/vm/VirtualMachineImpl.hpp"

namespace simgrid {
namespace vm {
class DirtyPageTrackingExt {
  bool dp_tracking_ = false;
  std::map<kernel::activity::ExecImpl const*, double> dp_objs_;
  double dp_updated_by_deleted_tasks_ = 0.0;
  // Percentage of pages that get dirty compared to netspeed [0;1] bytes per 1 flop execution
  double dp_intensity_          = 0.0;
  sg_size_t working_set_memory_ = 0.0;
  double max_downtime_          = 0.03;
  double mig_speed_             = 0.0;

public:
  void start_tracking();
  void stop_tracking() { dp_tracking_ = false; }
  bool is_tracking() const { return dp_tracking_; }
  void track(kernel::activity::ExecImpl const* exec, double amount) { dp_objs_.insert({exec, amount}); }
  void untrack(kernel::activity::ExecImpl const* exec) { dp_objs_.erase(exec); }
  double get_stored_remains(kernel::activity::ExecImpl const* exec) { return dp_objs_.at(exec); }
  void update_dirty_page_count(double delta) { dp_updated_by_deleted_tasks_ += delta; }
  double computed_flops_lookup();
  double get_intensity() const { return dp_intensity_; }
  void set_intensity(double intensity) { dp_intensity_ = intensity; }
  sg_size_t get_working_set_memory() const { return working_set_memory_; }
  void set_working_set_memory(sg_size_t size) { working_set_memory_ = size; }
  void set_migration_speed(double speed) { mig_speed_ = speed; }
  double get_migration_speed() const { return mig_speed_; }
  double get_max_downtime() const { return max_downtime_; }

  static simgrid::xbt::Extension<VirtualMachineImpl, DirtyPageTrackingExt> EXTENSION_ID;
  virtual ~DirtyPageTrackingExt() = default;
  DirtyPageTrackingExt()          = default;
};

simgrid::xbt::Extension<VirtualMachineImpl, DirtyPageTrackingExt> DirtyPageTrackingExt::EXTENSION_ID;

void DirtyPageTrackingExt::start_tracking()
{
  dp_tracking_ = true;
  for (auto const& elm : dp_objs_)
    dp_objs_[elm.first] = elm.first->get_remaining();
}

double DirtyPageTrackingExt::computed_flops_lookup()
{
  double total = 0;

  for (auto const& elm : dp_objs_) {
    total += elm.second - elm.first->get_remaining();
    dp_objs_[elm.first] = elm.first->get_remaining();
  }
  total += dp_updated_by_deleted_tasks_;

  dp_updated_by_deleted_tasks_ = 0;

  return total;
}
} // namespace vm
} // namespace simgrid

static void on_virtual_machine_creation(simgrid::vm::VirtualMachineImpl& vm)
{
  vm.extension_set<simgrid::vm::DirtyPageTrackingExt>(new simgrid::vm::DirtyPageTrackingExt());
}

static void on_exec_creation(simgrid::s4u::Actor const&, simgrid::s4u::Exec const& e)
{
  auto exec                        = static_cast<simgrid::kernel::activity::ExecImpl*>(e.get_impl());
  const simgrid::s4u::VirtualMachine* vm = dynamic_cast<simgrid::s4u::VirtualMachine*>(exec->get_host());
  if (vm == nullptr)
    return;

  if (vm->get_impl()->extension<simgrid::vm::DirtyPageTrackingExt>()->is_tracking()) {
    vm->get_impl()->extension<simgrid::vm::DirtyPageTrackingExt>()->track(exec, exec->get_remaining());
  } else {
    vm->get_impl()->extension<simgrid::vm::DirtyPageTrackingExt>()->track(exec, 0.0);
  }
}

static void on_exec_completion(simgrid::s4u::Actor const&, simgrid::s4u::Exec const& e)
{
  auto exec                        = static_cast<simgrid::kernel::activity::ExecImpl*>(e.get_impl());
  const simgrid::s4u::VirtualMachine* vm = dynamic_cast<simgrid::s4u::VirtualMachine*>(exec->get_host());
  if (vm == nullptr)
    return;

  /* If we are in the middle of dirty page tracking, we record how much computation has been done until now, and keep
   * the information for the lookup_() function that will called soon. */
  if (vm->get_impl()->extension<simgrid::vm::DirtyPageTrackingExt>()->is_tracking()) {
    double delta = vm->get_impl()->extension<simgrid::vm::DirtyPageTrackingExt>()->get_stored_remains(exec);
    vm->get_impl()->extension<simgrid::vm::DirtyPageTrackingExt>()->update_dirty_page_count(delta);
  }
  vm->get_impl()->extension<simgrid::vm::DirtyPageTrackingExt>()->untrack(exec);
}

void sg_vm_dirty_page_tracking_init()
{
  if (not simgrid::vm::DirtyPageTrackingExt::EXTENSION_ID.valid()) {
    simgrid::vm::DirtyPageTrackingExt::EXTENSION_ID =
        simgrid::vm::VirtualMachineImpl::extension_create<simgrid::vm::DirtyPageTrackingExt>();
    simgrid::vm::VirtualMachineImpl::on_creation.connect(&on_virtual_machine_creation);
    simgrid::s4u::Exec::on_start.connect(&on_exec_creation);
    simgrid::s4u::Exec::on_completion.connect(&on_exec_completion);
  }
}

void sg_vm_start_dirty_page_tracking(const_sg_vm_t vm)
{
  vm->get_impl()->extension<simgrid::vm::DirtyPageTrackingExt>()->start_tracking();
}

void sg_vm_stop_dirty_page_tracking(const_sg_vm_t vm)
{
  vm->get_impl()->extension<simgrid::vm::DirtyPageTrackingExt>()->stop_tracking();
}

double sg_vm_lookup_computed_flops(const_sg_vm_t vm)
{
  return vm->get_impl()->extension<simgrid::vm::DirtyPageTrackingExt>()->computed_flops_lookup();
}

void sg_vm_set_dirty_page_intensity(const_sg_vm_t vm, double intensity)
{
  vm->get_impl()->extension<simgrid::vm::DirtyPageTrackingExt>()->set_intensity(intensity);
}

double sg_vm_get_dirty_page_intensity(const_sg_vm_t vm)
{
  return vm->get_impl()->extension<simgrid::vm::DirtyPageTrackingExt>()->get_intensity();
}

void sg_vm_set_working_set_memory(const_sg_vm_t vm, sg_size_t size)
{
  vm->get_impl()->extension<simgrid::vm::DirtyPageTrackingExt>()->set_working_set_memory(size);
}

sg_size_t sg_vm_get_working_set_memory(const_sg_vm_t vm)
{
  return vm->get_impl()->extension<simgrid::vm::DirtyPageTrackingExt>()->get_working_set_memory();
}

void sg_vm_set_migration_speed(const_sg_vm_t vm, double speed)
{
  vm->get_impl()->extension<simgrid::vm::DirtyPageTrackingExt>()->set_migration_speed(speed);
}

double sg_vm_get_migration_speed(const_sg_vm_t vm)
{
  return vm->get_impl()->extension<simgrid::vm::DirtyPageTrackingExt>()->get_migration_speed();
}

double sg_vm_get_max_downtime(const_sg_vm_t vm)
{
  return vm->get_impl()->extension<simgrid::vm::DirtyPageTrackingExt>()->get_max_downtime();
}
