/* Copyright (c) 2013-2019. The SimGrid Team. All rights reserved.          */

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
/* Helper function for executeParallelTask */
static inline double has_cost(const double* array, size_t pos)
{
  if (array)
    return array[pos];
  return -1.0;
}

kernel::resource::Action* HostModel::execute_parallel(const std::vector<s4u::Host*>& host_list,
                                                      const double* flops_amount, const double* bytes_amount,
                                                      double rate)
{
  kernel::resource::Action* action = nullptr;
  if ((host_list.size() == 1) && (has_cost(bytes_amount, 0) <= 0) && (has_cost(flops_amount, 0) > 0)) {
    action = host_list[0]->pimpl_cpu->execution_start(flops_amount[0]);
  } else if ((host_list.size() == 1) && (has_cost(flops_amount, 0) <= 0)) {
    action = surf_network_model->communicate(host_list[0], host_list[0], bytes_amount[0], rate);
  } else if ((host_list.size() == 2) && (has_cost(flops_amount, 0) <= 0) && (has_cost(flops_amount, 1) <= 0)) {
    int nb = 0;
    double value = 0.0;

    for (size_t i = 0; i < host_list.size() * host_list.size(); i++) {
      if (has_cost(bytes_amount, i) > 0.0) {
        nb++;
        value = has_cost(bytes_amount, i);
      }
    }
    if (nb == 1) {
      action = surf_network_model->communicate(host_list[0], host_list[1], value, rate);
    } else if (nb == 0) {
      xbt_die("Cannot have a communication with no flop to exchange in this model. You should consider using the "
          "ptask model");
    } else {
      xbt_die("Cannot have a communication that is not a simple point-to-point in this model. You should consider "
          "using the ptask model");
    }
  } else {
    xbt_die(
        "This model only accepts one of the following. You should consider using the ptask model for the other cases.\n"
        " - execution with one host only and no communication\n"
        " - Self-comms with one host only\n"
        " - Communications with two hosts and no computation");
  }
  return action;
}

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
  /* All processes should be gone when the host is turned off (by the end of the simulation). */
  if (not actor_list_.empty()) {
    std::string msg = std::string("Shutting down host, but it's not empty:");
    for (auto const& process : actor_list_)
      msg += "\n\t" + std::string(process.get_name());

    SIMIX_display_process_status();
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
void HostImpl::turn_on()
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
void HostImpl::turn_off()
{
  if (not actor_list_.empty()) {
    for (auto& actor : actor_list_) {
      XBT_DEBUG("Killing Actor %s@%s on behalf of %s which turned off that host.", actor.get_cname(),
                actor.get_host()->get_cname(), SIMIX_process_self()->get_cname());
      SIMIX_process_self()->kill(&actor);
    }
  }
  // When a host is turned off, we want to keep only the actors that should restart for when it will boot again.
  // Then get rid of the others.
  auto elm = remove_if(begin(actors_at_boot_), end(actors_at_boot_), [](kernel::actor::ProcessArg* arg) {
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
    res.push_back(actor.ciface());
  return res;
}
size_t HostImpl::get_actor_count()
{
  return actor_list_.size();
}

std::vector<s4u::Disk*> HostImpl::get_disks()
{
  std::vector<s4u::Disk*> disks;
  for (auto const& d : disks_)
    disks.push_back(&d->piface_);
  return disks;
}

void HostImpl::add_disk(s4u::Disk* disk)
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
      storages.push_back(s.second->piface_.get_cname());
  return storages;
}

} // namespace surf
} // namespace simgrid
