/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "xbt/dict.h"
#include "portable.h"
#include "surf_private.h"
#include "storage_private.h"
#include "surf/surf_resource.h"
#include "simgrid/sg_config.h"

typedef struct workstation_CLM03 {
  s_surf_resource_t generic_resource;   /* Must remain first to add this to a trace */
  void *net_elm;
  xbt_dynar_t storage;
} s_workstation_CLM03_t, *workstation_CLM03_t;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_workstation, surf,
                                "Logging specific to the SURF workstation module");

surf_model_t surf_workstation_model = NULL;

static void workstation_new(sg_platf_host_cbarg_t host)
{
  workstation_CLM03_t workstation = xbt_new0(s_workstation_CLM03_t, 1);

  workstation->generic_resource.model = surf_workstation_model;
  workstation->generic_resource.name = xbt_strdup(host->id);
  workstation->storage = xbt_lib_get_or_null(storage_lib,host->id,ROUTING_STORAGE_HOST_LEVEL);
  workstation->net_elm = xbt_lib_get_or_null(host_lib,host->id,ROUTING_HOST_LEVEL);
  XBT_DEBUG("Create workstation %s with %ld mounted disks",host->id,xbt_dynar_length(workstation->storage));
  xbt_lib_set(host_lib, host->id, SURF_WKS_LEVEL, workstation);
}

static int ws_resource_used(void *resource_id)
{
  THROW_IMPOSSIBLE;             /* This model does not implement parallel tasks */
  return -1;
}

static void ws_parallel_action_cancel(surf_action_t action)
{
  THROW_UNIMPLEMENTED;          /* This model does not implement parallel tasks */
}

static int ws_parallel_action_free(surf_action_t action)
{
  THROW_UNIMPLEMENTED;          /* This model does not implement parallel tasks */
  return -1;
}

static int ws_action_unref(surf_action_t action)
{
  if (action->model_type == surf_network_model)
    return surf_network_model->action_unref(action);
  else if (action->model_type == surf_cpu_model)
    return surf_cpu_model->action_unref(action);
  else if (action->model_type == surf_workstation_model)
    return ws_parallel_action_free(action);
  else
    DIE_IMPOSSIBLE;
  return 0;
}

static void ws_action_cancel(surf_action_t action)
{
  if (action->model_type == surf_network_model)
    surf_network_model->action_cancel(action);
  else if (action->model_type == surf_cpu_model)
    surf_cpu_model->action_cancel(action);
  else if (action->model_type == surf_workstation_model)
    ws_parallel_action_cancel(action);
  else
    DIE_IMPOSSIBLE;
  return;
}

static void ws_action_state_set(surf_action_t action,
                                e_surf_action_state_t state)
{
  if (action->model_type == surf_network_model)
    surf_network_model->action_state_set(action, state);
  else if (action->model_type == surf_cpu_model)
    surf_cpu_model->action_state_set(action, state);
  else if (action->model_type == surf_workstation_model)
    surf_action_state_set(action, state);
  else
    DIE_IMPOSSIBLE;
  return;
}

static double ws_share_resources(double now)
{
  return -1.0;
}

static void ws_update_actions_state(double now, double delta)
{
  return;
}

static void ws_update_resource_state(void *id,
                                     tmgr_trace_event_t event_type,
                                     double value, double date)
{
  THROW_IMPOSSIBLE;             /* This model does not implement parallel tasks */
}

static surf_action_t ws_execute(void *workstation, double size)
{
  surf_resource_t cpu = ((surf_resource_t) surf_cpu_resource_priv(workstation));
  return cpu->model->extension.cpu.execute(workstation, size);
}

static surf_action_t ws_action_sleep(void *workstation, double duration)
{
  return surf_cpu_model->extension.cpu.
      sleep(workstation, duration);
}

static void ws_action_suspend(surf_action_t action)
{
  if (action->model_type == surf_network_model)
    surf_network_model->suspend(action);
  else if (action->model_type == surf_cpu_model)
    surf_cpu_model->suspend(action);
  else
    DIE_IMPOSSIBLE;
}

