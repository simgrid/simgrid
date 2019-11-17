/* Copyright (c) 2013-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/host_clm03.hpp"
#include "simgrid/sg_config.hpp"
#include "surf/surf.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_host);

void surf_host_model_init_current_default()
{
  surf_host_model = new simgrid::surf::HostCLM03Model();
  simgrid::config::set_default<bool>("network/crosstraffic", true);
  surf_cpu_model_init_Cas01();
  surf_network_model_init_LegrandVelho();
}

void surf_host_model_init_compound()
{
  xbt_assert(surf_cpu_model_pm, "No CPU model defined yet!");
  xbt_assert(surf_network_model, "No network model defined yet!");
  surf_host_model = new simgrid::surf::HostCLM03Model();
}

namespace simgrid {
namespace surf {
HostCLM03Model::HostCLM03Model()
{
  all_existing_models.push_back(this);
}

double HostCLM03Model::next_occurring_event(double now)
{
  double min_by_cpu = surf_cpu_model_pm->next_occurring_event(now);
  double min_by_net =
      surf_network_model->next_occurring_event_is_idempotent() ? surf_network_model->next_occurring_event(now) : -1;
  double min_by_sto = surf_storage_model->next_occurring_event(now);
  double min_by_dsk = surf_disk_model->next_occurring_event(now);

  XBT_DEBUG("model %p, %s min_by_cpu %f, %s min_by_net %f, %s min_by_sto %f, %s min_by_dsk %f", this,
            typeid(surf_cpu_model_pm).name(), min_by_cpu, typeid(surf_network_model).name(), min_by_net,
            typeid(surf_storage_model).name(), min_by_sto, typeid(surf_disk_model).name(), min_by_dsk);

  double res = min_by_cpu;
  if (res < 0 || (min_by_net >= 0.0 && min_by_net < res))
    res = min_by_net;
  if (res < 0 || (min_by_sto >= 0.0 && min_by_sto < res))
    res = min_by_sto;
  if (res < 0 || (min_by_dsk >= 0.0 && min_by_dsk < res))
    res = min_by_dsk;
  return res;
}

void HostCLM03Model::update_actions_state(double /*now*/, double /*delta*/)
{
  /* I've no action to update */
}

/* Helper function for executeParallelTask */
static inline double has_cost(const double* array, size_t pos)
{
  if (array)
    return array[pos];
  return -1.0;
}

kernel::resource::Action* HostCLM03Model::execute_parallel(const std::vector<s4u::Host*>& host_list,
                                                           const double* flops_amount, const double* bytes_amount,
                                                           double rate)
{
  kernel::resource::Action* action = nullptr;
  if ((host_list.size() == 1) && (has_cost(bytes_amount, 0) <= 0) && (has_cost(flops_amount, 0) > 0)) {
    action = host_list[0]->pimpl_cpu->execution_start(flops_amount[0]);
  } else if ((host_list.size() == 1) && (has_cost(flops_amount, 0) <= 0)) {
    action = surf_network_model->communicate(host_list[0], host_list[0], bytes_amount[0], rate);
  } else if ((host_list.size() == 2) && (has_cost(flops_amount, 0) <= 0) && (has_cost(flops_amount, 1) <= 0)) {
    int nb       = 0;
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

} // namespace surf
} // namespace simgrid
