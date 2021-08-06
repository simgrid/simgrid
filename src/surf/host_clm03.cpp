/* Copyright (c) 2013-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/host_clm03.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/kernel/routing/NetZoneImpl.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/sg_config.hpp"
#include "src/kernel/EngineImpl.hpp"
#include "surf/surf.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(res_host);

void surf_host_model_init_current_default()
{
  auto host_model = std::make_shared<simgrid::surf::HostCLM03Model>("Host_CLM03");
  simgrid::config::set_default<bool>("network/crosstraffic", true);
  simgrid::kernel::EngineImpl::get_instance()->add_model(host_model);
  simgrid::s4u::Engine::get_instance()->get_netzone_root()->get_impl()->set_host_model(host_model);
  surf_cpu_model_init_Cas01();
  surf_network_model_init_LegrandVelho();
}

void surf_host_model_init_compound()
{
  auto host_model = std::make_shared<simgrid::surf::HostCLM03Model>("Host_CLM03");
  simgrid::kernel::EngineImpl::get_instance()->add_model(host_model);
  simgrid::s4u::Engine::get_instance()->get_netzone_root()->get_impl()->set_host_model(host_model);
}

namespace simgrid {
namespace surf {
double HostCLM03Model::next_occurring_event(double now)
{
  /* nothing specific to be done here
   * surf_solve already calls all the models next_occurring_event properly */
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
  auto net_model = host_list[0]->get_netpoint()->get_englobing_zone()->get_network_model();
  if ((host_list.size() == 1) && (has_cost(bytes_amount, 0) <= 0) && (has_cost(flops_amount, 0) > 0)) {
    action = host_list[0]->get_cpu()->execution_start(flops_amount[0], rate);
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
