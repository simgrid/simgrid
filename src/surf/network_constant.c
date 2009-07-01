/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_private.h"
#include "network_common.h"
#include "surf/random_mgr.h"
#include "xbt/dict.h"
#include "xbt/str.h"
#include "xbt/log.h"

typedef struct network_card_Constant {
  s_surf_resource_t generic_resource;
  int id;
} s_network_card_Constant_t, *network_card_Constant_t;

typedef struct surf_action_network_Constant {
  s_surf_action_t generic_action;
  double latency;
  double lat_init;
  int suspended;
  network_card_Constant_t src;
  network_card_Constant_t dst;
} s_surf_action_network_Constant_t, *surf_action_network_Constant_t;

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_network);
static random_data_t random_latency = NULL;
static int card_number = 0;
static int host_number = 0;

static int network_card_new(const char *card_name)
{
  network_card_Constant_t card =
    surf_model_resource_by_name(surf_network_model, card_name);

  if (!card) {
    card = xbt_new0(s_network_card_Constant_t, 1);
    card->generic_resource.name = xbt_strdup(card_name);
    card->id = card_number++;
    xbt_dict_set(surf_model_resource_set(surf_network_model), card_name, card,
                 surf_resource_free);
  }
  return card->id;
}

static int src_id = -1;
static int dst_id = -1;

static void parse_route_set_endpoints(void)
{
  src_id = network_card_new(A_surfxml_route_src);
  dst_id = network_card_new(A_surfxml_route_dst);
  route_action = A_surfxml_route_action;
}

static void parse_route_set_route(void)
{
  char *name;
  if (src_id != -1 && dst_id != -1) {
    name = bprintf("%x#%x", src_id, dst_id);
    manage_route(route_table, name, route_action, 0);
    free(name);
  }
}

static void count_hosts(void)
{
  host_number++;
}

static void define_callbacks(const char *file)
{
  /* Figuring out the network links */
  surfxml_add_callback(STag_surfxml_host_cb_list, &count_hosts);
  surfxml_add_callback(STag_surfxml_route_cb_list,
                       &parse_route_set_endpoints);
  surfxml_add_callback(ETag_surfxml_route_cb_list, &parse_route_set_route);
}

static int resource_used(void *resource_id)
{
  return 0;
}

static int action_free(surf_action_t action)
{
  action->refcount--;
  if (!action->refcount) {
    xbt_swag_remove(action, action->state_set);
    free(action);
    return 1;
  }
  return 0;
}

static void action_use(surf_action_t action)
{
  action->refcount++;
}

static void action_cancel(surf_action_t action)
{
  return;
}

static void action_recycle(surf_action_t action)
{
  return;
}

static void action_change_state(surf_action_t action,
                                e_surf_action_state_t state)
{
  surf_action_change_state(action, state);
  return;
}

static double share_resources(double now)
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

static void update_actions_state(double now, double delta)
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
      action_change_state((surf_action_t) action, SURF_ACTION_DONE);
    } else if ((action->generic_action.max_duration != NO_MAX_DURATION) &&
               (action->generic_action.max_duration <= 0)) {
      action->generic_action.finish = surf_get_clock();
      action_change_state((surf_action_t) action, SURF_ACTION_DONE);
    }
  }

  return;
}

static void update_resource_state(void *id,
                                  tmgr_trace_event_t event_type,
                                  double value, double time)
{
  DIE_IMPOSSIBLE;
}