static void ws_action_resume(surf_action_t action)
{
  if (action->model_type == surf_network_model)
    surf_network_model->resume(action);
  else if (action->model_type == surf_cpu_model)
    surf_cpu_model->resume(action);
  else
    DIE_IMPOSSIBLE;
}

static int ws_action_is_suspended(surf_action_t action)
{
  if (action->model_type == surf_network_model)
    return surf_network_model->is_suspended(action);
  if (action->model_type == surf_cpu_model)
    return surf_cpu_model->is_suspended(action);
  DIE_IMPOSSIBLE;
  return -1;
}

static void ws_action_set_max_duration(surf_action_t action,
                                       double duration)
{
  if (action->model_type == surf_network_model)
    surf_network_model->set_max_duration(action, duration);
  else if (action->model_type == surf_cpu_model)
    surf_cpu_model->set_max_duration(action, duration);
  else
    DIE_IMPOSSIBLE;
}

static void ws_action_set_priority(surf_action_t action, double priority)
{
  if (action->model_type == surf_network_model)
    surf_network_model->set_priority(action, priority);
  else if (action->model_type == surf_cpu_model)
    surf_cpu_model->set_priority(action, priority);
  else
    DIE_IMPOSSIBLE;
}

#ifdef HAVE_TRACING
static void ws_action_set_category(surf_action_t action, const char *category)
{
  if (action->model_type == surf_network_model)
    surf_network_model->set_category(action, category);
  else if (action->model_type == surf_cpu_model)
    surf_cpu_model->set_category(action, category);
  else
    DIE_IMPOSSIBLE;
}
#endif

#ifdef HAVE_LATENCY_BOUND_TRACKING
static int ws_get_latency_limited(surf_action_t action)
{
  if (action->model_type == surf_network_model)
    return surf_network_model->get_latency_limited(action);
  else
    return 0;
}
#endif

static double ws_action_get_remains(surf_action_t action)
{
  if (action->model_type == surf_network_model)
    return surf_network_model->get_remains(action);
  if (action->model_type == surf_cpu_model)
    return surf_cpu_model->get_remains(action);
  DIE_IMPOSSIBLE;
  return -1.0;
}

static surf_action_t ws_communicate(void *workstation_src,
                                    void *workstation_dst, double size,
                                    double rate)
{
  workstation_CLM03_t src = surf_workstation_resource_priv(workstation_src);
  workstation_CLM03_t dst = surf_workstation_resource_priv(workstation_dst);
  return surf_network_model->extension.network.
      communicate(src->net_elm,
                  dst->net_elm, size, rate);
}

static e_surf_resource_state_t ws_get_state(void *workstation)
{
  return surf_cpu_model->extension.cpu.
      get_state(workstation);
}

static double ws_get_speed(void *workstation, double load)
{
  return surf_cpu_model->extension.cpu.
      get_speed(workstation, load);
}

static int ws_get_core(void *workstation)
{
  return surf_cpu_model->extension.cpu.
      get_core(workstation);
}



static double ws_get_available_speed(void *workstation)
{
  return surf_cpu_model->extension.cpu.
      get_available_speed(workstation);
}

static double ws_get_current_power_peak(void *workstation)
{
  return surf_cpu_model->extension.cpu.
      get_current_power_peak(workstation);
}

static double ws_get_power_peak_at(void *workstation, int pstate_index)
{
  return surf_cpu_model->extension.cpu.
      get_power_peak_at(workstation, pstate_index);
}

static int ws_get_nb_pstates(void *workstation)
{
  return surf_cpu_model->extension.cpu.
      get_nb_pstates(workstation);
}

static void ws_set_power_peak_at(void *workstation, int pstate_index)
{
  surf_cpu_model->extension.cpu.
      set_power_peak_at(workstation, pstate_index);
}

static double ws_get_consumed_energy(void *workstation)
{
  return surf_cpu_model->extension.cpu.
      get_consumed_energy(workstation);
}


