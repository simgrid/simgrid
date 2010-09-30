/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_private.h"
#include "surf/random_mgr.h"
#include "xbt/dict.h"
#include "xbt/str.h"
#include "xbt/log.h"

typedef struct surf_action_network_Vivaldi {
  s_surf_action_t generic_action;
  double latency;
  double lat_init;
  int suspended;
} s_surf_action_network_Vivaldi_t, *surf_action_network_Vivaldi_t;

typedef struct s_netviva_coords {
  double x,y,h;
} s_netviva_coords_t, *netviva_coords_t;
xbt_dict_t coords; /* Host name -> coordinates */

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_network);
static random_data_t random_latency_viva = NULL;
static int host_number_int_viva = 0;

static void netviva_count_hosts(void)
{
  host_number_int_viva++;
}

static void netviva_define_callbacks(const char *file)
{
  surfxml_add_callback(STag_surfxml_host_cb_list, &netviva_count_hosts);
}

static int netviva_resource_used(void *resource_id) {
  /* nothing to do here */
  return 0;
}

static int netviva_action_unref(surf_action_t action)
{
  action->refcount--;
  if (!action->refcount) {
    xbt_swag_remove(action, action->state_set);
    free(action);
    return 1;
  }
  return 0;
}

static void netviva_action_cancel(surf_action_t action)
{
  return;
}

static void netviva_action_recycle(surf_action_t action)
{
  return;
}

static double netviva_action_get_remains(surf_action_t action)
{
  return action->remains;
}

