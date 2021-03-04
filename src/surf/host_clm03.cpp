/* Copyright (c) 2013-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/host_clm03.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/sg_config.hpp"
#include "surf/surf.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(res_host);

void surf_host_model_init_current_default()
{
  /* FIXME[donassolo]: this smells bad, but works
   * (the constructor saves its pointer in all_existing_models and models_by_type :O).
   * We need a manager for these models */
  new simgrid::surf::HostCLM03Model();
  simgrid::config::set_default<bool>("network/crosstraffic", true);
  surf_cpu_model_init_Cas01();
  surf_network_model_init_LegrandVelho();
}

void surf_host_model_init_compound()
{
  /* FIXME[donassolo]: this smells bad, but works
   * (the constructor saves its pointer in all_existing_models and models_by_type :O).
   * We need a manager for these models */
  new simgrid::surf::HostCLM03Model();
}

namespace simgrid {
namespace surf {
HostCLM03Model::HostCLM03Model()
{
  all_existing_models.push_back(this);
  models_by_type[simgrid::kernel::resource::Model::Type::HOST].push_back(this);
}

double HostCLM03Model::next_occurring_event(double now)
{
  /* nothing specific to be done here
   * surf_solve already calls all the models next_occuring_event properly */
  return -1.0;
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
  /* FIXME[donassolo]: getting the network_model from the origin host
   * Soon we need to change this function to first get the routes and later
   * create the respective surf actions */
  auto* net_model = host_list[0]->get_netpoint()->get_englobing_zone()->get_network_model();
  if ((host_list.size() == 1) && (has_cost(bytes_amount, 0) <= 0) && (has_cost(flops_amount, 0) > 0)) {
    action = host_list[0]->pimpl_cpu->execution_start(flops_amount[0]);
  } else if ((host_list.size() == 1) && (has_cost(flops_amount, 0) <= 0)) {
    action = net_model->communicate(host_list[0], host_list[0], bytes_amount[0], rate);
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
      action = net_model->communicate(host_list[0], host_list[1], value, rate);
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
