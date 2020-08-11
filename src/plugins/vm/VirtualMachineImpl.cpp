/* Copyright (c) 2013-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/plugins/vm/VirtualMachineImpl.hpp"
#include "simgrid/Exception.hpp"
#include "simgrid/s4u/Exec.hpp"
#include "src/include/surf/surf.hpp"
#include "src/kernel/activity/ExecImpl.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_vm, surf, "Logging specific to the SURF VM module");

simgrid::vm::VMModel* surf_vm_model = nullptr;

void surf_vm_model_init_HL13()
{
  if (surf_cpu_model_vm != nullptr)
    surf_vm_model = new simgrid::vm::VMModel();
}

namespace simgrid {

template class xbt::Extendable<vm::VirtualMachineImpl>;

namespace vm {
/*************
 * Callbacks *
 *************/
xbt::signal<void(VirtualMachineImpl&)> VirtualMachineImpl::on_creation;
xbt::signal<void(VirtualMachineImpl const&)> VirtualMachineImpl::on_destruction;

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

static void add_active_exec(s4u::Actor const&, s4u::Exec const& task)
{
  const s4u::VirtualMachine* vm = dynamic_cast<s4u::VirtualMachine*>(task.get_host());
  if (vm != nullptr) {
    VirtualMachineImpl* vm_impl = vm->get_impl();
    vm_impl->add_active_exec();
    vm_impl->update_action_weight();
  }
}

static void remove_active_exec(s4u::Actor const&, s4u::Exec const& task)
{
  const s4u::VirtualMachine* vm = dynamic_cast<s4u::VirtualMachine*>(task.get_host());
  if (vm != nullptr) {
    VirtualMachineImpl* vm_impl = vm->get_impl();
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
    VirtualMachineImpl *vm_impl = vm->get_impl();
    vm_impl->add_active_exec();
    vm_impl->update_action_weight();
  }
}

static void remove_active_activity(kernel::activity::ActivityImpl const& act)
{
  const s4u::VirtualMachine* vm = get_vm_from_activity(act);
  if (vm != nullptr) {
    VirtualMachineImpl *vm_impl = vm->get_impl();
    vm_impl->remove_active_exec();
    vm_impl->update_action_weight();
  }
}

VMModel::VMModel()
{
  all_existing_models.push_back(this);
  s4u::Host::on_state_change.connect(host_state_change);
  s4u::Exec::on_start.connect(add_active_exec);
  s4u::Exec::on_completion.connect(remove_active_exec);
  kernel::activity::ActivityImpl::on_resumed.connect(add_active_activity);
  kernel::activity::ActivityImpl::on_suspended.connect(remove_active_activity);
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
    if (ws_vm->get_state() == s4u::VirtualMachine::state::SUSPENDED) // Ignore suspended VMs
      continue;

    const kernel::resource::Cpu* cpu = ws_vm->pimpl_cpu;

    // solved_value below is X1 in comment above: what this VM got in the sharing on the PM
    double solved_value = ws_vm->get_impl()->get_action()->get_variable()->get_value();
    XBT_DEBUG("assign %f to vm %s @ pm %s", solved_value, ws_vm->get_cname(), ws_vm->get_pm()->get_cname());

    xbt_assert(cpu->get_model() == surf_cpu_model_vm);
    kernel::lmm::System* vcpu_system = cpu->get_model()->get_maxmin_system();
    vcpu_system->update_constraint_bound(cpu->get_constraint(), virt_overhead * solved_value);
  }

  /* 2. Ready. Get the next occurring event */
  return surf_cpu_model_vm->next_occurring_event(now);
}

/************
 * Resource *
 ************/

VirtualMachineImpl::VirtualMachineImpl(simgrid::s4u::VirtualMachine* piface, simgrid::s4u::Host* host_PM,
                                       int core_amount, size_t ramsize)
    : HostImpl(piface), physical_host_(host_PM), core_amount_(core_amount), ramsize_(ramsize)
{
  /* Register this VM to the list of all VMs */
  allVms_.push_back(piface);

  /* We create cpu_action corresponding to a VM process on the host operating system. */
  /* TODO: we have to periodically input GUESTOS_NOISE to the system? how ?
   * The value for GUESTOS_NOISE corresponds to the cost of the global action associated to the VM.  It corresponds to
   * the cost of a VM running no tasks.
   */
  action_ = host_PM->pimpl_cpu->execution_start(0, core_amount);

  // It's empty for now, so it should not request resources in the PM
  update_action_weight();

  XBT_VERB("Create VM(%s)@PM(%s)", piface->get_cname(), physical_host_->get_cname());
  on_creation(*this);
}

