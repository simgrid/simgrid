/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/HostImpl.hpp"
#include "surf_interface.hpp"
#include "network_interface.hpp"
#include "src/instr/instr_private.h"
#include "plugins/energy.hpp"
#include "virtual_machine.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_kernel);

/*********
 * TOOLS *
 *********/

static simgrid::surf::HostImpl *get_casted_host(sg_host_t host){ //FIXME: killme
  return host->extension<simgrid::surf::HostImpl>();
}

static simgrid::surf::VirtualMachine *get_casted_vm(sg_host_t host){
  return static_cast<simgrid::surf::VirtualMachine*>(host->extension<simgrid::surf::HostImpl>());
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

    while ((event = future_evt_set->pop_leq(next_event_date, &value, &resource))) {
      if (value >= 0){
        resource->apply_event(event, value);
      }
    }
  }

  XBT_DEBUG ("Set every models in the right state by updating them to 0.");
  xbt_dynar_foreach(all_existing_models, iter, model)
      model->updateActionsState(NOW, 0.0);
}

double surf_solve(double max_date)
{
  double time_delta = -1.0; /* duration */
  double next_event_date = -1.0;
  double model_next_action_end = -1.0;
  double value = -1.0;
  simgrid::surf::Resource *resource = NULL;
  simgrid::surf::Model *model = NULL;
  tmgr_trace_iterator_t event = NULL;
  unsigned int iter;

  if (max_date > 0.0) {
    xbt_assert(max_date > NOW,"You asked to simulate up to %f, but that's in the past already", max_date);

    time_delta = max_date - NOW;
  }

  /* Physical models MUST be resolved first */
  XBT_DEBUG("Looking for next event in physical models");
  double next_event_phy = surf_host_model->next_occuring_event(NOW);
  if ((time_delta < 0.0 || next_event_phy < time_delta) && next_event_phy >= 0.0) {
    time_delta = next_event_phy;
  }
  if (surf_vm_model != NULL) {
    XBT_DEBUG("Looking for next event in virtual models");
    double next_event_virt = surf_vm_model->next_occuring_event(NOW);
    if ((time_delta < 0.0 || next_event_virt < time_delta) && next_event_virt >= 0.0)
      time_delta = next_event_virt;
  }

  XBT_DEBUG("Min for resources (remember that NS3 don't update that value): %f", time_delta);

  XBT_DEBUG("Looking for next trace event");

  while (1) { // Handle next occurring events until none remains
    next_event_date = future_evt_set->next_date();
    XBT_DEBUG("Next TRACE event: %f", next_event_date);

    if(! surf_network_model->next_occuring_event_isIdempotent()){ // NS3, I see you
      if (next_event_date!=-1.0 && time_delta!=-1.0) {
        time_delta = MIN(next_event_date - NOW, time_delta);
      } else {
        time_delta = MAX(next_event_date - NOW, time_delta); // Get the positive component
      }

      XBT_DEBUG("Run the NS3 network at most %fs", time_delta);
      // run until min or next flow
      model_next_action_end = surf_network_model->next_occuring_event(time_delta);

      XBT_DEBUG("Min for network : %f", model_next_action_end);
      if(model_next_action_end>=0.0)
        time_delta = model_next_action_end;
    }

    if (next_event_date < 0.0) {
      XBT_DEBUG("no next TRACE event. Stop searching for it");
      break;
    }

    if ((time_delta == -1.0) || (next_event_date > NOW + time_delta))
      break; // next event occurs after the next resource change, bail out

    XBT_DEBUG("Updating models (min = %g, NOW = %g, next_event_date = %g)", time_delta, NOW, next_event_date);

    while ((event = future_evt_set->pop_leq(next_event_date, &value, &resource))) {
      if (resource->isUsed() || xbt_dict_get_or_null(watched_hosts_lib, resource->getName())) {
        time_delta = next_event_date - NOW;
        XBT_DEBUG("This event invalidates the next_occuring_event() computation of models. Next event set to %f", time_delta);
      }
      // FIXME: I'm too lame to update NOW live, so I change it and restore it so that the real update with surf_min will work
      double round_start = NOW;
      NOW = next_event_date;
      /* update state of the corresponding resource to the new value. Does not touch lmm.
         It will be modified if needed when updating actions */
      XBT_DEBUG("Calling update_resource_state for resource %s", resource->getName());
      resource->apply_event(event, value);
      NOW = round_start;
    }
  }

  /* FIXME: Moved this test to here to avoid stopping simulation if there are actions running on cpus and all cpus are with availability = 0.
   * This may cause an infinite loop if one cpu has a trace with periodicity = 0 and the other a trace with periodicity > 0.
   * The options are: all traces with same periodicity(0 or >0) or we need to change the way how the events are managed */
  if (time_delta == -1.0) {
    XBT_DEBUG("No next event at all. Bail out now.");
    return -1.0;
  }

  XBT_DEBUG("Duration set to %f", time_delta);

  // Bump the time: jump into the future
  NOW = NOW + time_delta;

  // Inform the models of the date change
  xbt_dynar_foreach(all_existing_models, iter, model) {
    model->updateActionsState(NOW, time_delta);
  }

  TRACE_paje_dump_buffer (0);

  return time_delta;
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

void surf_cpu_action_set_bound(surf_action_t action, double bound) {
  static_cast<simgrid::surf::CpuAction*>(action)->setBound(bound);
}

surf_file_t surf_storage_action_get_file(surf_action_t action){
  return static_cast<simgrid::surf::StorageAction*>(action)->p_file;
}
