/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "network_gtnets_private.h"
#include "gtnets/gtnets_interface.h"
#include "xbt/str.h"

static double time_to_next_flow_completion = -1;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_network_gtnets, surf,
                                "Logging specific to the SURF network GTNetS module");

extern routing_global_t global_routing;

double sg_gtnets_jitter=0.0;
int sg_gtnets_jitter_seed=10;

static void link_new(char *name, double bw, double lat, xbt_dict_t props)
{
  static int link_count = -1;
  network_link_GTNETS_t gtnets_link;
  network_link_GTNETS_t gtnets_link_friend;
  int tmp_idsrc=-1;
  int tmp_iddst=-1;
  char *name_friend;

  if (xbt_dict_get_or_null(surf_network_model->resource_set, name)) {
    return;
  }

#ifdef HAVE_TRACING
  TRACE_surf_link_declaration (name, bw, lat);
#endif

  DEBUG1("Scanning link name %s", name);
  sscanf(name, "%d_%d", &tmp_idsrc, &tmp_iddst);
  DEBUG2("Link name split into %d and %d", tmp_idsrc, tmp_iddst);

  xbt_assert0( (tmp_idsrc!=-1)&&(tmp_idsrc!=-1), "You need to respect fullduplex convention x_y for xml link id.");

  name_friend = (char *)calloc(strlen(name), sizeof(char));
  sprintf(name_friend, "%d_%d", tmp_iddst, tmp_idsrc);

  gtnets_link = xbt_new0(s_network_link_GTNETS_t, 1);
  gtnets_link->generic_resource.name = name;
  gtnets_link->generic_resource.properties = props;
  gtnets_link->bw_current = bw;
  gtnets_link->lat_current = lat;

  if((gtnets_link_friend=xbt_dict_get_or_null(surf_network_model->resource_set, name_friend))) {
    gtnets_link->id = gtnets_link_friend->id;
  } else {
    link_count++;

    DEBUG4("Adding new link, linkid %d, name %s, latency %g, bandwidth %g", link_count, name, lat, bw);
    if (gtnets_add_link(link_count, bw, lat)) {
      xbt_assert0(0, "Cannot create GTNetS link");
    }
    gtnets_link->id = link_count;
  }
  xbt_dict_set(surf_network_model->resource_set, name, gtnets_link,
      surf_resource_free);
}

static void route_new(int src_id, int dst_id, xbt_dynar_t links,int nb_link)
{
  network_link_GTNETS_t link;
  unsigned int cursor;
  int i=0;
  int *gtnets_links;

  XBT_IN4("(src_id=%d, dst_id=%d, links=%p, nb_link=%d)",
          src_id, dst_id, links, nb_link);

  /* Build the list of gtnets link IDs */
  gtnets_links = (int *) calloc(nb_link, sizeof(int));
  i = 0;
  xbt_dynar_foreach(links, cursor, link) {
    gtnets_links[i++] = link->id;
  }

  if (gtnets_add_route(src_id, dst_id, gtnets_links, nb_link)) {
    xbt_assert0(0, "Cannot create GTNetS route");
  }
  XBT_OUT;
}

static void route_onehop_new(int src_id, int dst_id,
                             network_link_GTNETS_t link)
{
  if (gtnets_add_onehop_route(src_id, dst_id, link->id)) {
    xbt_assert0(0, "Cannot create GTNetS route");
  }
}

/* Parse the XML for a network link */
static void parse_link_init(void)
{
  char *name;
  double bw;
  double lat;
  e_surf_resource_state_t state;

  name = xbt_strdup(A_surfxml_link_id);
  surf_parse_get_double(&bw, A_surfxml_link_bandwidth);
  surf_parse_get_double(&lat, A_surfxml_link_latency);
  state = SURF_RESOURCE_ON;

  tmgr_trace_t bw_trace;
  tmgr_trace_t state_trace;
  tmgr_trace_t lat_trace;

  bw_trace = tmgr_trace_new(A_surfxml_link_bandwidth_file);
  lat_trace = tmgr_trace_new(A_surfxml_link_latency_file);
  state_trace = tmgr_trace_new(A_surfxml_link_state_file);

  if (bw_trace)
    INFO0("The GTNetS network model doesn't support bandwidth state traces");
  if (lat_trace)
    INFO0("The GTNetS network model doesn't support latency state traces");
  if (state_trace)
    INFO0("The GTNetS network model doesn't support link state traces");

  current_property_set = xbt_dict_new();
  link_new(name, bw, lat, current_property_set);
}

