/* Copyright (c) 2013-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/Exception.hpp>
#include <simgrid/kernel/routing/NetZoneImpl.hpp>
#include <simgrid/s4u/Exec.hpp>

#include "simgrid/sg_config.hpp"
#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/activity/ExecImpl.hpp"
#include "src/kernel/resource/VirtualMachineImpl.hpp"
#include "src/surf/cpu_cas01.hpp"
#include "src/surf/cpu_ti.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(res_vm, ker_resource, "Virtual Machines, containing actors and mobile across hosts");

void surf_vm_model_init_HL13(simgrid::kernel::resource::CpuModel* cpu_pm_model)
{
  auto vm_model = std::make_shared<simgrid::kernel::resource::VMModel>("VM_HL13");
  auto* engine  = simgrid::kernel::EngineImpl::get_instance();

  engine->add_model(vm_model, {cpu_pm_model});
  std::shared_ptr<simgrid::kernel::resource::CpuModel> cpu_model_vm;

  auto cpu_optim = simgrid::config::get_value<std::string>("cpu/optim");
  if (cpu_optim == "TI") {
    cpu_model_vm = std::make_shared<simgrid::kernel::resource::CpuTiModel>("VmCpu_TI");
  } else {
    cpu_model_vm = std::make_shared<simgrid::kernel::resource::CpuCas01Model>("VmCpu_Cas01");
  }
  engine->add_model(cpu_model_vm, {cpu_pm_model, vm_model.get()});
  engine->get_netzone_root()->set_cpu_vm_model(cpu_model_vm);
}

namespace simgrid {
template class xbt::Extendable<kernel::resource::VirtualMachineImpl>;

namespace kernel {
namespace resource {

/*********
 * Model *
 *********/

std::deque<s4u::VirtualMachine*> VirtualMachineImpl::allVms_;

/* In the real world, processes on the guest operating system will be somewhat degraded due to virtualization overhead.
 * The total CPU share these processes get is smaller than that of the VM process gets on a host operating system.
 * FIXME: add a configuration flag for this
 */
const double virt_overhead = 1; // 0.95

static void host_state_change(s4u::Host const& host)
{
  if (not host.is_on()) { // just turned off.
    std::vector<s4u::VirtualMachine*> trash;
    /* Find all VMs living on that host */
    for (s4u::VirtualMachine* const& vm : VirtualMachineImpl::allVms_)
      if (vm->get_pm() == &host)
        trash.push_back(vm);
    for (s4u::VirtualMachine* vm : trash)
      vm->shutdown();
  }
}

static void add_active_exec(s4u::Exec const& task)
{
  const s4u::VirtualMachine* vm = dynamic_cast<s4u::VirtualMachine*>(task.get_host());
  if (vm != nullptr) {
    VirtualMachineImpl* vm_impl = vm->get_vm_impl();
    vm_impl->add_active_exec();
    vm_impl->update_action_weight();
  }
}

static void remove_active_exec(s4u::Activity& task)
{
  const auto* exec = dynamic_cast<s4u::Exec*>(&task);
  if (exec == nullptr)
    return;
  if (not exec->is_assigned())
    return;
  const s4u::VirtualMachine* vm = dynamic_cast<s4u::VirtualMachine*>(exec->get_host());
  if (vm != nullptr) {
    VirtualMachineImpl* vm_impl = vm->get_vm_impl();
    vm_impl->remove_active_exec();
    vm_impl->update_action_weight();
  }
}

static s4u::VirtualMachine* get_vm_from_activity(kernel::activity::ActivityImpl const& act)
{
  auto* exec = dynamic_cast<kernel::activity::ExecImpl const*>(&act);
  return exec != nullptr ? dynamic_cast<s4u::VirtualMachine*>(exec->get_host()) : nullptr;
}

static void add_active_activity(kernel::activity::ActivityImpl const& act)
{
  const s4u::VirtualMachine* vm = get_vm_from_activity(act);
  if (vm != nullptr) {
    VirtualMachineImpl* vm_impl = vm->get_vm_impl();
    vm_impl->add_active_exec();
    vm_impl->update_action_weight();
  }
}

static void remove_active_activity(kernel::activity::ActivityImpl const& act)
{
  const s4u::VirtualMachine* vm = get_vm_from_activity(act);
  if (vm != nullptr) {
    VirtualMachineImpl* vm_impl = vm->get_vm_impl();
    vm_impl->remove_active_exec();
    vm_impl->update_action_weight();
  }
}

