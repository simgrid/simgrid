/* Copyright (c) 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_interface.hpp"
#include "workstation_interface.hpp"
#include "vm_workstation_interface.hpp"
#include "network_interface.hpp"
#include "surf_routing_cluster.hpp"
#include "instr/instr_private.h"
#include "plugins/energy.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_kernel);

/*********
 * TOOLS *
 *********/

static CpuPtr get_casted_cpu(surf_resource_t resource){
  return static_cast<CpuPtr>(surf_cpu_resource_priv(resource));
}

static WorkstationPtr get_casted_workstation(surf_resource_t resource){
  return static_cast<WorkstationPtr>(surf_workstation_resource_priv(resource));
}

static RoutingEdgePtr get_casted_routing(surf_resource_t resource){
  return static_cast<RoutingEdgePtr>(surf_routing_resource_priv(resource));
}

static WorkstationVMPtr get_casted_vm_workstation(surf_resource_t resource){
  return static_cast<WorkstationVMPtr>(surf_workstation_resource_priv(resource));
}

char *surf_routing_edge_name(sg_routing_edge_t edge){
  return edge->getName();
}

#ifdef CONTEXT_THREADS
//FIXME:keeporremove static xbt_parmap_t surf_parmap = NULL; /* parallel map on models */
#endif

extern double NOW;
extern double *surf_mins; /* return value of share_resources for each model */
extern int surf_min_index;       /* current index in surf_mins */
extern double surf_min;               /* duration determined by surf_solve */

void surf_presolve(void)
{
  double next_event_date = -1.0;
  tmgr_trace_event_t event = NULL;
  double value = -1.0;
  ResourcePtr resource = NULL;
  ModelPtr model = NULL;
  unsigned int iter;

  XBT_DEBUG
      ("First Run! Let's \"purge\" events and put models in the right state");
  while ((next_event_date = tmgr_history_next_date(history)) != -1.0) {
    if (next_event_date > NOW)
      break;
    while ((event =
            tmgr_history_get_next_event_leq(history, next_event_date,
                                            &value,
                                            (void **) &resource))) {
      if (value >= 0){
        resource->updateState(event, value, NOW);
      }
    }
  }
  xbt_dynar_foreach(model_list, iter, model)
      model->updateActionsState(NOW, 0.0);
}

/**
 * Computes when the next action executed in a
 * specific model terminates; this is important,
 * because we can safely skip the amount of time
 * in which no model (read: not even a single one)
 * changes its state; so, if for instance network,
 * cpu, storage don't change (and if we assume they're
 * the only models we use... simple example here :) )
 * for 2s, 1s, 3s then we can skip 1s as after this
 * amount of time the new state needs to be considered.
 *
 */
static void surf_share_resources(surf_model_t model)
{
  double next_action_end = -1.0;
  int i = __sync_fetch_and_add(&surf_min_index, 1);
  if (strcmp(model->getName(), "network NS3")) {
    XBT_DEBUG("Running for Resource [%s]", model->getName());
    next_action_end = model->shareResources(NOW);
    XBT_DEBUG("Resource [%s] : next action end = %f",
        model->getName(), next_action_end);
  }
  surf_mins[i] = next_action_end;
}

static void surf_update_actions_state(surf_model_t model)
{
  model->updateActionsState(NOW, surf_min);
}