static surf_action_t ws_execute_parallel_task(int workstation_nb,
                                              void **workstation_list,
                                              double *computation_amount,
                                              double *communication_amount,
                                              double rate)
{
#define cost_or_zero(array,pos) ((array)?(array)[pos]:0.0)
  if ((workstation_nb == 1)
      && (cost_or_zero(communication_amount, 0) == 0.0))
    return ws_execute(workstation_list[0], computation_amount[0]);
  else if ((workstation_nb == 1)
           && (cost_or_zero(computation_amount, 0) == 0.0))
    return ws_communicate(workstation_list[0], workstation_list[0],communication_amount[0], rate);
  else if ((workstation_nb == 2)
             && (cost_or_zero(computation_amount, 0) == 0.0)
             && (cost_or_zero(computation_amount, 1) == 0.0)) {
    int i,nb = 0;
    double value = 0.0;

    for (i = 0; i < workstation_nb * workstation_nb; i++) {
      if (cost_or_zero(communication_amount, i) > 0.0) {
        nb++;
        value = cost_or_zero(communication_amount, i);
      }
    }
    if (nb == 1)
      return ws_communicate(workstation_list[0], workstation_list[1],value, rate);
  }
#undef cost_or_zero

  THROW_UNIMPLEMENTED;          /* This model does not implement parallel tasks */
  return NULL;
}


/* returns an array of network_link_CM02_t */
static xbt_dynar_t ws_get_route(void *workstation_src, void *workstation_dst)
{
  XBT_DEBUG("ws_get_route");
  workstation_CLM03_t src = surf_workstation_resource_priv(workstation_src);
  workstation_CLM03_t dst = surf_workstation_resource_priv(workstation_dst);
  return surf_network_model->extension.
      network.get_route(src->net_elm,
                  dst->net_elm);
}

static double ws_get_link_bandwidth(const void *link)
{
  return surf_network_model->extension.network.get_link_bandwidth(link);
}

static double ws_get_link_latency(const void *link)
{
  return surf_network_model->extension.network.get_link_latency(link);
}

static int ws_link_shared(const void *link)
{
  return surf_network_model->extension.network.link_shared(link);
}

static void ws_finalize(void)
{
  surf_model_exit(surf_workstation_model);
  surf_workstation_model = NULL;
}

static xbt_dict_t ws_get_properties(const void *ws)
{
  return surf_resource_properties(surf_cpu_resource_priv(ws));
}

static storage_t find_storage_on_mount_list(void *workstation,const char* mount)
{
  storage_t st = NULL;
  s_mount_t mnt;
  unsigned int cursor;
  workstation_CLM03_t ws = (workstation_CLM03_t) surf_workstation_resource_priv(workstation);
  xbt_dynar_t storage_list = ws->storage;

  XBT_DEBUG("Search for storage name '%s' on '%s'",mount,ws->generic_resource.name);
  xbt_dynar_foreach(storage_list,cursor,mnt)
  {
    XBT_DEBUG("See '%s'",mnt.name);
    if(!strcmp(mount,mnt.name)){
      st = mnt.storage;
      break;
    }
  }
  if(!st) xbt_die("Can't find mount '%s' for '%s'",mount,ws->generic_resource.name);
  return st;
}

static xbt_dynar_t ws_get_storage_list(void *workstation)
{
  s_mount_t mnt;
  unsigned int i;
  xbt_dynar_t storage_list = xbt_dynar_new(sizeof(char*), NULL);

  workstation_CLM03_t ws = (workstation_CLM03_t) surf_workstation_resource_priv(workstation);
  xbt_dynar_t storages = ws->storage;

  xbt_dynar_foreach(storages,i,mnt)
  {
    xbt_dynar_push(storage_list, &mnt.name);
  }
  return storage_list;
}

static surf_action_t ws_action_open(void *workstation, const char* mount,
                                    const char* path)
{
  storage_t st = find_storage_on_mount_list(workstation, mount);
  XBT_DEBUG("OPEN on disk '%s'",st->generic_resource.name);
  surf_model_t model = st->generic_resource.model;
  return model->extension.storage.open(st, mount, path);
}