VMModel::VMModel(const std::string& name) : HostModel(name)
{
  s4u::Host::on_state_change_cb(host_state_change);
  s4u::Exec::on_start_cb(add_active_exec);
  s4u::Activity::on_completion_cb(remove_active_exec);
  activity::ActivityImpl::on_resumed.connect(add_active_activity);
  activity::ActivityImpl::on_suspended.connect(remove_active_activity);
}

double VMModel::next_occurring_event(double now)
{
  /* TODO: update action's cost with the total cost of processes on the VM. */

  /* 1. Now we know how many resource should be assigned to each virtual
   * machine. We update constraints of the virtual machine layer.
   *
   * If we have two virtual machine (VM1 and VM2) on a physical machine (PM1).
   *     X1 + X2 = C       (Equation 1)
   * where
   *    the resource share of VM1: X1
   *    the resource share of VM2: X2
   *    the capacity of PM1: C
   *
   * Then, if we have two process (P1 and P2) on VM1.
   *     X1_1 + X1_2 = X1  (Equation 2)
   * where
   *    the resource share of P1: X1_1
   *    the resource share of P2: X1_2
   *    the capacity of VM1: X1
   *
   * Equation 1 was solved in the physical machine layer.
   * Equation 2 is solved in the virtual machine layer (here).
   * X1 must be passed to the virtual machine layer as a constraint value.
   **/

  /* iterate for all virtual machines */
  for (s4u::VirtualMachine* const& ws_vm : VirtualMachineImpl::allVms_) {
    if (ws_vm->get_state() == s4u::VirtualMachine::State::SUSPENDED) // Ignore suspended VMs
      continue;

    const kernel::resource::CpuImpl* cpu = ws_vm->get_cpu();

    // solved_value below is X1 in comment above: what this VM got in the sharing on the PM
    double solved_value = ws_vm->get_vm_impl()->get_action()->get_rate();
    XBT_DEBUG("assign %f to vm %s @ pm %s", solved_value, ws_vm->get_cname(), ws_vm->get_pm()->get_cname());

    lmm::System* vcpu_system = cpu->get_model()->get_maxmin_system();
    vcpu_system->update_constraint_bound(cpu->get_constraint(), virt_overhead * solved_value);
  }
  /* actual next occurring event is determined by VM CPU model at EngineImpl::solve */
  return -1.0;
}

/************
 * Resource *
 ************/

VirtualMachineImpl::VirtualMachineImpl(const std::string& name, s4u::VirtualMachine* piface,
                                       simgrid::s4u::Host* host_PM, int core_amount, size_t ramsize)
    : HostImpl(name, piface), piface_(piface), physical_host_(host_PM), core_amount_(core_amount), ramsize_(ramsize)
{
  /* Register this VM to the list of all VMs */
  allVms_.push_back(piface);
  /* We create cpu_action corresponding to a VM process on the host operating system. */
  /* TODO: we have to periodically input GUESTOS_NOISE to the system? how ?
   * The value for GUESTOS_NOISE corresponds to the cost of the global action associated to the VM.  It corresponds to
   * the cost of a VM running no tasks.
   */
  action_ = physical_host_->get_cpu()->execution_start(0, core_amount_);

  // It's empty for now, so it should not request resources in the PM
  update_action_weight();

  XBT_VERB("Create VM(%s)@PM(%s)", name.c_str(), physical_host_->get_cname());
}

/** @brief A physical host does not disappear in the current SimGrid code, but a VM may disappear during a simulation */
void VirtualMachineImpl::vm_destroy()
{
  s4u::VirtualMachine::on_destruction(*piface_);
  /* I was already removed from the allVms set if the VM was destroyed cleanly */
  auto iter = find(allVms_.begin(), allVms_.end(), piface_);
  if (iter != allVms_.end())
    allVms_.erase(iter);

  /* Free the cpu_action of the VM. */
  XBT_ATTRIB_UNUSED bool ret = action_->unref();
  xbt_assert(ret, "Bug: some resource still remains");
}

void VirtualMachineImpl::suspend(smx_actor_t issuer)
{
  if (vm_state_ != s4u::VirtualMachine::State::RUNNING)
    throw VmFailureException(XBT_THROW_POINT,
                             xbt::string_printf("Cannot suspend VM %s: it is not running.", piface_->get_cname()));
  if (issuer->get_host() == piface_)
    throw VmFailureException(XBT_THROW_POINT, xbt::string_printf("Actor %s cannot suspend the VM %s in which it runs",
                                                                 issuer->get_cname(), piface_->get_cname()));

  XBT_DEBUG("suspend VM(%s), where %zu actors exist", piface_->get_cname(), get_actor_count());

  action_->suspend();

  foreach_actor([](auto& actor) {
    XBT_DEBUG("suspend %s", actor.get_cname());
    actor.suspend();
  });

  XBT_DEBUG("suspend all actors on the VM done done");

  vm_state_ = s4u::VirtualMachine::State::SUSPENDED;
}

