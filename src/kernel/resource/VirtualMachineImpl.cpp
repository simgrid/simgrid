/* Copyright (c) 2013-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/Exception.hpp>
#include <simgrid/kernel/routing/NetPoint.hpp>
#include <simgrid/kernel/routing/NetZoneImpl.hpp>
#include <simgrid/s4u/Exec.hpp>

#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/activity/ExecImpl.hpp"
#include "src/kernel/resource/VirtualMachineImpl.hpp"
#include "src/kernel/resource/models/cpu_cas01.hpp"
#include "src/kernel/resource/models/cpu_ti.hpp"
#include "src/simgrid/module.hpp"
#include "src/simgrid/sg_config.hpp"

#include <numeric>
#include <unordered_map>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(res_vm, ker_resource, "Virtual Machines, containing actors and mobile across hosts");

void simgrid_vm_model_init_HL13()
{
  auto* cpu_pm_model = simgrid::s4u::Engine::get_instance()->get_netzone_root()->get_impl()->get_cpu_pm_model().get();
  auto vm_model = std::make_shared<simgrid::kernel::resource::VMModel>("VM_HL13");
  auto* engine  = simgrid::kernel::EngineImpl::get_instance();

  engine->add_model(vm_model, {cpu_pm_model});
  std::shared_ptr<simgrid::kernel::resource::CpuModel> cpu_model_vm;

  if (simgrid::config::get_value<std::string>("cpu/optim") == "TI") {
    cpu_model_vm = std::make_shared<simgrid::kernel::resource::CpuTiModel>("VmCpu_TI");
  } else {
    cpu_model_vm = std::make_shared<simgrid::kernel::resource::CpuCas01Model>("VmCpu_Cas01");
  }
  engine->add_model(cpu_model_vm, {cpu_pm_model, vm_model.get()});
  engine->get_netzone_root()->set_cpu_vm_model(cpu_model_vm);
}

namespace simgrid {
template class xbt::Extendable<kernel::resource::VirtualMachineImpl>;

namespace kernel::resource {

/*********
 * Model *
 *********/

std::deque<s4u::VirtualMachine*> VirtualMachineImpl::allVms_;
xbt::signal<void(VirtualMachineImpl const&, s4u::Host*)> VirtualMachineImpl::on_physical_host_change;

/* In the real world, processes on the guest operating system will be somewhat degraded due to virtualization overhead.
 * The total CPU share these processes get is smaller than that of the VM process gets on a host operating system.
 * FIXME: add a configuration flag for this
 */
const double virt_overhead = 1; // 0.95

static void host_onoff(s4u::Host const& host)
{
  if (not host.is_on()) { // just turned off.
    std::vector<s4u::VirtualMachine*> trash;
    /* Find all VMs living on that host */
    for (auto* vm : VirtualMachineImpl::allVms_)
      if (vm->get_pm() == &host)
        trash.push_back(vm);
    for (auto* vm : trash)
      vm->shutdown();
  }
}

/* Accounting of the execs running within each VM, used to compute the VM's impact on its PM.
 *
 * An exec counts for its amount of threads in the VM it runs on, while it is started and not suspended. We remember
 * where each exec is accounted so that every operation below is idempotent: the signals used to maintain this
 * accounting are not always fired in a convenient order (an exec started in the suspended state is suspended before
 * being started; an exec can be suspended both by its actor and directly; an actor migration moves its execs), and a
 * mismatch would make the counter of the VM drift or underflow. */