static surf_action_t communicate(void *src, void *dst, double size,
                                 double rate)
{
  surf_action_network_Constant_t action = NULL;
  network_card_Constant_t card_src = src;
  network_card_Constant_t card_dst = dst;

  XBT_IN4("(%s,%s,%g,%g)", card_src->generic_resource.name, card_dst->generic_resource.name, size, rate);

  action = xbt_new0(s_surf_action_network_Constant_t, 1);

  action->generic_action.refcount = 1;
  action->generic_action.cost = size;
  action->generic_action.remains = size;
  action->generic_action.max_duration = NO_MAX_DURATION;
  action->generic_action.start = surf_get_clock();
  action->generic_action.finish = -1.0;
  action->generic_action.model_type = surf_network_model;
  action->suspended = 0;

  action->latency = random_generate(random_latency);
  action->lat_init = action->latency;

  if (action->latency <= 0.0)
    action->generic_action.state_set =
      surf_network_model->states.done_action_set;
  else
    action->generic_action.state_set =
      surf_network_model->states.running_action_set;

  xbt_swag_insert(action, action->generic_action.state_set);


  XBT_OUT;

  return (surf_action_t) action;
}

/* returns an array of link_Constant_t */
static const void **get_route(void *src, void *dst)
{
  xbt_assert0(0, "Calling this function does not make any sense");
  return (const void **) NULL;
}

static int get_route_size(void *src, void *dst)
{
  xbt_assert0(0, "Calling this function does not make any sense");
  return 0;
}

static double get_link_bandwidth(const void *link)
{
  DIE_IMPOSSIBLE;
}

static double get_link_latency(const void *link)
{
  DIE_IMPOSSIBLE;
}

static int link_shared(const void *link)
{
  DIE_IMPOSSIBLE;
}

static xbt_dict_t get_properties(void *link)
{
  DIE_IMPOSSIBLE;
}

static void action_suspend(surf_action_t action)
{
  ((surf_action_network_Constant_t) action)->suspended = 1;
}

static void action_resume(surf_action_t action)
{
  if (((surf_action_network_Constant_t) action)->suspended)
    ((surf_action_network_Constant_t) action)->suspended = 0;
}

static int action_is_suspended(surf_action_t action)
{
  return ((surf_action_network_Constant_t) action)->suspended;
}

static void action_set_max_duration(surf_action_t action, double duration)
{
  action->max_duration = duration;
}

static void finalize(void)
{
  surf_model_exit(surf_network_model);
  surf_network_model = NULL;

  card_number = 0;
}

static void surf_network_model_init_internal(void)
{
  surf_network_model = surf_model_init();

  surf_network_model->action_free = action_free;
  surf_network_model->action_use = action_use;
  surf_network_model->action_cancel = action_cancel;
  surf_network_model->action_recycle = action_recycle;
  surf_network_model->action_change_state = action_change_state;
  surf_network_model->action_set_data = surf_action_set_data;
  surf_network_model->name = "network";

  surf_network_model->model_private->resource_used = resource_used;
  surf_network_model->model_private->share_resources = share_resources;
  surf_network_model->model_private->update_actions_state =
    update_actions_state;
  surf_network_model->model_private->update_resource_state =
    update_resource_state;
  surf_network_model->model_private->finalize = finalize;

  surf_network_model->suspend = action_suspend;
  surf_network_model->resume = action_resume;
  surf_network_model->is_suspended = action_is_suspended;
  surf_cpu_model->set_max_duration = action_set_max_duration;

  surf_network_model->extension.network.communicate = communicate;
  surf_network_model->extension.network.get_route = get_route;
  surf_network_model->extension.network.get_route_size = get_route_size;
  surf_network_model->extension.network.get_link_bandwidth =
    get_link_bandwidth;
  surf_network_model->extension.network.get_link_latency = get_link_latency;
  surf_network_model->extension.network.link_shared = link_shared;

  surf_network_model->get_properties = get_properties;

  if (!random_latency)
    random_latency = random_new(RAND, 100, 0.0, 1.0, .125, .034);
}

void surf_network_model_init_Constant(const char *filename)
{

  if (surf_network_model)
    return;
  surf_network_model_init_internal();
  define_callbacks(filename);
  xbt_dynar_push(model_list, &surf_network_model);

  update_model_description(surf_network_model_description,
                           "Constant", surf_network_model);
}