static surf_action_t ws_action_close(void *workstation, surf_file_t fd)
{
  storage_t st = find_storage_on_mount_list(workstation, fd->mount);
  XBT_DEBUG("CLOSE on disk '%s'",st->generic_resource.name);
  surf_model_t model = st->generic_resource.model;
  return model->extension.storage.close(st, fd);
}

static surf_action_t ws_action_read(void *workstation, size_t size,
                                    surf_file_t fd)
{
  storage_t st = find_storage_on_mount_list(workstation, fd->mount);
  XBT_DEBUG("READ on disk '%s'",st->generic_resource.name);
  surf_model_t model = st->generic_resource.model;
  return model->extension.storage.read(st, size, fd);
}

static surf_action_t ws_action_write(void *workstation, size_t size, 
                                     surf_file_t fd)
{
  storage_t st = find_storage_on_mount_list(workstation, fd->mount);
  XBT_DEBUG("WRITE on disk '%s'",st->generic_resource.name);
  surf_model_t model = st->generic_resource.model;
  return model->extension.storage.write(st, size, fd);
}

static int ws_file_unlink(void *workstation, surf_file_t fd)
{
  if (!fd){
    XBT_WARN("No such file descriptor. Impossible to unlink");
    return 0;
  } else {
//    XBT_INFO("%s %zu", fd->storage, fd->size);
    storage_t st = find_storage_on_mount_list(workstation, fd->mount);
    xbt_dict_t content_dict = (st)->content;
    /* Check if the file is on this storage */
    if (!xbt_dict_get_or_null(content_dict, fd->name)){
      XBT_WARN("File %s is not on disk %s. Impossible to unlink", fd->name,
          st->generic_resource.name);
      return 0;
    } else {
      XBT_DEBUG("UNLINK on disk '%s'",st->generic_resource.name);
      st->used_size -= fd->size;

      // Remove the file from storage
      xbt_dict_remove(content_dict,fd->name);

      free(fd->name);
      free(fd->mount);
      xbt_free(fd);
      return 1;
    }
  }
}

static surf_action_t ws_action_ls(void *workstation, const char* mount,
                                  const char *path)
{
  XBT_DEBUG("LS on mount '%s' and file '%s'",mount, path);
  storage_t st = find_storage_on_mount_list(workstation, mount);
  surf_model_t model = st->generic_resource.model;
  return model->extension.storage.ls(st, path);
}

static size_t ws_file_get_size(void *workstation, surf_file_t fd)
{
  return fd->size;
}

static xbt_dynar_t ws_file_get_info(void *workstation, surf_file_t fd)
{
  storage_t st = find_storage_on_mount_list(workstation, fd->mount);
  xbt_dynar_t info = xbt_dynar_new(sizeof(void*), NULL);
  xbt_dynar_push_as(info, void *, (void*)fd->size);
  xbt_dynar_push_as(info, void *, fd->mount);
  xbt_dynar_push_as(info, void *, st->generic_resource.name);
  xbt_dynar_push_as(info, void *, st->type_id);
  xbt_dynar_push_as(info, void *, st->content_type);

  return info;
}

static size_t ws_storage_get_free_size(void *workstation,const char* name)
{
  storage_t st = find_storage_on_mount_list(workstation, name);
  return st->size - st->used_size;
}

static size_t ws_storage_get_used_size(void *workstation,const char* name)
{
  storage_t st = find_storage_on_mount_list(workstation, name);
  return st->used_size;
}