double surf_solve(double max_date)
{
  surf_min = -1.0; /* duration */
  double next_event_date = -1.0;
  double model_next_action_end = -1.0;
  double value = -1.0;
  ResourcePtr resource = NULL;
  ModelPtr model = NULL;
  tmgr_trace_event_t event = NULL;
  unsigned int iter;

  if(!host_that_restart)
    host_that_restart = xbt_dynar_new(sizeof(char*), NULL);

  if (max_date != -1.0 && max_date != NOW) {
    surf_min = max_date - NOW;
  }

  XBT_DEBUG("Looking for next action end for all models except NS3");

  if (surf_mins == NULL) {
    surf_mins = xbt_new(double, xbt_dynar_length(model_list_invoke));
  }
  surf_min_index = 0;

  /* sequential version */
  xbt_dynar_foreach(model_list_invoke, iter, model) {
    surf_share_resources(static_cast<ModelPtr>(model));
  }

  unsigned i;
  for (i = 0; i < xbt_dynar_length(model_list_invoke); i++) {
    if ((surf_min < 0.0 || surf_mins[i] < surf_min)
        && surf_mins[i] >= 0.0) {
      surf_min = surf_mins[i];
    }
  }

  XBT_DEBUG("Min for resources (remember that NS3 don't update that value) : %f", surf_min);

  XBT_DEBUG("Looking for next trace event");

  do {
    XBT_DEBUG("Next TRACE event : %f", next_event_date);

    next_event_date = tmgr_history_next_date(history);

    if(!strcmp(surf_network_model->getName(), "network NS3")){
      if(next_event_date!=-1.0 && surf_min!=-1.0) {
        surf_min = MIN(next_event_date - NOW, surf_min);
      } else{
        surf_min = MAX(next_event_date - NOW, surf_min);
      }

      XBT_DEBUG("Run for network at most %f", surf_min);
      // run until min or next flow
      model_next_action_end = surf_network_model->shareResources(surf_min);

      XBT_DEBUG("Min for network : %f", model_next_action_end);
      if(model_next_action_end>=0.0)
        surf_min = model_next_action_end;
    }

    if (next_event_date < 0.0) {
      XBT_DEBUG("no next TRACE event. Stop searching for it");
      break;
    }

    if ((surf_min == -1.0) || (next_event_date > NOW + surf_min)) break;

    XBT_DEBUG("Updating models (min = %g, NOW = %g, next_event_date = %g)", surf_min, NOW, next_event_date);
    while ((event =
            tmgr_history_get_next_event_leq(history, next_event_date,
                                            &value,
                                            (void **) &resource))) {
      if (resource->isUsed() || xbt_dict_get_or_null(watched_hosts_lib, resource->getName())) {
        surf_min = next_event_date - NOW;
        XBT_DEBUG
            ("This event will modify model state. Next event set to %f",
             surf_min);
      }
      /* update state of model_obj according to new value. Does not touch lmm.
         It will be modified if needed when updating actions */
      XBT_DEBUG("Calling update_resource_state for resource %s with min %f",
             resource->getName(), surf_min);
      resource->updateState(event, value, next_event_date);
    }
  } while (1);

  /* FIXME: Moved this test to here to avoid stopping simulation if there are actions running on cpus and all cpus are with availability = 0.
   * This may cause an infinite loop if one cpu has a trace with periodicity = 0 and the other a trace with periodicity > 0.
   * The options are: all traces with same periodicity(0 or >0) or we need to change the way how the events are managed */
  if (surf_min == -1.0) {
  XBT_DEBUG("No next event at all. Bail out now.");
    return -1.0;
  }

  XBT_DEBUG("Duration set to %f", surf_min);

  NOW = NOW + surf_min;
  /* FIXME: model_list or model_list_invoke? revisit here later */
  /* sequential version */
  xbt_dynar_foreach(model_list, iter, model) {
    surf_update_actions_state(model);
  }

  TRACE_paje_dump_buffer (0);

  return surf_min;
}

void routing_get_route_and_latency(sg_routing_edge_t src, sg_routing_edge_t dst,
                              xbt_dynar_t * route, double *latency){
  routing_platf->getRouteAndLatency(src, dst, route, latency);
}

/*********
 * MODEL *
 *********/

surf_model_t surf_resource_model(const void *host, int level) {
  /* If level is SURF_WKS_LEVEL, ws is a workstation_CLM03 object. It has
   * surf_resource at the generic_resource field. */
  ResourcePtr ws = static_cast<ResourcePtr>(xbt_lib_get_level((xbt_dictelm_t) host, level));
  return ws->getModel();
}

void *surf_as_cluster_get_backbone(AS_t as){
  return static_cast<AsClusterPtr>(as)->p_backbone;
}

void surf_as_cluster_set_backbone(AS_t as, void* backbone){
  static_cast<AsClusterPtr>(as)->p_backbone = static_cast<NetworkLinkPtr>(backbone);
}

const char *surf_model_name(surf_model_t model){
  return model->getName();
}

surf_action_t surf_model_extract_done_action_set(surf_model_t model){
  if (model->getDoneActionSet()->empty())
	return NULL;
  surf_action_t res = &model->getDoneActionSet()->front();
  model->getDoneActionSet()->pop_front();
  return res;
}

