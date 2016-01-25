/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "host_interface.hpp"
#include "surf_interface.hpp"
#include "network_interface.hpp"
#include "surf_routing_cluster.hpp"
#include "src/instr/instr_private.h"
#include "plugins/energy.hpp"
#include "virtual_machine.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_kernel);

/*********
 * TOOLS *
 *********/

static simgrid::surf::Host *get_casted_host(sg_host_t host){ //FIXME: killme
  return host->extension<simgrid::surf::Host>();
}

static simgrid::surf::VirtualMachine *get_casted_vm(sg_host_t host){
  return static_cast<simgrid::surf::VirtualMachine*>(host->extension<simgrid::surf::Host>());
}

extern double NOW;

void surf_presolve(void)
{
  double next_event_date = -1.0;
  tmgr_trace_iterator_t event = NULL;
  double value = -1.0;
  simgrid::surf::Resource *resource = NULL;
  simgrid::surf::Model *model = NULL;
  unsigned int iter;

  XBT_DEBUG ("Consume all trace events occurring before the starting time.");
  while ((next_event_date = future_evt_set->next_date()) != -1.0) {
    if (next_event_date > NOW)
      break;
    while ((event = future_evt_set->pop_leq(next_event_date,
                                            &value,
                                            (void **) &resource))) {
      if (value >= 0){
        resource->updateState(event, value, NOW);
      }
    }
  }

  XBT_DEBUG ("Set every models in the right state by updating them to 0.");
  xbt_dynar_foreach(all_existing_models, iter, model)
      model->updateActionsState(NOW, 0.0);
}