/* look for the next event to finish */
static double netviva_share_resources(double now)
{
  surf_action_network_Vivaldi_t action = NULL;
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

static void netviva_update_actions_state(double now, double delta) {
  /* Advance the actions by delta seconds */
  surf_action_network_Vivaldi_t action = NULL;
  surf_action_network_Vivaldi_t next_action = NULL;
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

    if (action->generic_action.remains <= 0) { /* This action is done, inform SURF about it */
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

static void netviva_update_resource_state(void *id,
                                  tmgr_trace_event_t event_type,
                                  double value, double time)
{ /* Called as soon as there is a trace modification */
  DIE_IMPOSSIBLE;
}

static surf_action_t netviva_communicate(const char *src_name, const char *dst_name,
                                  double size, double rate)
{
  surf_action_network_Vivaldi_t action = NULL;
  netviva_coords_t c1,c2;
  c1 = xbt_dict_get(coords,src_name);
  c2 = xbt_dict_get(coords,dst_name);

  action = surf_action_new(sizeof(s_surf_action_network_Vivaldi_t), size,surf_network_model, 0);

  action->suspended = 0;

  action->latency = sqrt((c1->x-c2->x)*(c1->x-c2->x) + (c1->y-c2->y)*(c1->y-c2->y)) + fabs(c1->h)+ fabs(c2->h) ;          //random_generate(random_latency_viva);
  action->lat_init = action->latency;

  if (action->latency <= 0.0) {
    action->generic_action.state_set =
      surf_network_model->states.done_action_set;
    xbt_swag_insert(action, action->generic_action.state_set);
  }

  return (surf_action_t) action;
}

/* returns an array of link_Vivaldi_t */
static xbt_dynar_t netviva_get_route(void *src, void *dst)
{
  xbt_die("Calling this function does not make any sense");
}

static double netviva_get_link_bandwidth(const void *link)
{
  DIE_IMPOSSIBLE;
}

static double netviva_get_link_latency(const void *link)
{
  DIE_IMPOSSIBLE;
}

static int netviva_link_shared(const void *link)
{
  DIE_IMPOSSIBLE;
}

static void netviva_action_suspend(surf_action_t action)
{
  ((surf_action_network_Vivaldi_t) action)->suspended = 1;
}

static void netviva_action_resume(surf_action_t action)
{
  if (((surf_action_network_Vivaldi_t) action)->suspended)
    ((surf_action_network_Vivaldi_t) action)->suspended = 0;
}

static int netviva_action_is_suspended(surf_action_t action)
{
  return ((surf_action_network_Vivaldi_t) action)->suspended;
}

static void netviva_action_set_max_duration(surf_action_t action, double duration)
{
  action->max_duration = duration;
}

static void netviva_finalize(void)
{
  surf_model_exit(surf_network_model);
  surf_network_model = NULL;
}

static void netviva_parse_host(void) {
  netviva_coords_t coord = xbt_new(s_netviva_coords_t,1);

  xbt_dynar_t ctn =xbt_str_split_str(A_surfxml_host_vivaldi," ");

  coord->x = atof(xbt_dynar_get_as(ctn, 0, char*));
  coord->y = atof(xbt_dynar_get_as(ctn, 1, char*));
  coord->h = atof(xbt_dynar_get_as(ctn, 2, char*));

#ifdef HAVE_TRACING
  TRACE_surf_host_vivaldi_parse (A_surfxml_host_id, coord->x, coord->y, coord->h);
#endif

  xbt_dynar_free(&ctn);
  xbt_dict_set(coords, A_surfxml_host_id,coord,NULL);
}

#ifdef HAVE_LATENCY_BOUND_TRACKING
static int netviva_get_latency_limited(surf_action_t action){
  return 0;
}
#endif

void surf_network_model_init_Vivaldi(const char *filename)
{
  xbt_assert(surf_network_model == NULL);
  if (surf_network_model)
    return;
  surf_network_model = surf_model_init();
  coords = xbt_dict_new();

  INFO0("Blih");
  surf_network_model->name = "Vivaldi time network";
  surf_network_model->action_unref = netviva_action_unref;
  surf_network_model->action_cancel = netviva_action_cancel;
  surf_network_model->action_recycle = netviva_action_recycle;
  surf_network_model->get_remains = netviva_action_get_remains;
#ifdef HAVE_LATENCY_BOUND_TRACKING
  surf_network_model->get_latency_limited = netviva_get_latency_limited;
#endif

  surf_network_model->model_private->resource_used = netviva_resource_used;
  surf_network_model->model_private->share_resources = netviva_share_resources;
  surf_network_model->model_private->update_actions_state =
    netviva_update_actions_state;
  surf_network_model->model_private->update_resource_state =
    netviva_update_resource_state;
  surf_network_model->model_private->finalize = netviva_finalize;

  surf_network_model->suspend = netviva_action_suspend;
  surf_network_model->resume = netviva_action_resume;
  surf_network_model->is_suspended = netviva_action_is_suspended;
  surf_cpu_model->set_max_duration = netviva_action_set_max_duration;

  surf_network_model->extension.network.communicate = netviva_communicate;
  surf_network_model->extension.network.get_link_bandwidth =
    netviva_get_link_bandwidth;
  surf_network_model->extension.network.get_link_latency = netviva_get_link_latency;
  surf_network_model->extension.network.link_shared = netviva_link_shared;

  if (!random_latency_viva)
    random_latency_viva = random_new(RAND, 100, 0.0, 1.0, .125, .034);
  netviva_define_callbacks(filename);
  xbt_dynar_push(model_list, &surf_network_model);

  /* Callbacks settings */
  surfxml_add_callback(STag_surfxml_host_cb_list, &netviva_parse_host);

  update_model_description(surf_network_model_description,
                           "Vivaldi", surf_network_model);

#ifdef HAVE_TRACING
  __TRACE_host_variable(0,"vivaldi_x",0,"declare");
  __TRACE_host_variable(0,"vivaldi_y",0,"declare");
  __TRACE_host_variable(0,"vivaldi_h",0,"declare");
#endif

  xbt_cfg_set_string(_surf_cfg_set, "routing", "none");
  routing_model_create(sizeof(double), NULL);
}

