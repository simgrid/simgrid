/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstdlib>

#include <algorithm>

#include "host_clm03.hpp"

#include "cpu_cas01.hpp"
#include "simgrid/sg_config.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_host);

/*************
 * CallBacks *
 *************/

/*********
 * Model *
 *********/

void surf_host_model_init_current_default()
{
  surf_host_model = new simgrid::surf::HostCLM03Model();
  xbt_cfg_setdefault_boolean("network/crosstraffic", "yes");
  surf_cpu_model_init_Cas01();
  surf_network_model_init_LegrandVelho();

  all_existing_models->push_back(surf_host_model);
}

void surf_host_model_init_compound()
{
  xbt_assert(surf_cpu_model_pm, "No CPU model defined yet!");
  xbt_assert(surf_network_model, "No network model defined yet!");

  surf_host_model = new simgrid::surf::HostCLM03Model();
  all_existing_models->push_back(surf_host_model);
}

namespace simgrid {
namespace surf {

double HostCLM03Model::nextOccuringEvent(double now){
  ignoreEmptyVmInPmLMM();

  double min_by_cpu = surf_cpu_model_pm->nextOccuringEvent(now);
  double min_by_net = surf_network_model->nextOccuringEventIsIdempotent() ? surf_network_model->nextOccuringEvent(now) : -1;
  double min_by_sto = surf_storage_model->nextOccuringEvent(now);

  XBT_DEBUG("model %p, %s min_by_cpu %f, %s min_by_net %f, %s min_by_sto %f",
      this, typeid(surf_cpu_model_pm).name(), min_by_cpu,
      typeid(surf_network_model).name(), min_by_net,
      typeid(surf_storage_model).name(), min_by_sto);

  double res = std::max({min_by_cpu, min_by_net, min_by_sto});
  if (min_by_cpu >= 0.0 && min_by_cpu < res)
    res = min_by_cpu;
  if (min_by_net >= 0.0 && min_by_net < res)
    res = min_by_net;
  if (min_by_sto >= 0.0 && min_by_sto < res)
    res = min_by_sto;
  return res;
}

void HostCLM03Model::updateActionsState(double /*now*/, double /*delta*/){
  /* I won't do what you tell me */
}

}
}