surf_action_t surf_model_extract_failed_action_set(surf_model_t model){
  if (model->getFailedActionSet()->empty())
	return NULL;
  surf_action_t res = &model->getFailedActionSet()->front();
  model->getFailedActionSet()->pop_front();
  return res;
}

surf_action_t surf_model_extract_ready_action_set(surf_model_t model){
  if (model->getReadyActionSet()->empty())
	return NULL;
  surf_action_t res = &model->getReadyActionSet()->front();
  model->getReadyActionSet()->pop_front();
  return res;
}

surf_action_t surf_model_extract_running_action_set(surf_model_t model){
  if (model->getRunningActionSet()->empty())
	return NULL;
  surf_action_t res = &model->getRunningActionSet()->front();
  model->getRunningActionSet()->pop_front();
  return res;
}

int surf_model_running_action_set_size(surf_model_t model){
  return model->getRunningActionSet()->size();
}

surf_action_t surf_workstation_model_execute_parallel_task(surf_workstation_model_t model,
		                                    int workstation_nb,
                                            void **workstation_list,
                                            double *flops_amount,
                                            double *bytes_amount,
                                            double rate){
  return static_cast<ActionPtr>(model->executeParallelTask(workstation_nb, workstation_list, flops_amount, bytes_amount, rate));
}

surf_action_t surf_workstation_model_communicate(surf_workstation_model_t model, surf_resource_t src, surf_resource_t dst, double size, double rate){
  return model->communicate(get_casted_workstation(src), get_casted_workstation(dst), size, rate);
}

xbt_dynar_t surf_workstation_model_get_route(surf_workstation_model_t /*model*/,
                                             surf_resource_t src, surf_resource_t dst){
  xbt_dynar_t route = NULL;
  routing_platf->getRouteAndLatency(get_casted_workstation(src)->p_netElm,
		                            get_casted_workstation(dst)->p_netElm, &route, NULL);
  return route;
}

void surf_vm_workstation_model_create(const char *name, surf_resource_t ind_phys_host){
  surf_vm_workstation_model->createWorkstationVM(name, ind_phys_host);
}

surf_action_t surf_network_model_communicate(surf_network_model_t model, sg_routing_edge_t src, sg_routing_edge_t dst, double size, double rate){
  return model->communicate(src, dst, size, rate);
}

const char *surf_resource_name(surf_cpp_resource_t resource){
  return resource->getName();
}

xbt_dict_t surf_resource_get_properties(surf_cpp_resource_t resource){
  return resource->getProperties();
}

e_surf_resource_state_t surf_resource_get_state(surf_cpp_resource_t resource){
  return resource->getState();
}

void surf_resource_set_state(surf_cpp_resource_t resource, e_surf_resource_state_t state){
  resource->setState(state);
}

surf_action_t surf_workstation_sleep(surf_resource_t resource, double duration){
  return get_casted_workstation(resource)->sleep(duration);
}

double surf_workstation_get_speed(surf_resource_t resource, double load){
  return get_casted_workstation(resource)->getSpeed(load);
}

double surf_workstation_get_available_speed(surf_resource_t resource){
  return get_casted_workstation(resource)->getAvailableSpeed();
}

int surf_workstation_get_core(surf_resource_t resource){
  return get_casted_workstation(resource)->getCore();
}

surf_action_t surf_workstation_execute(surf_resource_t resource, double size){
  return get_casted_workstation(resource)->execute(size);
}

double surf_workstation_get_current_power_peak(surf_resource_t resource){
  return get_casted_workstation(resource)->getCurrentPowerPeak();
}

double surf_workstation_get_power_peak_at(surf_resource_t resource, int pstate_index){
  return get_casted_workstation(resource)->getPowerPeakAt(pstate_index);
}

int surf_workstation_get_nb_pstates(surf_resource_t resource){
  return get_casted_workstation(resource)->getNbPstates();
}

void surf_workstation_set_pstate(surf_resource_t resource, int pstate_index){
  return get_casted_workstation(resource)->setPstate(pstate_index);
}

double surf_workstation_get_consumed_energy(surf_resource_t resource){
  xbt_assert(surf_energy!=NULL, "The Energy plugin is not active.");
  std::map<CpuPtr, CpuEnergyPtr>::iterator cpuIt = surf_energy->find(get_casted_workstation(resource)->p_cpu);
  return cpuIt->second->getConsumedEnergy();
}