/* Create the gtnets topology based on routing strategy */
static void create_gtnets_topology()
{
//  xbt_dict_cursor_t cursor = NULL;
//  char *key, *data;
//   xbt_dict_t onelink_routes = global_routing->get_onelink_routes();
//   xbt_assert0(onelink_routes, "Error onelink_routes was not initialized");
//
//   DEBUG0("Starting topology generation");
// À refaire plus tard. Il faut prendre la liste des hôtes/routeurs (dans routing)
// À partir de cette liste, on les numérote.
// Ensuite, on peut utiliser les id pour refaire les appels GTNets qui suivent.

//   xbt_dict_foreach(onelink_routes, cursor, key, data){
// 	s_onelink_t link = (s_onelink_t) data;
//
// 	DEBUG3("Link (#%d), src (#%s), dst (#%s)", ((network_link_GTNETS_t)(link->link_ptr))->id , link->src, link->dst);
//     DEBUG0("Calling one link route");
//     if(global_routing->is_router(link->src)){
//     	gtnets_add_router(link->src_id);
//     }
//     if(global_routing->is_router(link->dst)){
//     	gtnets_add_router(link->dst_id);
//     }
//     route_onehop_new(link->src_id, link->dst_id, (network_link_GTNETS_t)(link->link_ptr));
//   }
//
//   xbt_dict_free(&route_table);
//   if (XBT_LOG_ISENABLED(surf_network_gtnets, xbt_log_priority_debug)) {
// 	  gtnets_print_topology();
//   }
}

/* Main XML parsing */
static void define_callbacks(const char *file)
{
  /* Figuring out the network links */
  surfxml_add_callback(STag_surfxml_link_cb_list, &parse_link_init);
  surfxml_add_callback(ETag_surfxml_platform_cb_list, &create_gtnets_topology);
}

static int resource_used(void *resource_id)
{
  xbt_assert0(0, "The resource_used feature is not implemented in GTNets model");
}

static int action_unref(surf_action_t action)
{
  action->refcount--;
  if (!action->refcount) {
    xbt_swag_remove(action, action->state_set);
#ifdef HAVE_TRACING
    if (action->category) xbt_free (action->category);
#endif
    free(action);
    return 1;
  }
  return 0;
}

static void action_cancel(surf_action_t action)
{
  xbt_assert0(0,"Cannot cancel GTNetS flow");
  return;
}

static void action_recycle(surf_action_t action)
{
  xbt_assert0(0,"Cannot recycle GTNetS flow");
  return;
}

static double action_get_remains(surf_action_t action)
{
  return action->remains;
}

static void action_state_set(surf_action_t action,
                             e_surf_action_state_t state)
{
  surf_action_state_set(action, state);
}

static double share_resources(double now)
{
  xbt_swag_t running_actions = surf_network_model->states.running_action_set;

  //get the first relevant value from the running_actions list
  if (!xbt_swag_size(running_actions))
    return -1.0;

  xbt_assert0(time_to_next_flow_completion,
              "Time to next flow completion not initialized!\n");

  DEBUG0("Calling gtnets_get_time_to_next_flow_completion");
  time_to_next_flow_completion = gtnets_get_time_to_next_flow_completion();
  DEBUG1("gtnets_get_time_to_next_flow_completion received %lg", time_to_next_flow_completion);

  return time_to_next_flow_completion;
}

static void update_actions_state(double now, double delta)
{
  surf_action_network_GTNETS_t action = NULL;
  xbt_swag_t running_actions = surf_network_model->states.running_action_set;

  /* If there are no renning flows, just return */
  if (time_to_next_flow_completion < 0.0) {
    return;
  }

  /* if delta == time_to_next_flow_completion, too. */
  if (time_to_next_flow_completion <= delta) {  /* run until the first flow completes */
    void **metadata;
    int i, num_flows;

    num_flows = 0;

    if (gtnets_run_until_next_flow_completion(&metadata, &num_flows)) {
      xbt_assert0(0,
                  "Cannot run GTNetS simulation until next flow completion");
    }
    if (num_flows < 1) {
      xbt_assert0(0,
                  "GTNetS simulation couldn't find a flow that would complete");
    }

    xbt_swag_foreach(action, running_actions) {
      DEBUG2("Action (%p) remains old value: %f", action,
             action->generic_action.remains);
      double sent = gtnets_get_flow_rx(action);

#ifdef HAVE_TRACING
      double trace_sent = sent;
      if (trace_sent == 0){
        //if sent is equals to 0, means that gtnets sent all the bytes
        trace_sent = action->generic_action.cost;
      }
      // tracing resource utilization
// COMMENTED BY DAVID
//       int src = TRACE_surf_gtnets_get_src (action);
//       int dst = TRACE_surf_gtnets_get_dst (action);
//       if (src != -1 && dst != -1){
//         xbt_dynar_t route = used_routing->get_route(src, dst);
//         network_link_GTNETS_t link;
//         unsigned int i;
//         xbt_dynar_foreach(route, i, link) {
//         	TRACE_surf_link_set_utilization (link->generic_resource.name,
//             action->generic_action.data, trace_sent/delta, now-delta, delta);
//         }
//       }
#endif

      DEBUG1("Sent value returned by GTNetS : %f", sent);
      //need to trust this remain value
      if (sent == 0) {
        action->generic_action.remains = 0;
      } else {
        action->generic_action.remains = action->generic_action.cost - sent;
      }
      DEBUG2("Action (%p) remains new value: %f", action,
             action->generic_action.remains);
    }

    for (i = 0; i < num_flows; i++) {
      action = (surf_action_network_GTNETS_t) (metadata[i]);

      action->generic_action.finish = now + time_to_next_flow_completion;
#ifdef HAVE_TRACING
      TRACE_surf_gtnets_destroy (action);
#endif
      action_state_set((surf_action_t) action, SURF_ACTION_DONE);
      DEBUG1("----> Action (%p) just terminated", action);
    }


  } else {                      /* run for a given number of seconds */
    if (gtnets_run(delta)) {
      xbt_assert0(0, "Cannot run GTNetS simulation");
    }
  }

  return;
}

