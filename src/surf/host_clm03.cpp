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
double HostCLM03Model::next_occuring_event(double now)
{
  double min_by_cpu = surf_cpu_model_pm->next_occuring_event(now);
  double min_by_net =
      surf_network_model->next_occuring_event_is_idempotent() ? surf_network_model->next_occuring_event(now) : -1;
  double min_by_sto = surf_storage_model->next_occuring_event(now);
  double min_by_dsk = surf_disk_model->next_occuring_event(now);

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

} // namespace surf
} // namespace simgrid