void VirtualMachineImpl::resume()
{
  if (vm_state_ != s4u::VirtualMachine::State::SUSPENDED)
    throw VmFailureException(XBT_THROW_POINT,
                             xbt::string_printf("Cannot resume VM %s: it was not suspended", piface_->get_cname()));

  XBT_DEBUG("Resume VM %s, containing %zu actors.", piface_->get_cname(), get_actor_count());

  action_->resume();

  foreach_actor([](auto& actor) {
    XBT_DEBUG("resume %s", actor.get_cname());
    actor.resume();
  });

  vm_state_ = s4u::VirtualMachine::State::RUNNING;
}

/** @brief Power off a VM.
 *
 * All hosted processes will be killed, but the VM state is preserved on memory.
 * It can later be restarted.
 *
 * @param issuer the actor requesting the shutdown
 */
void VirtualMachineImpl::shutdown(smx_actor_t issuer)
{
  if (vm_state_ != s4u::VirtualMachine::State::RUNNING)
    XBT_VERB("Shutting down the VM %s even if it's not running but in state %s", piface_->get_cname(),
             s4u::VirtualMachine::to_c_str(get_state()));

  XBT_DEBUG("shutdown VM %s, that contains %zu actors", piface_->get_cname(), get_actor_count());

  foreach_actor([issuer](auto& actor) {
    XBT_DEBUG("kill %s@%s on behalf of %s which shutdown that VM.", actor.get_cname(), actor.get_host()->get_cname(),
              issuer->get_cname());
    issuer->kill(&actor);
  });

  set_state(s4u::VirtualMachine::State::DESTROYED);

  /* FIXME: we may have to do something at the surf layer, e.g., vcpu action */
}

/** @brief Change the physical host on which the given VM is running
 *
 * This is an instantaneous migration.
 */
void VirtualMachineImpl::set_physical_host(s4u::Host* destination)
{
  std::string vm_name     = piface_->get_name();
  std::string pm_name_src = physical_host_->get_name();
  std::string pm_name_dst = destination->get_name();

  /* update net_elm with that of the destination physical host */
  piface_->set_netpoint(destination->get_netpoint());

  /* Adapt the speed, pstate and other physical characteristics to the one of our new physical CPU */
  piface_->get_cpu()->reset_vcpu(destination->get_cpu());

  physical_host_ = destination;

  /* Update vcpu's action for the new pm */
  /* create a cpu action bound to the pm model at the destination. */
  CpuAction* new_cpu_action = destination->get_cpu()->execution_start(0, this->core_amount_);

  if (action_->get_remains_no_update() > 0)
    XBT_CRITICAL("FIXME: need copy the state(?), %f", action_->get_remains_no_update());

  /* keep the bound value of the cpu action of the VM. */
  double old_bound = action_->get_bound();
  if (old_bound > 0) {
    XBT_DEBUG("migrate VM(%s): set bound (%f) at %s", vm_name.c_str(), old_bound, pm_name_dst.c_str());
    new_cpu_action->set_bound(old_bound);
  }

  XBT_ATTRIB_UNUSED bool ret = action_->unref();
  xbt_assert(ret, "Bug: some resource still remains");

  action_ = new_cpu_action;

  XBT_DEBUG("migrate VM(%s): change PM (%s to %s)", vm_name.c_str(), pm_name_src.c_str(), pm_name_dst.c_str());
}

void VirtualMachineImpl::set_bound(double bound)
{
  user_bound_ = bound;
  action_->set_user_bound(user_bound_);
  update_action_weight();
}

void VirtualMachineImpl::update_action_weight()
{
  /* The impact of the VM over its PM is the min between its vCPU amount and the amount of tasks it contains */
  int impact = std::min(active_execs_, get_core_amount());

  XBT_DEBUG("set the weight of the dummy CPU action of VM%p on PM to %d (#tasks: %u)", this, impact, active_execs_);

  if (impact > 0)
    action_->set_sharing_penalty(1. / impact);
  else
    action_->set_sharing_penalty(0.);

  action_->set_bound(std::min(impact * physical_host_->get_speed(), user_bound_));
}

} // namespace resource
} // namespace kernel
} // namespace simgrid
