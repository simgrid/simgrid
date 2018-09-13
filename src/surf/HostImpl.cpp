/* Copyright (c) 2013-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/plugins/vm/VirtualMachineImpl.hpp"
#include "src/simix/smx_private.hpp"

#include <string>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_host, surf, "Logging specific to the SURF host module");

simgrid::surf::HostModel *surf_host_model = nullptr;

/*************
 * Callbacks *
 *************/

namespace simgrid {
namespace surf {

/*********
 * Model *
 *********/

/* Each VM has a dummy CPU action on the PM layer. This CPU action works as the constraint (capacity) of the VM in the
 * PM layer. If the VM does not have any active task, the dummy CPU action must be deactivated, so that the VM does not
 * get any CPU share in the PM layer. */
void HostModel::ignore_empty_vm_in_pm_LMM()
{
  /* iterate for all virtual machines */
  for (s4u::VirtualMachine* const& ws_vm : vm::VirtualMachineImpl::allVms_) {
    Cpu* cpu = ws_vm->pimpl_cpu;
    int active_tasks = cpu->get_constraint()->get_variable_amount();

    /* The impact of the VM over its PM is the min between its vCPU amount and the amount of tasks it contains */
    int impact = std::min(active_tasks, ws_vm->get_impl()->get_core_amount());

    XBT_DEBUG("set the weight of the dummy CPU action of VM%p on PM to %d (#tasks: %d)", ws_vm, impact, active_tasks);
    if (impact > 0)
      ws_vm->get_impl()->action_->set_priority(1. / impact);
    else
      ws_vm->get_impl()->action_->set_priority(0.);
  }
}

/* Helper function for executeParallelTask */
static inline double has_cost(double* array, int pos)
{
  if (array)
    return array[pos];
  else
    return -1.0;
}

kernel::resource::Action* HostModel::execute_parallel(int host_nb, s4u::Host** host_list, double* flops_amount,
                                                      double* bytes_amount, double rate)
{
  kernel::resource::Action* action = nullptr;
  if ((host_nb == 1) && (has_cost(bytes_amount, 0) <= 0)) {
    action = host_list[0]->pimpl_cpu->execution_start(flops_amount[0]);
  } else if ((host_nb == 1) && (has_cost(flops_amount, 0) <= 0)) {
    action = surf_network_model->communicate(host_list[0], host_list[0], bytes_amount[0], rate);
  } else if ((host_nb == 2) && (has_cost(flops_amount, 0) <= 0) && (has_cost(flops_amount, 1) <= 0)) {
    int nb = 0;
    double value = 0.0;

    for (int i = 0; i < host_nb * host_nb; i++) {
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
  delete[] host_list;
  delete[] flops_amount;
  delete[] bytes_amount;
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
  if (not process_list_.empty()) {
    std::string msg = std::string("Shutting down host, but it's not empty:");
    for (auto const& process : process_list_)
      msg += "\n\t" + std::string(process.get_name());

    SIMIX_display_process_status();
    THROWF(arg_error, 0, "%s", msg.c_str());
  }
  for (auto const& arg : actors_at_boot_)
    delete arg;
  actors_at_boot_.clear();
}

/** Re-starts all the actors that are marked as restartable.
 *
 * Weird things will happen if you turn on an host that is already on. S4U is fool-proof, not this.
 */
void HostImpl::turn_on()
{
  for (auto const& arg : actors_at_boot_) {
    XBT_DEBUG("Booting Actor %s(%s) right now", arg->name.c_str(), arg->host->get_cname());
    smx_actor_t actor = simix_global->create_process_function(arg->name.c_str(), arg->code, nullptr, arg->host,
                                                              arg->properties.get(), nullptr);
    if (arg->kill_time >= 0)
      simcall_process_set_kill_time(actor, arg->kill_time);
    if (arg->auto_restart)
      actor->auto_restart_ = arg->auto_restart;
    if (arg->daemon_)
      actor->daemonize();
  }
}

/** Kill all actors hosted here */
void HostImpl::turn_off()
{
  if (not process_list_.empty()) {
    for (auto& actor : process_list_) {
      XBT_DEBUG("Killing Actor %s@%s on behalf of %s which turned off that host.", actor.get_cname(),
                actor.host_->get_cname(), SIMIX_process_self()->get_cname());
      SIMIX_process_kill(&actor, SIMIX_process_self());
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
  for (auto& actor : process_list_)
    res.push_back(actor.ciface());
  return res;
}
int HostImpl::get_actor_count()
{
  return process_list_.size();
}
std::vector<const char*> HostImpl::get_attached_storages()
{
  std::vector<const char*> storages;
  for (auto const& s : storage_)
    if (s.second->getHost() == piface_->get_cname())
      storages.push_back(s.second->piface_.get_cname());
  return storages;
}

}
}
