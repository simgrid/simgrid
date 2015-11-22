/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "host_clm03.hpp"

#include "cpu_cas01.hpp"
#include "simgrid/sg_config.h"
#include "virtual_machine.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_host);

/*************
 * CallBacks *
 *************/

/*********
 * Model *
 *********/

void surf_host_model_init_current_default(void)
{
  surf_host_model = new HostCLM03Model();
  xbt_cfg_setdefault_boolean(_sg_cfg_set, "network/crosstraffic", "yes");
  surf_cpu_model_init_Cas01();
  surf_network_model_init_LegrandVelho();

  Model *model = surf_host_model;
  xbt_dynar_push(all_existing_models, &model);
  xbt_dynar_push(model_list_invoke, &model);
  sg_platf_host_add_cb(host_parse_init);
}

void surf_host_model_init_compound()
{

  xbt_assert(surf_cpu_model_pm, "No CPU model defined yet!");
  xbt_assert(surf_network_model, "No network model defined yet!");
  surf_host_model = new HostCLM03Model();

  Model *model = surf_host_model;
  xbt_dynar_push(all_existing_models, &model);
  xbt_dynar_push(model_list_invoke, &model);
  sg_platf_host_add_cb(host_parse_init);
}

Host *HostCLM03Model::createHost(const char *name){
  sg_host_t sg_host = sg_host_by_name(name);
  Host *host = new HostCLM03(surf_host_model, name, NULL,
		  (xbt_dynar_t)xbt_lib_get_or_null(storage_lib, name, ROUTING_STORAGE_HOST_LEVEL),
		  sg_host_edge(sg_host),
		  sg_host_surfcpu(sg_host));
  XBT_DEBUG("Create host %s with %ld mounted disks", name, xbt_dynar_length(host->p_storage));
  xbt_lib_set(host_lib, name, SURF_HOST_LEVEL, host);
  return host;
}

double HostCLM03Model::shareResources(double now){
  adjustWeightOfDummyCpuActions();

  double min_by_cpu = surf_cpu_model_pm->shareResources(now);
  double min_by_net = surf_network_model->shareResourcesIsIdempotent() ? surf_network_model->shareResources(now) : -1;
  double min_by_sto = surf_storage_model->shareResources(now);

  XBT_DEBUG("model %p, %s min_by_cpu %f, %s min_by_net %f, %s min_by_sto %f",
      this, typeid(surf_cpu_model_pm).name(), min_by_cpu,
	        typeid(surf_network_model).name(), min_by_net,
			typeid(surf_storage_model).name(), min_by_sto);

  double res = max(max(min_by_cpu, min_by_net), min_by_sto);
  if (min_by_cpu >= 0.0 && min_by_cpu < res)
	res = min_by_cpu;
  if (min_by_net >= 0.0 && min_by_net < res)
	res = min_by_net;
  if (min_by_sto >= 0.0 && min_by_sto < res)
	res = min_by_sto;
  return res;
}

void HostCLM03Model::updateActionsState(double /*now*/, double /*delta*/){
  return;
}

Action *HostCLM03Model::executeParallelTask(int host_nb,
                                        sg_host_t *host_list,
                                        double *flops_amount,
                                        double *bytes_amount,
                                        double rate){
#define cost_or_zero(array,pos) ((array)?(array)[pos]:0.0)
  Action *action =NULL;
  if ((host_nb == 1)
      && (cost_or_zero(bytes_amount, 0) == 0.0)){
    action = surf_host_execute(host_list[0],flops_amount[0]);
  } else if ((host_nb == 1)
           && (cost_or_zero(flops_amount, 0) == 0.0)) {
    action = surf_network_model->communicate(sg_host_edge(host_list[0]),
    		                                 sg_host_edge(host_list[0]),
											 bytes_amount[0], rate);
  } else if ((host_nb == 2)
             && (cost_or_zero(flops_amount, 0) == 0.0)
             && (cost_or_zero(flops_amount, 1) == 0.0)) {
    int i,nb = 0;
    double value = 0.0;

    for (i = 0; i < host_nb * host_nb; i++) {
      if (cost_or_zero(bytes_amount, i) > 0.0) {
        nb++;
        value = cost_or_zero(bytes_amount, i);
      }
    }
    if (nb == 1){
      action = surf_network_model->communicate(sg_host_edge(host_list[0]),
    		                                   sg_host_edge(host_list[1]),
											   value, rate);
    }
  } else
    THROW_UNIMPLEMENTED;      /* This model does not implement parallel tasks for more than 2 hosts */
#undef cost_or_zero
  xbt_free(host_list);
  return action;
}

/************
 * Resource *
 ************/
HostCLM03::HostCLM03(HostModel *model, const char* name, xbt_dict_t properties, xbt_dynar_t storage, RoutingEdge *netElm, Cpu *cpu)
  : Host(model, name, properties, storage, netElm, cpu) {}

bool HostCLM03::isUsed(){
  THROW_IMPOSSIBLE;             /* This model does not implement parallel tasks */
  return -1;
}

void HostCLM03::updateState(tmgr_trace_event_t /*event_type*/, double /*value*/, double /*date*/){
  THROW_IMPOSSIBLE;             /* This model does not implement parallel tasks */
}

Action *HostCLM03::execute(double size) {
  return p_cpu->execute(size);
}

Action *HostCLM03::sleep(double duration) {
  return p_cpu->sleep(duration);
}

e_surf_resource_state_t HostCLM03::getState() {
  return p_cpu->getState();
}

/**********
 * Action *
 **********/