double surf_solve(double max_date)
{
  double surf_min = -1.0; /* duration */
  double next_event_date = -1.0;
  double model_next_action_end = -1.0;
  double value = -1.0;
  simgrid::surf::Resource *resource = NULL;
  simgrid::surf::Model *model = NULL;
  tmgr_trace_iterator_t event = NULL;
  unsigned int iter;

  if(!host_that_restart)
    host_that_restart = xbt_dynar_new(sizeof(char*), NULL);

  if (max_date != -1.0 && max_date != NOW) {
    surf_min = max_date - NOW;
  }

  /* Physical models MUST be resolved first */
  XBT_DEBUG("Looking for next event in physical models");
  double next_event_phy = surf_host_model->shareResources(NOW);
  if ((surf_min < 0.0 || next_event_phy < surf_min) && next_event_phy >= 0.0) {
	  surf_min = next_event_phy;
  }
  if (surf_vm_model != NULL) {
	  XBT_DEBUG("Looking for next event in virtual models");
	  double next_event_virt = surf_vm_model->shareResources(NOW);
	  if ((surf_min < 0.0 || next_event_virt < surf_min) && next_event_virt >= 0.0)
		  surf_min = next_event_virt;
  }

  XBT_DEBUG("Min for resources (remember that NS3 don't update that value) : %f", surf_min);

  XBT_DEBUG("Looking for next trace event");

  do {
    XBT_DEBUG("Next TRACE event : %f", next_event_date);

    next_event_date = future_evt_set->next_date();

    if(! surf_network_model->shareResourcesIsIdempotent()){ // NS3, I see you
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
    while ((event = future_evt_set->pop_leq(next_event_date,
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
  xbt_dynar_foreach(all_existing_models, iter, model) {
	  model->updateActionsState(NOW, surf_min);
  }

  TRACE_paje_dump_buffer (0);

  return surf_min;
}

/*********
 * MODEL *
 *********/

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

int surf_model_running_action_set_size(surf_model_t model){
  return model->getRunningActionSet()->size();
}

xbt_dynar_t surf_host_model_get_route(surf_host_model_t /*model*/,
                                             sg_host_t src, sg_host_t dst){
  xbt_dynar_t route = NULL;
  routing_platf->getRouteAndLatency(src->pimpl_netcard, dst->pimpl_netcard, &route, NULL);
  return route;
}

void surf_vm_model_create(const char *name, sg_host_t ind_phys_host){
  surf_vm_model->createVM(name, ind_phys_host);
}

surf_action_t surf_network_model_communicate(surf_network_model_t model, sg_host_t src, sg_host_t dst, double size, double rate){
  return model->communicate(src->pimpl_netcard, dst->pimpl_netcard, size, rate);
}

const char *surf_resource_name(surf_cpp_resource_t resource){
  return resource->getName();
}

surf_action_t surf_host_sleep(sg_host_t host, double duration){
	return host->pimpl_cpu->sleep(duration);
}


double surf_host_get_available_speed(sg_host_t host){
  return host->pimpl_cpu->getAvailableSpeed();
}

xbt_dynar_t surf_host_get_attached_storage_list(sg_host_t host){
  return get_casted_host(host)->getAttachedStorageList();
}

surf_action_t surf_host_open(sg_host_t host, const char* fullpath){
  return get_casted_host(host)->open(fullpath);
}

surf_action_t surf_host_close(sg_host_t host, surf_file_t fd){
  return get_casted_host(host)->close(fd);
}

int surf_host_unlink(sg_host_t host, surf_file_t fd){
  return get_casted_host(host)->unlink(fd);
}

size_t surf_host_get_size(sg_host_t host, surf_file_t fd){
  return get_casted_host(host)->getSize(fd);
}

surf_action_t surf_host_read(sg_host_t host, surf_file_t fd, sg_size_t size){
  return get_casted_host(host)->read(fd, size);
}

surf_action_t surf_host_write(sg_host_t host, surf_file_t fd, sg_size_t size){
  return get_casted_host(host)->write(fd, size);
}

xbt_dynar_t surf_host_get_info(sg_host_t host, surf_file_t fd){
  return get_casted_host(host)->getInfo(fd);
}

size_t surf_host_file_tell(sg_host_t host, surf_file_t fd){
  return get_casted_host(host)->fileTell(fd);
}

int surf_host_file_seek(sg_host_t host, surf_file_t fd,
                               sg_offset_t offset, int origin){
  return get_casted_host(host)->fileSeek(fd, offset, origin);
}

int surf_host_file_move(sg_host_t host, surf_file_t fd, const char* fullpath){
  return get_casted_host(host)->fileMove(fd, fullpath);
}

xbt_dynar_t surf_host_get_vms(sg_host_t host){
  xbt_dynar_t vms = get_casted_host(host)->getVms();
  xbt_dynar_t vms_ = xbt_dynar_new(sizeof(sg_host_t), NULL);
  unsigned int cpt;
  simgrid::surf::VirtualMachine *vm;
  xbt_dynar_foreach(vms, cpt, vm) {
    // TODO, use a backlink from simgrid::surf::Host to simgrid::s4u::Host 
    sg_host_t vm_ = (sg_host_t) xbt_dict_get_elm_or_null(host_list, vm->getName());
    xbt_dynar_push(vms_, &vm_);
  }
  xbt_dynar_free(&vms);
  return vms_;
}

void surf_host_get_params(sg_host_t host, vm_params_t params){
  get_casted_host(host)->getParams(params);
}

void surf_host_set_params(sg_host_t host, vm_params_t params){
  get_casted_host(host)->setParams(params);
}

void surf_vm_destroy(sg_host_t vm){ // FIXME:DEADCODE
  vm->pimpl_cpu = nullptr;
  vm->pimpl_netcard = nullptr;
}

void surf_vm_suspend(sg_host_t vm){
  get_casted_vm(vm)->suspend();
}

void surf_vm_resume(sg_host_t vm){
  get_casted_vm(vm)->resume();
}

void surf_vm_save(sg_host_t vm){
  get_casted_vm(vm)->save();
}

void surf_vm_restore(sg_host_t vm){
  get_casted_vm(vm)->restore();
}

void surf_vm_migrate(sg_host_t vm, sg_host_t ind_vm_ws_dest){
  get_casted_vm(vm)->migrate(ind_vm_ws_dest);
}

sg_host_t surf_vm_get_pm(sg_host_t vm){
  return get_casted_vm(vm)->getPm();
}

void surf_vm_set_bound(sg_host_t vm, double bound){
  return get_casted_vm(vm)->setBound(bound);
}

void surf_vm_set_affinity(sg_host_t vm, sg_host_t host, unsigned long mask){
  return get_casted_vm(vm)->setAffinity(host->pimpl_cpu, mask);
}

xbt_dict_t surf_storage_get_content(surf_resource_t resource){
  return static_cast<simgrid::surf::Storage*>(surf_storage_resource_priv(resource))->getContent();
}

sg_size_t surf_storage_get_size(surf_resource_t resource){
  return static_cast<simgrid::surf::Storage*>(surf_storage_resource_priv(resource))->getSize();
}

sg_size_t surf_storage_get_free_size(surf_resource_t resource){
  return static_cast<simgrid::surf::Storage*>(surf_storage_resource_priv(resource))->getFreeSize();
}

sg_size_t surf_storage_get_used_size(surf_resource_t resource){
  return static_cast<simgrid::surf::Storage*>(surf_storage_resource_priv(resource))->getUsedSize();
}

xbt_dict_t surf_storage_get_properties(surf_resource_t resource){
  return static_cast<simgrid::surf::Storage*>(surf_storage_resource_priv(resource))->getProperties();
}

const char* surf_storage_get_host(surf_resource_t resource){
  return static_cast<simgrid::surf::Storage*>(surf_storage_resource_priv(resource))->p_attach;
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

void surf_cpu_action_set_affinity(surf_action_t action, sg_host_t host, unsigned long mask) {
  static_cast<simgrid::surf::CpuAction*>(action)->setAffinity(host->pimpl_cpu, mask);
}

void surf_cpu_action_set_bound(surf_action_t action, double bound) {
  static_cast<simgrid::surf::CpuAction*>(action)->setBound(bound);
}

#ifdef HAVE_LATENCY_BOUND_TRACKING
double surf_network_action_get_latency_limited(surf_action_t action) {
  return static_cast<simgrid::surf::NetworkAction*>(action)->getLatencyLimited();
}
#endif

surf_file_t surf_storage_action_get_file(surf_action_t action){
  return static_cast<simgrid::surf::StorageAction*>(action)->p_file;
}
