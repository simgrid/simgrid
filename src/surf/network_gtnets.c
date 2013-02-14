/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "network_gtnets_private.h"
#include "gtnets/gtnets_interface.h"
#include "xbt/str.h"
#include "surf/surfxml_parse_values.h"

static double time_to_next_flow_completion = -1;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_network_gtnets, surf,
                                "Logging specific to the SURF network GTNetS module");

extern routing_platf_t routing_platf;

double sg_gtnets_jitter = 0.0;
int sg_gtnets_jitter_seed = 10;

static void link_new(const char *name, double bw, double lat, xbt_dict_t props)
{
  static int link_count = -1;
  network_link_GTNETS_t gtnets_link;

  if (xbt_lib_get_or_null(link_lib, name, SURF_LINK_LEVEL)) {
    return;
  }

  XBT_DEBUG("Scanning link name %s", name);


  gtnets_link = xbt_new0(s_network_link_GTNETS_t, 1);
  gtnets_link->generic_resource.name = xbt_strdup(name);
  gtnets_link->generic_resource.properties = props;
  gtnets_link->bw_current = bw;
  gtnets_link->lat_current = lat;

  link_count++;

  XBT_DEBUG("Adding new link, linkid %d, name %s, latency %g, bandwidth %g",
           link_count, name, lat, bw);

  if (gtnets_add_link(link_count, bw, lat)) {
    xbt_die("Cannot create GTNetS link");
  }
  gtnets_link->id = link_count;

  xbt_lib_set(link_lib, name, SURF_LINK_LEVEL, gtnets_link);
}

static void route_new(int src_id, int dst_id, xbt_dynar_t links,
                      int nb_link)
{
  network_link_GTNETS_t link;
  unsigned int cursor;
  int i = 0;
  int *gtnets_links;

  XBT_IN("(src_id=%d, dst_id=%d, links=%p, nb_link=%d)",
          src_id, dst_id, links, nb_link);

  /* Build the list of gtnets link IDs */
  gtnets_links = xbt_new0(int, nb_link);
  i = 0;
  xbt_dynar_foreach(links, cursor, link) {
    gtnets_links[i++] = link->id;
  }

  if (gtnets_add_route(src_id, dst_id, gtnets_links, nb_link)) {
    xbt_die("Cannot create GTNetS route");
  }
  XBT_OUT();
}

static void route_onehop_new(int src_id, int dst_id,
                             network_link_GTNETS_t link)
{
  if (gtnets_add_onehop_route(src_id, dst_id, link->id)) {
    xbt_die("Cannot create GTNetS route");
  }
}

/* Parse the XML for a network link */
static void parse_link_init(sg_platf_link_cbarg_t link)
{
  XBT_DEBUG("link_gtnets");

  if (link->bandwidth_trace)
    XBT_INFO
        ("The GTNetS network model doesn't support bandwidth state traces");
  if (link->latency_trace)
    XBT_INFO("The GTNetS network model doesn't support latency state traces");
  if (link->state_trace)
    XBT_INFO("The GTNetS network model doesn't support link state traces");

  if (link->policy == SURF_LINK_FULLDUPLEX)
  {
    link_new(bprintf("%s_UP",link->id), link->bandwidth, link->latency, current_property_set);
    link_new(bprintf("%s_DOWN",link->id), link->bandwidth, link->latency, current_property_set);

  }
  else  link_new(link->id, link->bandwidth, link->latency, current_property_set);
  current_property_set = NULL;
}

