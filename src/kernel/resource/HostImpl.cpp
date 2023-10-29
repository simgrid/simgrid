/* Copyright (c) 2013-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/kernel/routing/NetPoint.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Host.hpp>

#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/resource/NetworkModel.hpp"
#include "src/kernel/resource/VirtualMachineImpl.hpp"
#include "xbt/asserts.hpp"

#include <string>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(res_host, ker_resource, "Host resources agregate CPU, networking and I/O features");

/*************
 * Callbacks *
 *************/

namespace simgrid::kernel::resource {

/*********
 * Model *
 *********/
Action* HostModel::io_stream(s4u::Host* src_host, DiskImpl* src_disk, s4u::Host* dst_host, DiskImpl* dst_disk,
                                double size)
{
  auto* net_model = src_host->get_englobing_zone()->get_network_model();
  auto* system    = net_model->get_maxmin_system();
  auto* action   = net_model->communicate(src_host, dst_host, size, -1, true);

  // We don't want to apply the network model bandwidth factor to the I/O constraints
  double bw_factor = net_model->get_bandwidth_factor();
  if (src_disk != nullptr) {
    // FIXME: if the stream starts from a disk, we might not want to pay the network latency
    system->expand(src_disk->get_constraint(), action->get_variable(), bw_factor);
    system->expand(src_disk->get_read_constraint(), action->get_variable(), bw_factor);
  }
  if (dst_disk != nullptr) {
    system->expand(dst_disk->get_constraint(), action->get_variable(), bw_factor);
    system->expand(dst_disk->get_write_constraint(), action->get_variable(), bw_factor);
  }

  return action;
}

/************
 * Resource *
 ************/
HostImpl::HostImpl(const std::string& name) : piface_(this), name_(name)
{
  xbt_enforce(s4u::Host::by_name_or_null(name_) == nullptr, "Refusing to create a second host named '%s'.",
              get_cname());
}

HostImpl::~HostImpl()
{
  /* All actors should be gone when the host is turned off (by the end of the simulation). */
  if (not actor_list_.empty()) {
    const char* msg = "Shutting down host, but it's not empty";
    try {
      std::string actors;
      for (auto const& actor : actor_list_)
        actors += "\n\t" + actor.get_name();

      EngineImpl::get_instance()->display_all_actor_status();
      xbt_die("%s:%s", msg, actors.c_str());
    } catch (const std::bad_alloc& ba) {
      xbt_die("%s (cannot print actor list: %s)", msg, ba.what());
    }
  }
  for (auto const& arg : actors_at_boot_)
    delete arg;
  actors_at_boot_.clear();

  for (auto const& [_, vm] : vms_)
    vm->vm_destroy();
}

/** @brief Fire the required callbacks and destroy the object
 *
 * Don't delete directly a Host, call h->destroy() instead.
 */
void HostImpl::destroy()
{
  s4u::Host::on_destruction(*this->get_iface());
  this->get_iface()->on_this_destruction(*this->get_iface());
  delete this;
}

/** Re-starts all the actors that are marked as restartable.
 *
 * Weird things will happen if you turn on a host that is already on. S4U is fool-proof, not this.
 */
void HostImpl::turn_on() const
{
  for (auto const& arg : actors_at_boot_) {
    XBT_DEBUG("Booting Actor %s(%s) right now", arg->name.c_str(), arg->host->get_cname());
    actor::ActorImplPtr actor = actor::ActorImpl::create(arg);
  }
}

/** Kill all actors hosted here */
void HostImpl::turn_off(const actor::ActorImpl* issuer)
{
  /* turn_off VMs running on host */
  for (const auto& [_, vm] : vms_) {
    // call s4u functions to generate the good on_state_change signal, maybe one day this wont be necessary
    vm->get_iface()->shutdown();
    vm->get_iface()->turn_off();
  }
  for (auto& actor : actor_list_) {
    XBT_DEBUG("Killing Actor %s@%s on behalf of %s which turned off that host.", actor.get_cname(),
              actor.get_host()->get_cname(), issuer->get_cname());
    issuer->kill(&actor);
  }
  for (const auto& activity : EngineImpl::get_instance()->get_maestro()->activities_) {
    auto const& hosts = activity->get_hosts();
    if (std::find(hosts.begin(), hosts.end(), &piface_) != hosts.end()) {
      activity->cancel();
      activity->set_state(activity::State::FAILED);
    }
  }
  // When a host is turned off, we want to keep only the actors that should restart for when it will boot again.
  // Then get rid of the others.
  auto elm = remove_if(begin(actors_at_boot_), end(actors_at_boot_), [](const actor::ProcessArg* arg) {
    if (arg->auto_restart)
      return false;
    delete arg;
    return true;
  });
  actors_at_boot_.erase(elm, end(actors_at_boot_));
}

HostImpl* HostImpl::set_englobing_zone(routing::NetZoneImpl* englobing_zone)
{
  englobing_zone_ = englobing_zone;
  return this;
}

std::vector<s4u::ActorPtr> HostImpl::get_all_actors()
{
  std::vector<s4u::ActorPtr> res;
  for (auto& actor : actor_list_)
    res.emplace_back(actor.get_ciface());
  return res;
}

size_t HostImpl::get_actor_count() const
{
  return actor_list_.size();
}

std::vector<s4u::Disk*> HostImpl::get_disks() const
{
  std::vector<s4u::Disk*> disks;
  for (auto const& [_, d] : disks_)
    disks.push_back(d->get_iface());
  return disks;
}

s4u::VirtualMachine* HostImpl::create_vm(const std::string& name, int core_amount, size_t ramsize)
{
  auto* host_vm = new kernel::resource::VirtualMachineImpl(name, get_iface(), core_amount, ramsize);
  auto* vm      = new s4u::VirtualMachine(host_vm);
  host_vm->set_piface(vm);
  return create_vm(name, vm);
}

s4u::VirtualMachine* HostImpl::create_vm(const std::string& name, s4u::VirtualMachine* vm)
{
  vms_[name] = vm->get_vm_impl();

  // Create a VCPU for this VM
  std::vector<double> speeds;
  for (unsigned long i = 0; i < get_iface()->get_pstate_count(); i++)
    speeds.push_back(get_iface()->get_pstate_speed(i));

  auto* cpu =
      englobing_zone_->get_cpu_vm_model()->create_cpu(vm, speeds)->set_core_count(vm->get_vm_impl()->get_core_amount());

  cpu->seal();

  if (get_iface()->get_pstate() != 0) {
    cpu->set_pstate(get_iface()->get_pstate());
  }

  /* Currently, a VM uses the network resource of its physical host */
  vm->set_netpoint(get_iface()->get_netpoint());

  vm->seal();

  return vm;
}

void HostImpl::move_vm(VirtualMachineImpl* vm, HostImpl* destination)
{
  xbt_assert(vm && destination);

  vms_.erase(vm->get_name());
  destination->vms_[vm->get_name()] = vm;
}

void HostImpl::destroy_vm(const std::string& name)
{
  auto* vm = vms_[name];
  vms_.erase(name);
  vm->vm_destroy();
}

VirtualMachineImpl* HostImpl::get_vm_by_name_or_null(const std::string& name) const
{
  auto vm_it = vms_.find(name);
  return vm_it == vms_.end() ? nullptr : vm_it->second;
}

std::vector<s4u::VirtualMachine*> HostImpl::get_vms() const
{
  std::vector<s4u::VirtualMachine*> vms;
  for (const auto& [_, vm] : vms_) {
    vms.push_back(vm->get_iface());
  }
  return vms;
}

s4u::Disk* HostImpl::create_disk(const std::string& name, double read_bandwidth, double write_bandwidth)
{
  auto* disk = piface_.get_netpoint()->get_englobing_zone()->get_disk_model()->create_disk(name, read_bandwidth,
                                                                                           write_bandwidth);
  if (sealed_)
    disk->seal();
  return disk->set_host(&piface_)->get_iface();
}

void HostImpl::add_disk(const s4u::Disk* disk)
{
  disks_.insert({disk->get_name(), kernel::resource::DiskImplPtr(disk->get_impl())});
}

void HostImpl::remove_disk(const std::string& name)
{
  disks_.erase(name);
}

void HostImpl::seal()
{
  if (sealed_) {
    return;
  }
  // seals host's CPU
  get_iface()->get_cpu()->seal();
  sealed_ = true;

  /* seal its disks */
  for (auto const& [_, disk] : disks_)
    disk->seal();

  /* seal its VMs */
  for (auto const& [_, vm] : vms_)
    vm->seal();
}
} // namespace simgrid::kernel::resource
