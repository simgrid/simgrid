/* Copyright (c) 2013-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/plugins/vm/VirtualMachineImpl.hpp"
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

/* Each VM has a dummy CPU action on the PM layer. This CPU action works as the
 * constraint (capacity) of the VM in the PM layer. If the VM does not have any
 * active task, the dummy CPU action must be deactivated, so that the VM does
 * not get any CPU share in the PM layer. */
void HostModel::ignoreEmptyVmInPmLMM()
{
  /* iterate for all virtual machines */
  for (s4u::VirtualMachine* const& ws_vm : vm::VirtualMachineImpl::allVms_) {
    Cpu* cpu = ws_vm->pimpl_cpu;
    int active_tasks = cpu->constraint()->get_variable_amount();

    /* The impact of the VM over its PM is the min between its vCPU amount and the amount of tasks it contains */
    int impact = std::min(active_tasks, ws_vm->pimpl_vm_->coreAmount());

    XBT_DEBUG("set the weight of the dummy CPU action of VM%p on PM to %d (#tasks: %d)", ws_vm, impact, active_tasks);
    if (impact > 0)
      ws_vm->pimpl_vm_->action_->setSharingWeight(1. / impact);
    else
      ws_vm->pimpl_vm_->action_->setSharingWeight(0.);
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

Action* HostModel::executeParallelTask(int host_nb, simgrid::s4u::Host** host_list, double* flops_amount,
    double* bytes_amount, double rate)
{
  Action* action = nullptr;
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

void HostImpl::getAttachedStorageList(std::vector<const char*>* storages)
{
  for (auto const& s : storage_)
    if (s.second->getHost() == piface_->getCname())
      storages->push_back(s.second->piface_.getCname());
}

}
}
