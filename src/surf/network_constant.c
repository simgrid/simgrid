/* Copyright (c) 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_private.h"
#include "surf/random_mgr.h"
#include "xbt/dict.h"
#include "xbt/str.h"
#include "xbt/log.h"

typedef struct surf_action_network_Constant {
  s_surf_action_t generic_action;
  double latency;
  double lat_init;
  int suspended;
} s_surf_action_network_Constant_t, *surf_action_network_Constant_t;

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_network);
static random_data_t random_latency = NULL;
static int host_number_int = 0;

static void netcste_count_hosts(void)
{
  host_number_int++;
}

static void netcste_define_callbacks(const char *file)
{
  surfxml_add_callback(STag_surfxml_host_cb_list, &netcste_count_hosts);
}

static int netcste_resource_used(void *resource_id)
{
  return 0;
}

static int netcste_action_unref(surf_action_t action)
{
  action->refcount--;
  if (!action->refcount) {
    xbt_swag_remove(action, action->state_set);
    free(action);
    return 1;
  }
  return 0;
}

static void netcste_action_cancel(surf_action_t action)
{
  return;
}

static double netcste_share_resources(double now)
{
  surf_action_network_Constant_t action = NULL;
  xbt_swag_t running_actions = surf_network_model->states.running_action_set;
  double min = -1.0;

  xbt_swag_foreach(action, running_actions) {
    if (action->latency > 0) {
      if (min < 0)
        min = action->latency;
      else if (action->latency < min)
        min = action->latency;
    }
  }

  return min;
}

static void netcste_update_actions_state(double now, double delta)
{
  surf_action_network_Constant_t action = NULL;
  surf_action_network_Constant_t next_action = NULL;
  xbt_swag_t running_actions = surf_network_model->states.running_action_set;

  xbt_swag_foreach_safe(action, next_action, running_actions) {
    if (action->latency > 0) {
      if (action->latency > delta) {
        double_update(&(action->latency), delta);
      } else {
        action->latency = 0.0;
      }
    }
    double_update(&(action->generic_action.remains),
                  action->generic_action.cost * delta / action->lat_init);
    if (action->generic_action.max_duration != NO_MAX_DURATION)
      double_update(&(action->generic_action.max_duration), delta);

    if (action->generic_action.remains <= 0) {
      action->generic_action.finish = surf_get_clock();
      surf_network_model->action_state_set((surf_action_t) action,
                                           SURF_ACTION_DONE);
    } else if ((action->generic_action.max_duration != NO_MAX_DURATION)
               && (action->generic_action.max_duration <= 0)) {
      action->generic_action.finish = surf_get_clock();
      surf_network_model->action_state_set((surf_action_t) action,
                                           SURF_ACTION_DONE);
    }
  }
}

static void netcste_update_resource_state(void *id,
                                  tmgr_trace_event_t event_type,
                                  double value, double time)
{
  DIE_IMPOSSIBLE;
}

static surf_action_t netcste_communicate(const char *src_name, const char *dst_name,
                                 int src, int dst, double size, double rate)
{
  surf_action_network_Constant_t action = NULL;

  XBT_IN4("(%s,%s,%g,%g)", src_name, dst_name, size, rate);

  action =
    surf_action_new(sizeof(s_surf_action_network_Constant_t), size,
                    surf_network_model, 0);

  action->suspended = 0;

  action->latency = 1;          //random_generate(random_latency);
  action->lat_init = action->latency;

  if (action->latency <= 0.0) {
    action->generic_action.state_set =
      surf_network_model->states.done_action_set;
    xbt_swag_insert(action, action->generic_action.state_set);
  }

  XBT_OUT;

  return (surf_action_t) action;
}

/* returns an array of link_Constant_t */
static xbt_dynar_t netcste_get_route(void *src, void *dst)
{
  xbt_die("Calling this function does not make any sense");
}

static double netcste_get_link_bandwidth(const void *link)
{
  DIE_IMPOSSIBLE;
}

static double netcste_get_link_latency(const void *link)
{
  DIE_IMPOSSIBLE;
}

static int link_shared(const void *link)
{
  DIE_IMPOSSIBLE;
}

static void netcste_action_suspend(surf_action_t action)
{
  ((surf_action_network_Constant_t) action)->suspended = 1;
}

static void netcste_action_resume(surf_action_t action)
{
  if (((surf_action_network_Constant_t) action)->suspended)
    ((surf_action_network_Constant_t) action)->suspended = 0;
}

static int netcste_action_is_suspended(surf_action_t action)
{
  return ((surf_action_network_Constant_t) action)->suspended;
}

static void netcste_finalize(void)
{
  surf_model_exit(surf_network_model);
  surf_network_model = NULL;
}



void surf_network_model_init_Constant(const char *filename)
{
  xbt_assert(surf_network_model == NULL);
  if (surf_network_model)
    return;
  surf_network_model = surf_model_init();

  INFO0("Blah");
  surf_network_model->name = "constant time network";
  surf_network_model->action_unref = netcste_action_unref;
  surf_network_model->action_cancel = netcste_action_cancel;
  surf_network_model->action_recycle = net_action_recycle;
  surf_network_model->get_remains = net_action_get_remains;
  surf_network_model->get_latency_limited = net_get_link_latency;

  surf_network_model->model_private->resource_used = netcste_resource_used;
  surf_network_model->model_private->share_resources = netcste_share_resources;
  surf_network_model->model_private->update_actions_state =
    netcste_update_actions_state;
  surf_network_model->model_private->update_resource_state =
    netcste_update_resource_state;
  surf_network_model->model_private->finalize = netcste_finalize;

  surf_network_model->suspend = netcste_action_suspend;
  surf_network_model->resume = netcste_action_resume;
  surf_network_model->is_suspended = netcste_action_is_suspended;
  surf_cpu_model->set_max_duration = net_action_set_max_duration;

  surf_network_model->extension.network.communicate = netcste_communicate;
  surf_network_model->extension.network.get_link_bandwidth =
    netcste_get_link_bandwidth;
  surf_network_model->extension.network.get_link_latency = netcste_get_link_latency;
  surf_network_model->extension.network.link_shared = link_shared;

  if (!random_latency)
    random_latency = random_new(RAND, 100, 0.0, 1.0, .125, .034);
  netcste_define_callbacks(filename);
  xbt_dynar_push(model_list, &surf_network_model);

  update_model_description(surf_network_model_description,
                           "Constant", surf_network_model);

  xbt_cfg_set_string(_surf_cfg_set, "routing", "none");
  routing_model_create(sizeof(double), NULL);
}