static void surf_workstation_model_init_internal(void)
{
  surf_workstation_model = surf_model_init();

  surf_workstation_model->name = "Workstation";
  surf_workstation_model->action_unref = ws_action_unref;
  surf_workstation_model->action_cancel = ws_action_cancel;
  surf_workstation_model->action_state_set = ws_action_state_set;

  surf_workstation_model->model_private->resource_used = ws_resource_used;
  surf_workstation_model->model_private->share_resources =
      ws_share_resources;
  surf_workstation_model->model_private->update_actions_state =
      ws_update_actions_state;
  surf_workstation_model->model_private->update_resource_state =
      ws_update_resource_state;
  surf_workstation_model->model_private->finalize = ws_finalize;

  surf_workstation_model->suspend = ws_action_suspend;
  surf_workstation_model->resume = ws_action_resume;
  surf_workstation_model->is_suspended = ws_action_is_suspended;
  surf_workstation_model->set_max_duration = ws_action_set_max_duration;
  surf_workstation_model->set_priority = ws_action_set_priority;
#ifdef HAVE_TRACING
  surf_workstation_model->set_category = ws_action_set_category;
#endif
  surf_workstation_model->get_remains = ws_action_get_remains;
#ifdef HAVE_LATENCY_BOUND_TRACKING
  surf_workstation_model->get_latency_limited = ws_get_latency_limited;
#endif

  surf_workstation_model->extension.workstation.execute = ws_execute;
  surf_workstation_model->extension.workstation.sleep = ws_action_sleep;
  surf_workstation_model->extension.workstation.get_state = ws_get_state;
  surf_workstation_model->extension.workstation.get_core = ws_get_core;
  surf_workstation_model->extension.workstation.get_speed = ws_get_speed;
  surf_workstation_model->extension.workstation.get_available_speed =
      ws_get_available_speed;
  surf_workstation_model->extension.workstation.get_current_power_peak = ws_get_current_power_peak;
  surf_workstation_model->extension.workstation.get_power_peak_at = ws_get_power_peak_at;
  surf_workstation_model->extension.workstation.get_nb_pstates = ws_get_nb_pstates;
  surf_workstation_model->extension.workstation.set_power_peak_at = ws_set_power_peak_at;
  surf_workstation_model->extension.workstation.get_consumed_energy = ws_get_consumed_energy;

  surf_workstation_model->extension.workstation.communicate =
      ws_communicate;
  surf_workstation_model->extension.workstation.get_route = ws_get_route;
  surf_workstation_model->extension.workstation.execute_parallel_task =
      ws_execute_parallel_task;
  surf_workstation_model->extension.workstation.get_link_bandwidth =
      ws_get_link_bandwidth;
  surf_workstation_model->extension.workstation.get_link_latency =
      ws_get_link_latency;
  surf_workstation_model->extension.workstation.link_shared =
      ws_link_shared;
  surf_workstation_model->extension.workstation.get_properties =
      ws_get_properties;

  surf_workstation_model->extension.workstation.open = ws_action_open;
  surf_workstation_model->extension.workstation.close = ws_action_close;
  surf_workstation_model->extension.workstation.read = ws_action_read;
  surf_workstation_model->extension.workstation.write = ws_action_write;
  surf_workstation_model->extension.workstation.unlink = ws_file_unlink;
  surf_workstation_model->extension.workstation.ls = ws_action_ls;
  surf_workstation_model->extension.workstation.get_size = ws_file_get_size;
  surf_workstation_model->extension.workstation.get_info = ws_file_get_info;
  surf_workstation_model->extension.workstation.get_free_size = ws_storage_get_free_size;
  surf_workstation_model->extension.workstation.get_used_size = ws_storage_get_used_size;
  surf_workstation_model->extension.workstation.get_storage_list = ws_get_storage_list;
}

void surf_workstation_model_init_current_default(void)
{
  surf_workstation_model_init_internal();
  xbt_cfg_setdefault_boolean(_sg_cfg_set, "network/crosstraffic", "yes");
  surf_cpu_model_init_Cas01();
  surf_network_model_init_LegrandVelho();

  xbt_dynar_push(model_list, &surf_workstation_model);
  sg_platf_host_add_cb(workstation_new);
}

void surf_workstation_model_init_compound()
{

  xbt_assert(surf_cpu_model, "No CPU model defined yet!");
  xbt_assert(surf_network_model, "No network model defined yet!");
  surf_workstation_model_init_internal();
  xbt_dynar_push(model_list, &surf_workstation_model);
  sg_platf_host_add_cb(workstation_new);
}