/* Create the gtnets topology based on routing strategy */
static void create_gtnets_topology(void)
{
   XBT_DEBUG("Starting topology generation");
// FIXME: We should take the list of hosts/routers (in the routing module), number the elements of this list,
//   and then you can use the id to reimplement properly the following GTNets calls

   //get the onelinks from the parsed platform
   xbt_dynar_t onelink_routes = routing_platf->get_onelink_routes();
   if (!onelink_routes)
     return;

   //save them in trace file
   onelink_t onelink;
   unsigned int iter;
   xbt_dynar_foreach(onelink_routes, iter, onelink) {
     void *link = onelink->link_ptr;

     if(onelink->src->id != onelink->dst->id){
     XBT_DEBUG("Link (#%p), src (#%s), dst (#%s), src_id = %d, dst_id = %d",
         link,
         onelink->src->name,
         onelink->dst->name,
         onelink->src->id,
         onelink->dst->id);
     XBT_DEBUG("Calling one link route");
        if(onelink->src->rc_type == SURF_NETWORK_ELEMENT_ROUTER){
          gtnets_add_router(onelink->src->id);
        }
        if(onelink->dst->rc_type == SURF_NETWORK_ELEMENT_ROUTER){
         gtnets_add_router(onelink->dst->id);
        }
        route_onehop_new(onelink->src->id, onelink->dst->id, (network_link_GTNETS_t)(link));
     }
   }

   if (XBT_LOG_ISENABLED(surf_network_gtnets, xbt_log_priority_debug)) {
        gtnets_print_topology();
   }
}

/* Main XML parsing */
static void define_callbacks(void)
{
  /* Figuring out the network links */
  sg_platf_link_add_cb (&parse_link_init);
  sg_platf_postparse_add_cb(&create_gtnets_topology);
}

static int resource_used(void *resource_id)
{
  xbt_die("The resource_used feature is not implemented in GTNets model");
}

static int action_unref(surf_action_t action)
{
  action->refcount--;
  if (!action->refcount) {
    xbt_swag_remove(action, action->state_set);
#ifdef HAVE_TRACING
    xbt_free(action->category);
#endif
    surf_action_free(&action);
    return 1;
  }
  return 0;
}

static void action_cancel(surf_action_t action)
{
  xbt_die("Cannot cancel GTNetS flow");
  return;
}

