/* Copyright (c) 2013-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/plugins/vm/VirtualMachineImpl.hpp"
#include "src/simix/smx_private.hpp"

#include <string>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_host, surf, "Logging specific to the SURF host module");

simgrid::surf::HostModel *surf_host_model = nullptr;

/*************
 * Callbacks *t
 *************/

namespace simgrid {
namespace surf {

/*********
 * Model *
 *********/
/************
 * Resource *
 ************/
HostImpl::HostImpl(s4u::Host* host) : piface_(host)
{
  /* The VM wants to reinstall a new HostImpl, but we don't want to leak the previously existing one */
  delete piface_->pimpl_;
  piface_->pimpl_ = this;
}

HostImpl::~HostImpl()
{
  /* All actors should be gone when the host is turned off (by the end of the simulation). */
  if (not actor_list_.empty()) {
    std::string msg = "Shutting down host, but it's not empty:";
    for (auto const& actor : actor_list_)
      msg += "\n\t" + std::string(actor.get_name());

    simix_global->display_all_actor_status();
    xbt_die("%s", msg.c_str());
  }
  for (auto const& arg : actors_at_boot_)
    delete arg;
  actors_at_boot_.clear();

  for (auto const& d : disks_)
    d->destroy();
}

/** Re-starts all the actors that are marked as restartable.
 *
 * Weird things will happen if you turn on a host that is already on. S4U is fool-proof, not this.
 */
void HostImpl::turn_on() const
{
  for (auto const& arg : actors_at_boot_) {
    XBT_DEBUG("Booting Actor %s(%s) right now", arg->name.c_str(), arg->host->get_cname());
    simgrid::kernel::actor::ActorImplPtr actor = simgrid::kernel::actor::ActorImpl::create(
        arg->name, arg->code, nullptr, arg->host, arg->properties.get(), nullptr);
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
void HostImpl::turn_off(const kernel::actor::ActorImpl* issuer)
{
  for (auto& actor : actor_list_) {
    XBT_DEBUG("Killing Actor %s@%s on behalf of %s which turned off that host.", actor.get_cname(),
              actor.get_host()->get_cname(), issuer->get_cname());
    issuer->kill(&actor);
  }
  // When a host is turned off, we want to keep only the actors that should restart for when it will boot again.
  // Then get rid of the others.
  auto elm = remove_if(begin(actors_at_boot_), end(actors_at_boot_), [](const kernel::actor::ProcessArg* arg) {
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
    disks.push_back(d->get_iface());
  return disks;
}

void HostImpl::set_disks(const std::vector<kernel::resource::DiskImpl*>& disks, s4u::Host* host)
{
  disks_ = std::move(disks);
  for (auto d : disks_)
    d->set_host(host);
}

void HostImpl::add_disk(const s4u::Disk* disk)
{
  disks_.push_back(disk->get_impl());
}

void HostImpl::remove_disk(const std::string& disk_name)
{
  auto position = disks_.begin();
  for (auto const& d : disks_) {
    if (d->get_name() == disk_name) {
      disks_.erase(position);
      break;
    }
    position++;
  }
}

std::vector<const char*> HostImpl::get_attached_storages()
{
  std::vector<const char*> storages;
  for (auto const& s : storage_)
    if (s.second->get_host() == piface_->get_cname())
      storages.push_back(s.second->get_iface()->get_cname());
  return storages;
}
std::unordered_map<std::string, s4u::Storage*>* HostImpl::get_mounted_storages()
{
  auto* mounts = new std::unordered_map<std::string, s4u::Storage*>();
  for (auto const& m : storage_) {
    mounts->insert({m.first, m.second->get_iface()});
  }
  return mounts;
}
} // namespace surf
} // namespace simgrid