namespace {
struct AccountedExec {
  VirtualMachineImpl* vm;
  int thread_count;
  double bound;
};
std::unordered_map<const activity::ExecImpl*, AccountedExec> accounted_execs;

const activity::ExecImpl* get_exec_impl(s4u::Exec const& exec)
{
  return static_cast<const activity::ExecImpl*>(exec.get_impl());
}

void account_exec(const activity::ExecImpl* exec, const s4u::Host* host)
{
  const auto* vm = dynamic_cast<const s4u::VirtualMachine*>(host);
  if (vm == nullptr || accounted_execs.find(exec) != accounted_execs.end())
    return;
  VirtualMachineImpl* vm_impl = vm->get_vm_impl();
  // The bound of multi-threaded execs is ignored by the CPU models (see ExecImpl::start()), so ignore it here too
  double bound = exec->get_thread_count() == 1 ? exec->get_bound() : 0.0;
  vm_impl->add_active_exec(exec->get_thread_count(), bound);
  vm_impl->update_action_weight();
  accounted_execs.try_emplace(exec, AccountedExec{vm_impl, exec->get_thread_count(), bound});
}

void unaccount_exec(const activity::ExecImpl* exec)
{
  auto it = accounted_execs.find(exec);
  if (it == accounted_execs.end())
    return;
  auto [vm_impl, thread_count, bound] = it->second;
  accounted_execs.erase(it);
  vm_impl->remove_active_exec(thread_count, bound);
  vm_impl->update_action_weight();
}

void on_exec_start(s4u::Exec const& exec)
{
  const activity::ExecImpl* impl = get_exec_impl(exec);
  xbt_assert(accounted_execs.find(impl) == accounted_execs.end(), "Exec %s is started while already accounted in a VM",
             impl->get_cname());
  if (not exec.is_suspended()) // An exec started in the suspended state will be accounted when resumed
    account_exec(impl, exec.get_host());
}

/* We cannot rely on s4u::Exec::on_completion here: it is fired only when someone waits for the exec (or when it is
 * detached), not when the exec ends unobserved or when its actor gets killed. Such an exec would remain accounted. */
void on_exec_termination(activity::ExecImpl const& exec)
{
  unaccount_exec(&exec);
}

void on_exec_suspend(s4u::Exec const& exec)
{
  unaccount_exec(get_exec_impl(exec));
}

void on_exec_resume(s4u::Exec const& exec)
{
  // Exec::start() may resume an exec that is not started yet: on_exec_start() will account it if needed
  if (exec.get_state() == s4u::Activity::State::STARTED)
    account_exec(get_exec_impl(exec), exec.get_host());
}

void on_exec_migration(activity::ExecImpl const& exec, s4u::Host* to)
{
  unaccount_exec(&exec);
  // A suspended exec will be accounted on its new host when resumed. The model action was already moved to the new
  // host when this signal is fired, keeping its suspension state.
  if (exec.model_action_ == nullptr || not exec.model_action_->is_suspended())
    account_exec(&exec, to);
}

/* Forget the execs accounted on a VM that is being destroyed, to not touch its counter afterward */
void forget_accounted_execs(const VirtualMachineImpl* vm_impl)
{
  std::erase_if(accounted_execs, [vm_impl](auto const& entry) { return entry.second.vm == vm_impl; });
}
} // namespace