xbt_dict_t surf_workstation_get_mounted_storage_list(surf_resource_t workstation){
  return get_casted_workstation(workstation)->getMountedStorageList();
}

xbt_dynar_t surf_workstation_get_attached_storage_list(surf_resource_t workstation){
  return get_casted_workstation(workstation)->getAttachedStorageList();
}

surf_action_t surf_workstation_open(surf_resource_t workstation, const char* fullpath){
  return get_casted_workstation(workstation)->open(fullpath);
}

surf_action_t surf_workstation_close(surf_resource_t workstation, surf_file_t fd){
  return get_casted_workstation(workstation)->close(fd);
}

int surf_workstation_unlink(surf_resource_t workstation, surf_file_t fd){
  return get_casted_workstation(workstation)->unlink(fd);
}

size_t surf_workstation_get_size(surf_resource_t workstation, surf_file_t fd){
  return get_casted_workstation(workstation)->getSize(fd);
}

surf_action_t surf_workstation_read(surf_resource_t resource, surf_file_t fd, sg_size_t size){
  return get_casted_workstation(resource)->read(fd, size);
}

surf_action_t surf_workstation_write(surf_resource_t resource, surf_file_t fd, sg_size_t size){
  return get_casted_workstation(resource)->write(fd, size);
}

xbt_dynar_t surf_workstation_get_info(surf_resource_t resource, surf_file_t fd){
  return get_casted_workstation(resource)->getInfo(fd);
}

size_t surf_workstation_file_tell(surf_resource_t workstation, surf_file_t fd){
  return get_casted_workstation(workstation)->fileTell(fd);
}

int surf_workstation_file_seek(surf_resource_t workstation, surf_file_t fd,
                               sg_offset_t offset, int origin){
  return get_casted_workstation(workstation)->fileSeek(fd, offset, origin);
}

int surf_workstation_file_move(surf_resource_t workstation, surf_file_t fd, const char* fullpath){
  return get_casted_workstation(workstation)->fileMove(fd, fullpath);
}

xbt_dynar_t surf_workstation_get_vms(surf_resource_t resource){
  xbt_dynar_t vms = get_casted_workstation(resource)->getVms();
  xbt_dynar_t vms_ = xbt_dynar_new(sizeof(smx_host_t), NULL);
  unsigned int cpt;
  WorkstationVMPtr vm;
  xbt_dynar_foreach(vms, cpt, vm) {
    smx_host_t vm_ = xbt_lib_get_elm_or_null(host_lib, vm->getName());
    xbt_dynar_push(vms_, &vm_);
  }
  xbt_dynar_free(&vms);
  return vms_;
}

void surf_workstation_get_params(surf_resource_t resource, ws_params_t params){
  get_casted_workstation(resource)->getParams(params);
}

void surf_workstation_set_params(surf_resource_t resource, ws_params_t params){
  get_casted_workstation(resource)->setParams(params);
}

void surf_vm_workstation_destroy(surf_resource_t resource){
  /* ind_phys_workstation equals to smx_host_t */
  //surf_resource_t ind_vm_workstation = xbt_lib_get_elm_or_null(host_lib, getName());

  /* Before clearing the entries in host_lib, we have to pick up resources. */
  CpuPtr cpu = get_casted_cpu(resource);
  WorkstationVMPtr vm = get_casted_vm_workstation(resource);
  RoutingEdgePtr routing = get_casted_routing(resource);
  char* name = xbt_dict_get_elm_key(resource);
  /* We deregister objects from host_lib, without invoking the freeing callback
   * of each level.
   *
   * Do not call xbt_lib_remove() here. It deletes all levels of the key,
   * including MSG_HOST_LEVEL and others. We should unregister only what we know.
   */
  xbt_lib_unset(host_lib, name, SURF_CPU_LEVEL, 0);
  xbt_lib_unset(host_lib, name, ROUTING_HOST_LEVEL, 0);
  xbt_lib_unset(host_lib, name, SURF_WKS_LEVEL, 0);

  /* TODO: comment out when VM storage is implemented. */
  // xbt_lib_unset(host_lib, name, SURF_STORAGE_LEVEL, 0);

  delete cpu;
  delete vm;
  delete routing;
}

void surf_vm_workstation_suspend(surf_resource_t resource){
  get_casted_vm_workstation(resource)->suspend();
}

void surf_vm_workstation_resume(surf_resource_t resource){
  get_casted_vm_workstation(resource)->resume();
}