static void update_resource_state(void *id,
                                  tmgr_trace_event_t event_type,
                                  double value, double date)
{
  xbt_assert0(0, "Cannot update model state for GTNetS simulation");
}

/* Max durations are not supported */
static surf_action_t communicate(const char *src_name, const char *dst_name,
                                 double size, double rate)
{
  int src,dst;

  // Utiliser le dictionnaire définit dans create_gtnets_topology pour initialiser correctement src et dst
  src=dst=-1;
  surf_action_network_GTNETS_t action = NULL;

  xbt_assert0((src >= 0 && dst >= 0), "Either src or dst have invalid id (id<0)");

  DEBUG4("Setting flow src %d \"%s\", dst %d \"%s\"", src, src_name, dst, dst_name);

  xbt_dynar_t links = global_routing->get_route(src_name, dst_name);
  route_new(src, dst, links, xbt_dynar_length(links));

  action =  surf_action_new(sizeof(s_surf_action_network_GTNETS_t), size, surf_network_model, 0);

  /* Add a flow to the GTNets Simulation, associated to this action */
  if (gtnets_create_flow(src, dst, size, (void *) action) < 0) {
    xbt_assert2(0, "Not route between host %s and host %s", src_name,
                dst_name);
  }
#ifdef HAVE_TRACING
  TRACE_surf_gtnets_communicate (action, src, dst);
#endif

  return (surf_action_t) action;
}

/* Suspend a flow() */
static void action_suspend(surf_action_t action)
{
  THROW_UNIMPLEMENTED;
}

/* Resume a flow() */
static void action_resume(surf_action_t action)
{
  THROW_UNIMPLEMENTED;
}

/* Test whether a flow is suspended */
static int action_is_suspended(surf_action_t action)
{
  return 0;
}

static void finalize(void)
{
  gtnets_finalize();
}

static void surf_network_model_init_internal(void)
{
  surf_network_model = surf_model_init();

  surf_network_model->name = "network GTNetS";
  surf_network_model->action_unref = action_unref;
  surf_network_model->action_cancel = action_cancel;
  surf_network_model->action_recycle = action_recycle;
  surf_network_model->action_state_set = action_state_set;
  surf_network_model->get_remains = action_get_remains;

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

  surf_network_model->extension.network.communicate = communicate;

  /* Added the initialization for GTNetS interface */
  if (gtnets_initialize(sg_tcp_gamma)) {
    xbt_assert0(0, "Impossible to initialize GTNetS interface");
  }

  routing_model_create(sizeof(network_link_GTNETS_t), NULL);
}

#ifdef HAVE_LATENCY_BOUND_TRACKING
static int get_latency_limited(surf_action_t action){
	return 0;
}
#endif

#ifdef HAVE_GTNETS
void surf_network_model_init_GTNETS(const char *filename)
{
  if (surf_network_model)
    return;
  surf_network_model_init_internal();
  define_callbacks(filename);
  xbt_dynar_push(model_list, &surf_network_model);

#ifdef HAVE_LATENCY_BOUND_TRACKING
  surf_network_model->get_latency_limited = get_latency_limited;
#endif

  if(sg_gtnets_jitter > 0.0){
	  gtnets_set_jitter(sg_gtnets_jitter);
	  gtnets_set_jitter_seed(sg_gtnets_jitter_seed);
  }

  update_model_description(surf_network_model_description,
                           "GTNets", surf_network_model);
}
#endif