/** @brief A physical host does not disappear in the current SimGrid code, but a VM may disappear during a simulation */
VirtualMachineImpl::~VirtualMachineImpl()
{
  on_destruction(*this);
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
  if (get_state() != s4u::VirtualMachine::state::RUNNING)
    throw VmFailureException(XBT_THROW_POINT,
                             xbt::string_printf("Cannot suspend VM %s: it is not running.", piface_->get_cname()));
  if (issuer->get_host() == piface_)
    throw VmFailureException(XBT_THROW_POINT, xbt::string_printf("Actor %s cannot suspend the VM %s in which it runs",
                                                                 issuer->get_cname(), piface_->get_cname()));

  XBT_DEBUG("suspend VM(%s), where %zu actors exist", piface_->get_cname(), get_actor_count());

  action_->suspend();

  for (auto& actor : actor_list_) {
    XBT_DEBUG("suspend %s", actor.get_cname());
    actor.suspend();
  }

  XBT_DEBUG("suspend all actors on the VM done done");

  vm_state_ = s4u::VirtualMachine::state::SUSPENDED;
}

void VirtualMachineImpl::resume()
{
  if (get_state() != s4u::VirtualMachine::state::SUSPENDED)
    throw VmFailureException(XBT_THROW_POINT,
                             xbt::string_printf("Cannot resume VM %s: it was not suspended", piface_->get_cname()));

  XBT_DEBUG("Resume VM %s, containing %zu actors.", piface_->get_cname(), get_actor_count());

  action_->resume();

  for (auto& actor : actor_list_) {
    XBT_DEBUG("resume %s", actor.get_cname());
    actor.resume();
  }

  vm_state_ = s4u::VirtualMachine::state::RUNNING;
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
  if (get_state() != s4u::VirtualMachine::state::RUNNING) {
    const char* stateName = "(unknown state)";
    switch (get_state()) {
      case s4u::VirtualMachine::state::CREATED:
        stateName = "created, but not yet started";
        break;
      case s4u::VirtualMachine::state::SUSPENDED:
        stateName = "suspended";
        break;
      case s4u::VirtualMachine::state::DESTROYED:
        stateName = "destroyed";
        break;
      default: /* SURF_VM_STATE_RUNNING or unexpected values */
        THROW_IMPOSSIBLE;
    }
    XBT_VERB("Shutting down the VM %s even if it's not running but %s", piface_->get_cname(), stateName);
  }

  XBT_DEBUG("shutdown VM %s, that contains %zu actors", piface_->get_cname(), get_actor_count());

  for (auto& actor : actor_list_) {
    XBT_DEBUG("kill %s@%s on behalf of %s which shutdown that VM.", actor.get_cname(), actor.get_host()->get_cname(),
              issuer->get_cname());
    issuer->kill(&actor);
  }

  set_state(s4u::VirtualMachine::state::DESTROYED);

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

  physical_host_ = destination;

  /* Update vcpu's action for the new pm */
  /* create a cpu action bound to the pm model at the destination. */
  kernel::resource::CpuAction* new_cpu_action = destination->pimpl_cpu->execution_start(0, this->core_amount_);

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
  update_action_weight();
}

void VirtualMachineImpl::update_action_weight(){
  /* The impact of the VM over its PM is the min between its vCPU amount and the amount of tasks it contains */
  int impact = std::min(active_execs_, get_core_amount());

  XBT_DEBUG("set the weight of the dummy CPU action of VM%p on PM to %d (#tasks: %u)", this, impact, active_execs_);

  if (impact > 0)
    action_->set_sharing_penalty(1. / impact);
  else
    action_->set_sharing_penalty(0.);

  action_->set_bound(std::min(impact * physical_host_->get_speed(), user_bound_));
}

}
}
