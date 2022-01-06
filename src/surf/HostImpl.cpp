/* Copyright (c) 2013-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/kernel/routing/NetPoint.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Host.hpp>

#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/resource/VirtualMachineImpl.hpp"

#include <string>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(res_host, ker_resource, "Host resources agregate CPU, networking and I/O features");

/*************
 * Callbacks *t
 *************/

namespace simgrid {
namespace kernel {
namespace resource {

/*********
 * Model *
 *********/
/************
 * Resource *
 ************/
HostImpl::HostImpl(const std::string& name, s4u::Host* piface) : piface_(this), name_(name)
{
  xbt_assert(s4u::Host::by_name_or_null(name_) == nullptr, "Refusing to create a second host named '%s'.", get_cname());
  s4u::Engine::get_instance()->host_register(name_, piface);
}

HostImpl::HostImpl(const std::string& name) : piface_(this), name_(name)
{
  xbt_assert(s4u::Host::by_name_or_null(name_) == nullptr, "Refusing to create a second host named '%s'.", get_cname());
  s4u::Engine::get_instance()->host_register(name_, &piface_);
}

HostImpl::~HostImpl()
{
  /* All actors should be gone when the host is turned off (by the end of the simulation). */
  if (not actor_list_.empty()) {
    std::string msg = "Shutting down host, but it's not empty:";
    for (auto const& actor : actor_list_)
      msg += "\n\t" + std::string(actor.get_name());

    EngineImpl::get_instance()->display_all_actor_status();
    xbt_die("%s", msg.c_str());
  }
  for (auto const& arg : actors_at_boot_)
    delete arg;
  actors_at_boot_.clear();

  for (auto const& d : disks_)
    d.second->destroy();
}

/** @brief Fire the required callbacks and destroy the object
 *
 * Don't delete directly a Host, call h->destroy() instead.
 */
void HostImpl::destroy()
{
  s4u::Host::on_destruction(*this->get_iface());
  s4u::Engine::get_instance()->host_unregister(std::string(name_));
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
    actor::ActorImplPtr actor = actor::ActorImpl::create(arg->name, arg->code, nullptr, arg->host, nullptr);
    actor->set_properties(arg->properties);
    if (arg->on_exit)
      *actor->on_exit = *arg->on_exit;
    if (arg->kill_time >= 0)
      actor->set_kill_time(arg->kill_time);
    if (arg->auto_restart)
      actor->set_auto_restart(arg->auto_restart);
    if (arg->daemon_)
      actor->daemonize();
  }
}

/** Kill all actors hosted here */
void HostImpl::turn_off(const actor::ActorImpl* issuer)
{
  for (auto& actor : actor_list_) {
    XBT_DEBUG("Killing Actor %s@%s on behalf of %s which turned off that host.", actor.get_cname(),
              actor.get_host()->get_cname(), issuer->get_cname());
    issuer->kill(&actor);
  }
  for (const auto& activity : EngineImpl::get_instance()->get_maestro()->activities_) {
    auto* exec = dynamic_cast<activity::ExecImpl*>(activity.get());
    if (exec != nullptr) {
      auto hosts = exec->get_hosts();
      if (std::find(hosts.begin(), hosts.end(), &piface_) != hosts.end()) {
        exec->cancel();
        exec->state_ = activity::State::FAILED;
      }
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
  for (auto const& d : disks_)
    disks.push_back(d.second->get_iface());
  return disks;
}

s4u::Disk* HostImpl::create_disk(const std::string& name, double read_bandwidth, double write_bandwidth)
{
  auto disk = piface_.get_netpoint()->get_englobing_zone()->get_disk_model()->create_disk(name, read_bandwidth,
                                                                                          write_bandwidth);
  return disk->set_host(&piface_)->get_iface();
}

void HostImpl::add_disk(const s4u::Disk* disk)
{
  disks_[disk->get_name()] = disk->get_impl();
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
  for (auto const& disk : disks_)
    disk.second->seal();
}
} // namespace resource
} // namespace kernel
} // namespace simgrid