void surf_vm_workstation_save(surf_resource_t resource){
  get_casted_vm_workstation(resource)->save();
}

void surf_vm_workstation_restore(surf_resource_t resource){
  get_casted_vm_workstation(resource)->restore();
}

void surf_vm_workstation_migrate(surf_resource_t resource, surf_resource_t ind_vm_ws_dest){
  get_casted_vm_workstation(resource)->migrate(ind_vm_ws_dest);
}

surf_resource_t surf_vm_workstation_get_pm(surf_resource_t resource){
  return get_casted_vm_workstation(resource)->getPm();
}

void surf_vm_workstation_set_bound(surf_resource_t resource, double bound){
  return get_casted_vm_workstation(resource)->setBound(bound);
}

void surf_vm_workstation_set_affinity(surf_resource_t resource, surf_resource_t cpu, unsigned long mask){
  return get_casted_vm_workstation(resource)->setAffinity(get_casted_cpu(cpu), mask);
}

int surf_network_link_is_shared(surf_cpp_resource_t link){
  return static_cast<NetworkLinkPtr>(link)->isShared();
}

double surf_network_link_get_bandwidth(surf_cpp_resource_t link){
  return static_cast<NetworkLinkPtr>(link)->getBandwidth();
}

double surf_network_link_get_latency(surf_cpp_resource_t link){
  return static_cast<NetworkLinkPtr>(link)->getLatency();
}

xbt_dict_t surf_storage_get_content(surf_resource_t resource){
  return static_cast<StoragePtr>(surf_storage_resource_priv(resource))->getContent();
}

sg_size_t surf_storage_get_size(surf_resource_t resource){
  return static_cast<StoragePtr>(surf_storage_resource_priv(resource))->getSize();
}

sg_size_t surf_storage_get_free_size(surf_resource_t resource){
  return static_cast<StoragePtr>(surf_storage_resource_priv(resource))->getFreeSize();
}

sg_size_t surf_storage_get_used_size(surf_resource_t resource){
  return static_cast<StoragePtr>(surf_storage_resource_priv(resource))->getUsedSize();
}

const char* surf_storage_get_host(surf_resource_t resource){
  return static_cast<StoragePtr>(surf_storage_resource_priv(resource))->p_attach;
}

surf_action_t surf_cpu_execute(surf_resource_t cpu, double size){
  return get_casted_cpu(cpu)->execute(size);
}

surf_action_t surf_cpu_sleep(surf_resource_t cpu, double duration){
  return get_casted_cpu(cpu)->sleep(duration);
}

double surf_action_get_start_time(surf_action_t action){
  return action->getStartTime();
}

double surf_action_get_finish_time(surf_action_t action){
  return action->getFinishTime();
}

double surf_action_get_remains(surf_action_t action){
  return action->getRemains();
}

void surf_action_unref(surf_action_t action){
  action->unref();
}

void surf_action_suspend(surf_action_t action){
  action->suspend();
}

void surf_action_resume(surf_action_t action){
  action->resume();
}

void surf_action_cancel(surf_action_t action){
  action->cancel();
}

void surf_action_set_priority(surf_action_t action, double priority){
  action->setPriority(priority);
}

void surf_action_set_category(surf_action_t action, const char *category){
  action->setCategory(category);
}

void *surf_action_get_data(surf_action_t action){
  return action->getData();
}

void surf_action_set_data(surf_action_t action, void *data){
  action->setData(data);
}

e_surf_action_state_t surf_action_get_state(surf_action_t action){
  return action->getState();
}

double surf_action_get_cost(surf_action_t action){
  return action->getCost();
}

void surf_cpu_action_set_affinity(surf_action_t action, surf_resource_t cpu, unsigned long mask) {
  static_cast<CpuActionPtr>(action)->setAffinity(get_casted_cpu(cpu), mask);
}

void surf_cpu_action_set_bound(surf_action_t action, double bound) {
  static_cast<CpuActionPtr>(action)->setBound(bound);
}

#ifdef HAVE_LATENCY_BOUND_TRACKING
double surf_network_action_get_latency_limited(surf_action_t action) {
  return static_cast<NetworkActionPtr>(action)->getLatencyLimited();
}
#endif

surf_file_t surf_storage_action_get_file(surf_action_t action){
  return static_cast<StorageActionPtr>(action)->p_file;
}