VMModel::VMModel(const std::string& name) : HostModel(name)
{
  s4u::Host::on_onoff_cb(host_onoff);
  s4u::Exec::on_start_cb(on_exec_start);
  activity::ExecImpl::on_termination.connect(on_exec_termination);
  s4u::Exec::on_suspend_cb(on_exec_suspend);
  s4u::Exec::on_resume_cb(on_exec_resume);
  activity::ExecImpl::on_migration.connect(on_exec_migration);
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
  for (auto const* ws_vm : VirtualMachineImpl::allVms_) {
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

Action* VMModel::execute_thread(const s4u::Host* host, double flops_amount, int thread_count)
{
  auto* cpu = host->get_cpu();
  return cpu->execution_start(thread_count * flops_amount, thread_count, -1);
}

/************
 * Resource *
 ************/

VirtualMachineImpl::VirtualMachineImpl(const std::string& name, s4u::VirtualMachine* piface,
                                       simgrid::s4u::Host* host_PM, int core_amount, size_t ramsize)
    : VirtualMachineImpl(name, host_PM, core_amount, ramsize)
{
  set_piface(piface);
}

VirtualMachineImpl::VirtualMachineImpl(const std::string& name, simgrid::s4u::Host* host_PM, int core_amount,
                                       size_t ramsize)
    : HostImpl(name), physical_host_(host_PM), core_amount_(core_amount), ramsize_(ramsize)
{
  /* We create cpu_action corresponding to a VM process on the host operating system. */
  /* TODO: we have to periodically input GUESTOS_NOISE to the system? how ?
   * The value for GUESTOS_NOISE corresponds to the cost of the global action associated to the VM.  It corresponds to
   * the cost of a VM running no tasks.
   */
  action_ = physical_host_->get_cpu()->execution_start(0, core_amount_, 0);

  // It's empty for now, so it should not request resources in the PM
  update_action_weight();
  XBT_VERB("Create VM(%s)@PM(%s)", name.c_str(), physical_host_->get_cname());
}

void VirtualMachineImpl::set_piface(s4u::VirtualMachine* piface)
{
  xbt_assert(not piface_, "Pointer to interface already configured for this VM (%s)", get_cname());
  piface_ = piface;
  /* Register this VM to the list of all VMs */
  allVms_.push_back(piface);
}

/** @brief A physical host does not disappear in the current SimGrid code, but a VM may disappear during a simulation */
void VirtualMachineImpl::vm_destroy()
{
  /* I was already removed from the allVms set if the VM was destroyed cleanly */
  if (auto iter = find(allVms_.begin(), allVms_.end(), piface_); iter != allVms_.end())
    allVms_.erase(iter);

  forget_accounted_execs(this);

  /* Free the cpu_action of the VM. */
  XBT_ATTRIB_UNUSED bool ret = action_->unref();
  xbt_assert(ret, "Bug: some resource still remains");

  // VM uses the host's netpoint, clean but don't destroy it
  get_iface()->set_netpoint(nullptr);
  // Take a temporary copy to delete iface safely after impl is destroy'ed
  const auto* iface = get_iface();
  // calls the HostImpl() destroy, it'll delete the impl object
  destroy();

  delete iface;
}

void VirtualMachineImpl::start()
{
  s4u::VirtualMachine::on_start(*get_iface());
  get_iface()->on_this_start(*get_iface());
  s4u::VmHostExt::ensureVmExtInstalled();

  if (physical_host_->extension<s4u::VmHostExt>() == nullptr)
    physical_host_->extension_set(new s4u::VmHostExt());

  if (size_t pm_ramsize = physical_host_->extension<s4u::VmHostExt>()->ramsize;
      pm_ramsize &&
      not physical_host_->extension<s4u::VmHostExt>()->overcommit) { /* Need to verify that we don't overcommit */
    /* Retrieve the memory occupied by the VMs on that host. Yep, we have to traverse all VMs of all hosts for that */
    size_t total_ramsize_of_vms = 0;
    for (auto const* ws_vm : allVms_)
      if (physical_host_ == ws_vm->get_pm())
        total_ramsize_of_vms += ws_vm->get_ramsize();

    if (total_ramsize_of_vms + get_ramsize() > pm_ramsize) {
      XBT_WARN("cannot start %s@%s due to memory shortage: get_ramsize() %zu, free %zu, pm_ramsize %zu (bytes).",
               get_cname(), physical_host_->get_cname(), get_ramsize(), pm_ramsize - total_ramsize_of_vms, pm_ramsize);
      throw VmFailureException(XBT_THROW_POINT,
                               xbt::string_printf("Memory shortage on host '%s', VM '%s' cannot be started",
                                                  physical_host_->get_cname(), get_cname()));
    }
  }
  vm_state_ = s4u::VirtualMachine::State::RUNNING;

  s4u::VirtualMachine::on_started(*get_iface());
  get_iface()->on_this_started(*get_iface());
}

void VirtualMachineImpl::suspend(const actor::ActorImpl* issuer)
{
  s4u::VirtualMachine::on_suspend(*get_iface());
  get_iface()->on_this_suspend(*get_iface());

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
  s4u::VirtualMachine::on_resume(*get_iface());
  get_iface()->on_this_resume(*get_iface());
}

/** @brief Power off a VM.
 *
 * All hosted processes will be killed, but the VM state is preserved on memory.
 * It can later be restarted.
 *
 * @param issuer the actor requesting the shutdown
 */
void VirtualMachineImpl::shutdown(actor::ActorImpl* issuer)
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

  s4u::VirtualMachine::on_shutdown(*get_iface());
  get_iface()->on_this_shutdown(*get_iface());
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

  on_physical_host_change(*this, destination);

  /* update net_elm with that of the destination physical host */
  piface_->set_netpoint(destination->get_netpoint());
  physical_host_->get_impl()->move_vm(this, destination->get_impl());

  /* Adapt the speed, pstate and other physical characteristics to the one of our new physical CPU */
  piface_->get_cpu()->reset_vcpu(destination->get_cpu());

  physical_host_ = destination;

  /* Update vcpu's action for the new pm */
  /* create a cpu action bound to the pm model at the destination. */
  CpuAction* new_cpu_action = destination->get_cpu()->execution_start(0, this->core_amount_);

  if (action_->get_remains_no_update() > 0)
    XBT_CRITICAL("FIXME: need copy the state(?), %f", action_->get_remains_no_update());

  XBT_ATTRIB_UNUSED bool ret = action_->unref();
  xbt_assert(ret, "Bug: some resource still remains");

  action_ = new_cpu_action;
  action_->set_user_bound(user_bound_);
  update_action_weight();

  XBT_DEBUG("migrate VM(%s): change PM (%s to %s)", vm_name.c_str(), pm_name_src.c_str(), pm_name_dst.c_str());
}

void VirtualMachineImpl::set_bound(double bound)
{
  user_bound_ = bound;
  action_->set_user_bound(user_bound_);
  update_action_weight();
}

void VirtualMachineImpl::add_active_exec(int thread_count, double bound)
{
  if (bound > 0)
    active_bounds_.insert(bound);
  else
    active_threads_ += thread_count;
}

void VirtualMachineImpl::remove_active_exec(int thread_count, double bound)
{
  if (bound > 0) {
    auto it = active_bounds_.find(bound);
    xbt_assert(it != active_bounds_.end(), "VM %s: removing a bounded exec that is not accounted as active",
               get_cname());
    active_bounds_.erase(it);
  } else {
    xbt_assert(active_threads_ >= static_cast<unsigned>(thread_count),
               "VM %s: removing more threads than accounted as active", get_cname());
    active_threads_ -= thread_count;
  }
}

double VirtualMachineImpl::get_active_load() const
{
  // A bounded exec uses the fraction of a core given by its bound, and never more than one core. The bounds are
  // converted with the current speed of the PM, which may have changed since the exec started.
  double core_speed = physical_host_->get_speed();
  double load       = active_threads_;
  for (double bound : active_bounds_)
    load += std::min(1.0, bound / core_speed);
  return load;
}

void VirtualMachineImpl::update_action_weight()
{
  /* The VM competes on its PM like the execs it contains, up to its vCPU amount: each exec weighs for one competitor
   * in the sharing, whatever its bound, exactly as if it was running directly on the PM. */
  double weight =
      std::min(static_cast<double>(active_threads_ + active_bounds_.size()), static_cast<double>(get_core_amount()));
  /* But the VM cannot use more cores than what its execs need: a bounded exec only needs a fraction of a core. */
  double load = std::min(get_active_load(), static_cast<double>(get_core_amount()));

  XBT_DEBUG("set the weight of the dummy CPU action of VM%p on PM to %g and its bound to %g cores (#unbounded threads: "
            "%u, #bounded execs: %zu)",
            this, weight, load, active_threads_, active_bounds_.size());

  if (weight > 0)
    action_->set_sharing_penalty(1. / weight);
  else
    action_->set_sharing_penalty(0.);

  action_->set_bound(std::min(load * physical_host_->get_speed(), user_bound_));
}

void VirtualMachineImpl::start_migration()
{
  is_migrating_ = true;
  s4u::VirtualMachine::on_migration_start(*get_iface());
  get_iface()->on_this_migration_start(*get_iface());
}

void VirtualMachineImpl::end_migration()
{
  is_migrating_ = false;
  s4u::VirtualMachine::on_migration_end(*get_iface());
  get_iface()->on_this_migration_end(*get_iface());
}

void VirtualMachineImpl::seal()
{
  HostImpl::seal();
  s4u::VirtualMachine::on_vm_creation(*get_iface());
}

} // namespace kernel::resource
} // namespace simgrid