static void action_recycle(surf_action_t action)
{
  xbt_die("Cannot recycle GTNetS flow");
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

static double share_resources(surf_model_t network_model, double now)
{
  xbt_swag_t running_actions =
      surf_network_model->states.running_action_set;

  //get the first relevant value from the running_actions list
  if (!xbt_swag_size(running_actions))
    return -1.0;

  xbt_assert(time_to_next_flow_completion,
              "Time to next flow completion not initialized!\n");

  XBT_DEBUG("Calling gtnets_get_time_to_next_flow_completion");
  time_to_next_flow_completion = gtnets_get_time_to_next_flow_completion();
  XBT_DEBUG("gtnets_get_time_to_next_flow_completion received %lg",
         time_to_next_flow_completion);

  return time_to_next_flow_completion;
}

static void update_actions_state(surf_model_t network_model, double now, double delta)
{
  surf_action_network_GTNETS_t action = NULL;
  xbt_swag_t running_actions =
      network_model->states.running_action_set;

  /* If there are no running flows, just return */
  if (time_to_next_flow_completion < 0.0) {
    return;
  }

  /* if delta == time_to_next_flow_completion, too. */
  if (time_to_next_flow_completion <= delta) {  /* run until the first flow completes */
    void **metadata;
    int i, num_flows;

    num_flows = 0;

    if (gtnets_run_until_next_flow_completion(&metadata, &num_flows)) {
      xbt_die("Cannot run GTNetS simulation until next flow completion");
    }
    if (num_flows < 1) {
      xbt_die("GTNetS simulation couldn't find a flow that would complete");
    }

    xbt_swag_foreach(action, running_actions) {
      XBT_DEBUG("Action (%p) remains old value: %f", action,
             action->generic_action.remains);
      double sent = gtnets_get_flow_rx(action);

      XBT_DEBUG("Sent value returned by GTNetS : %f", sent);

#ifdef HAVE_TRACING
      action->last_remains = action->generic_action.remains;
#endif

     //need to trust this remain value
     if (sent == 0) {
       action->generic_action.remains = 0;
      } else {
        action->generic_action.remains =
            action->generic_action.cost - sent;
      }

     // verify that this action is a finishing action.
     int found=0;
     for (i = 0; i < num_flows; i++) {
       if(action == (surf_action_network_GTNETS_t) (metadata[i])){
           found = 1;
           break;
       }
     }

     // indeed this action have not yet started
     // because of that we need to fix the remaining to the
     // original total cost
     if(found != 1 && action->generic_action.remains == 0 ){
         action->generic_action.remains = action->generic_action.cost;
     }

     XBT_DEBUG("Action (%p) remains new value: %f", action,
             action->generic_action.remains);

#ifdef HAVE_TRACING
      if (TRACE_is_enabled()) {
        double last_amount_sent = (action->generic_action.cost - action->last_remains);
        double amount_sent = (action->generic_action.cost - action->generic_action.remains);

        // tracing resource utilization
        xbt_dynar_t route = NULL;

        routing_get_route_and_latency (action->src, action->dst, &route, NULL);

        unsigned int i;
        for (i = 0; i < xbt_dynar_length (route); i++){
          network_link_GTNETS_t *link = ((network_link_GTNETS_t*)xbt_dynar_get_ptr (route, i));
          TRACE_surf_link_set_utilization ((*link)->generic_resource.name,
                                            ((surf_action_t) action)->category,
                                            (amount_sent - last_amount_sent)/(delta),
                                            now-delta,
                                            delta);
      }
      }
#endif


    }

    for (i = 0; i < num_flows; i++) {
      action = (surf_action_network_GTNETS_t) (metadata[i]);



      action->generic_action.finish = now + time_to_next_flow_completion;
      action_state_set((surf_action_t) action, SURF_ACTION_DONE);
      XBT_DEBUG("----> Action (%p) just terminated", action);

    }


  } else {                      /* run for a given number of seconds */
    if (gtnets_run(delta)) {
      xbt_die("Cannot run GTNetS simulation");
    }
  }

  return;
}

static void update_resource_state(void *id,
                                  tmgr_trace_event_t event_type,
                                  double value, double date)
{
  xbt_die("Cannot update model state for GTNetS simulation");
}

/* Max durations are not supported */
static surf_action_t communicate(sg_routing_edge_t src_card,
                                 sg_routing_edge_t dst_card,
                                 double size, double rate)
{
  surf_action_network_GTNETS_t action = NULL;

  int src = src_card->id;
  int dst = dst_card->id;
  char *src_name = src_card->name;
  char *dst_name = dst_card->name;

  xbt_assert((src >= 0
               && dst >= 0), "Either src or dst have invalid id (id<0)");

  XBT_DEBUG("Setting flow src %d \"%s\", dst %d \"%s\"", src, src_name, dst,
         dst_name);

  xbt_dynar_t route = NULL;

  routing_get_route_and_latency(src_card, dst_card, &route, NULL);

  route_new(src, dst, route, xbt_dynar_length(route));

  action =
      surf_action_new(sizeof(s_surf_action_network_GTNETS_t), size,
                      surf_network_model, 0);

#ifdef HAVE_TRACING
  action->last_remains = 0;
#endif

  /* Add a flow to the GTNets Simulation, associated to this action */
  if (gtnets_create_flow(src, dst, size, (void *) action) < 0) {
    xbt_die("Not route between host %s and host %s", src_name, dst_name);
  }
#ifdef HAVE_TRACING
  TRACE_surf_gtnets_communicate(action, src_card, dst_card);
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

#ifdef HAVE_TRACING
static void gtnets_action_set_category(surf_action_t action, const char *category)
{
  action->category = xbt_strdup (category);
}
#endif

static void finalize(surf_model_t network_model)
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
#ifdef HAVE_TRACING
  surf_network_model->set_category = gtnets_action_set_category;
#endif

  surf_network_model->extension.network.communicate = communicate;

  /* Added the initialization for GTNetS interface */
  if (gtnets_initialize(sg_tcp_gamma)) {
    xbt_die("Impossible to initialize GTNetS interface");
  }

  routing_model_create(NULL);
}

#ifdef HAVE_LATENCY_BOUND_TRACKING
static int get_latency_limited(surf_action_t action)
{
  return 0;
}
#endif

void surf_network_model_init_GTNETS(void)
{
  if (surf_network_model)
    return;

  surf_network_model_init_internal();
  define_callbacks();
  xbt_dynar_push(model_list, &surf_network_model);

#ifdef HAVE_LATENCY_BOUND_TRACKING
  surf_network_model->get_latency_limited = get_latency_limited;
#endif

  if (sg_gtnets_jitter > 0.0) {
    gtnets_set_jitter(sg_gtnets_jitter);
    gtnets_set_jitter_seed(sg_gtnets_jitter_seed);
  }
}
