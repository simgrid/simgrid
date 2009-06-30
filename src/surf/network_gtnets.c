/*     $Id$     */

/* Copyright (c) 2005 Henri Casanova. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "network_gtnets_private.h"
#include "gtnets/gtnets_interface.h"
#include "xbt/str.h"


static double time_to_next_flow_completion = -1;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_network_gtnets, surf,
                                "Logging specific to the SURF network module");

/** QUESTIONS for GTNetS integration
 **   1. Check that we did the right thing with name_service and get_resource_name
 **   2. Right now there is no "kill flow" in our GTNetS implementation. Do we
 **      need to do something about this?
 **   3. We ignore the fact there is some max_duration on flows (see #2 above)
 **   4. share_resources() returns a duration, not a date, right?
 **   5. We don't suppoer "rates"
 **   6. We don't update "remaining" for ongoing flows. Is it bad?
 **/

static int src_id = -1;
static int dst_id = -1;

/* Free memory for a network link */
static void link_free(void *nw_link)
{
  free(((network_link_GTNETS_t) nw_link)->name);
  xbt_dict_free(&(((network_link_GTNETS_t) nw_link)->properties));
  free(nw_link);
}

/* Instantiate a new network link */
/* name: some name for the link, from the XML */
/* bw: The bandwidth value            */
/* lat: The latency value             */
static void link_new(char *name, double bw, double lat, xbt_dict_t props)
{
  static int link_count = -1;
  network_link_GTNETS_t gtnets_link;

  /* If link already exists, nothing to do (FIXME: check that multiple definition match?) */
  if (xbt_dict_get_or_null(link_set, name)) {
    return;
  }

  /* KF: Increment the link counter for GTNetS */
  link_count++;

/*
  nw_link->model =  surf_network_model;
  nw_link->name = name;
  nw_link->bw_current = bw_initial;
  if (bw_trace)
    nw_link->bw_event =
	tmgr_history_add_trace(history, bw_trace, 0.0, 0, nw_link);
  nw_link->lat_current = lat_initial;
  if (lat_trace)
    nw_link->lat_event =
	tmgr_history_add_trace(history, lat_trace, 0.0, 0, nw_link);
  nw_link->state_current = state_initial;
  if (state_trace)
    nw_link->state_event =
	tmgr_history_add_trace(history, state_trace, 0.0, 0, nw_link);
*/

  /* KF: Add the link to the GTNetS simulation */
  if (gtnets_add_link(link_count, bw, lat)) {
    xbt_assert0(0, "Cannot create GTNetS link");
  }

  /* KF: Insert entry in the dictionary */
  gtnets_link = xbt_new0(s_network_link_GTNETS_t, 1);
  gtnets_link->name = name;
  gtnets_link->bw_current = bw;
  gtnets_link->lat_current = lat;
  gtnets_link->id = link_count;
  /* Add the properties */
  gtnets_link->properties = props;

  xbt_dict_set(link_set, name, gtnets_link, link_free);

  return;
}

/* free the network card */
static void network_card_free(void *nw_card)
{
  free(((network_card_GTNETS_t) nw_card)->name);
  free(nw_card);
}

/* Instantiate a new network card: MODIFYED BY KF */
static int network_card_new(const char *name)
{
  static int card_count = -1;

  XBT_IN1("(%s)", name);
  /* KF: Check that we haven't seen the network card before */
  network_card_GTNETS_t card =
    surf_model_resource_by_name(surf_network_model, name);

  if (!card) {
    /* KF: Increment the card counter for GTNetS */
    card_count++;

    /* KF: just use the dictionary to map link names to link indices */
    card = xbt_new0(s_network_card_GTNETS_t, 1);
    card->name = xbt_strdup(name);
    card->id = card_count;
    xbt_dict_set(surf_model_resource_set(surf_network_model), name, card,
                 network_card_free);
  }

  LOG1(xbt_log_priority_trace, "   return %d", card->id);
  XBT_OUT;
  /* KF: just return the GTNetS ID as the SURF ID */
  return card->id;
}

/* Instantiate a new route: MODIFY BY KF */
static void route_new(int src_id, int dst_id, network_link_GTNETS_t * links,
                      int nb_link)
{
  int i;
  int *gtnets_links;
  XBT_IN4("(src_id=%d, dst_id=%d, links=%p, nb_link=%d)",
          src_id, dst_id, links, nb_link);

  /* KF: Build the list of gtnets link IDs */
  gtnets_links = (int *) calloc(nb_link, sizeof(int));
  for (i = 0; i < nb_link; i++) {
    gtnets_links[i] = links[i]->id;
  }

  /* KF: Create the GTNets route */
  if (gtnets_add_route(src_id, dst_id, gtnets_links, nb_link)) {
    xbt_assert0(0, "Cannot create GTNetS route");
  }
  XBT_OUT;
}

/* Instantiate a new route: MODIFY BY KF */
static void route_onehop_new(int src_id, int dst_id,
                             network_link_GTNETS_t * links, int nb_link)
{
  int linkid;

  if (nb_link != 1) {
    xbt_assert0(0, "In onehop_new, nb_link should be 1");
  }

  /* KF: Build the linbst of gtnets link IDs */
  linkid = links[0]->id;

  /* KF: Create the GTNets route */
  if (gtnets_add_onehop_route(src_id, dst_id, linkid)) {
    xbt_assert0(0, "Cannot create GTNetS route");
  }
}



/* Parse the XML for a network link */
static void parse_link_init(void)
{
  char *name;
  double bw;
  double lat;
  e_surf_link_state_t state;

  name = xbt_strdup(A_surfxml_link_id);
  surf_parse_get_double(&bw, A_surfxml_link_bandwidth);
  surf_parse_get_double(&lat, A_surfxml_link_latency);
  state = SURF_LINK_ON;

  tmgr_trace_t bw_trace;
  tmgr_trace_t state_trace;
  tmgr_trace_t lat_trace;

  surf_parse_get_trace(&bw_trace, A_surfxml_link_bandwidth_file);
  surf_parse_get_trace(&lat_trace, A_surfxml_link_latency_file);
  surf_parse_get_trace(&state_trace, A_surfxml_link_state_file);

  if (bw_trace)
    INFO0("The GTNetS network model doesn't support bandwidth state traces");
  if (lat_trace)
    INFO0("The GTNetS network model doesn't support latency state traces");
  if (state_trace)
    INFO0("The GTNetS network model doesn't support link state traces");

  current_property_set = xbt_dict_new();
  link_new(name, bw, lat, current_property_set);
}

/* Parses a route from the XML: UNMODIFIED BY HC */
static void parse_route_set_endpoints(void)
{
  src_id = network_card_new(A_surfxml_route_src);
  dst_id = network_card_new(A_surfxml_route_dst);
  route_action = A_surfxml_route_action;
}

/* KF*/
static void parse_route_set_routers(void)
{
  int id = network_card_new(A_surfxml_router_id);

  /* KF: Create the GTNets router */
  if (gtnets_add_router(id)) {
    xbt_assert0(0, "Cannot add GTNetS router");
  }
}

/* Create the route (more than one hops): MODIFIED BY KF */
static void parse_route_set_route(void)
{
  char *name;
  if (src_id != -1 && dst_id != -1) {
    name = bprintf("%x#%x", src_id, dst_id);
    manage_route(route_table, name, route_action, 0);
    free(name);
  }
}

static void add_route()
{
  xbt_ex_t e;
  unsigned int cpt = 0;
  int link_list_capacity = 0;
  int nb_link = 0;
  xbt_dict_cursor_t cursor = NULL;
  char *key, *data, *end;
  const char *sep = "#";
  xbt_dynar_t links, keys;
  static network_link_GTNETS_t *link_list = NULL;


  XBT_IN;
  xbt_dict_foreach(route_table, cursor, key, data) {
    char *link = NULL;
    nb_link = 0;
    links = (xbt_dynar_t) data;
    keys = xbt_str_split_str(key, sep);

    link_list_capacity = xbt_dynar_length(links);
    link_list = xbt_new(network_link_GTNETS_t, link_list_capacity);

    src_id = strtol(xbt_dynar_get_as(keys, 0, char *), &end, 16);
    dst_id = strtol(xbt_dynar_get_as(keys, 1, char *), &end, 16);
    xbt_dynar_free(&keys);

    xbt_dynar_foreach(links, cpt, link) {
      TRY {
        link_list[nb_link++] = xbt_dict_get(link_set, link);
      }
      CATCH(e) {
        RETHROW1("Link %s not found (dict raised this exception: %s)", link);
      }
    }
    if (nb_link == 1)
      route_onehop_new(src_id, dst_id, link_list, nb_link);
  }

  xbt_dict_foreach(route_table, cursor, key, data) {
    char *link = NULL;
    nb_link = 0;
    links = (xbt_dynar_t) data;
    keys = xbt_str_split_str(key, sep);

    link_list_capacity = xbt_dynar_length(links);
    link_list = xbt_new(network_link_GTNETS_t, link_list_capacity);

    src_id = strtol(xbt_dynar_get_as(keys, 0, char *), &end, 16);
    dst_id = strtol(xbt_dynar_get_as(keys, 1, char *), &end, 16);
    xbt_dynar_free(&keys);

    xbt_dynar_foreach(links, cpt, link) {
      TRY {
        link_list[nb_link++] = xbt_dict_get(link_set, link);
      }
      CATCH(e) {
        RETHROW1("Link %s not found (dict raised this exception: %s)", link);
      }
    }
    if (nb_link >= 1)
      route_new(src_id, dst_id, link_list, nb_link);
  }

  xbt_dict_free(&route_table);
  gtnets_print_topology();
  XBT_OUT;
}

/* Main XML parsing */
static void define_callbacks(const char *file)
{
  surfxml_add_callback(STag_surfxml_router_cb_list, &parse_route_set_routers);
  surfxml_add_callback(STag_surfxml_link_cb_list, &parse_link_init);
  surfxml_add_callback(STag_surfxml_route_cb_list,
                       &parse_route_set_endpoints);
  surfxml_add_callback(ETag_surfxml_route_cb_list, &parse_route_set_route);
  surfxml_add_callback(ETag_surfxml_platform_cb_list, &add_route);
}

static const char *get_resource_name(void *resource_id)
{
  return ((network_card_GTNETS_t) resource_id)->name;
}

static xbt_dict_t get_properties(void *link)
{
  return ((network_card_GTNETS_t) link)->properties;
}


/* We do not care about this: only used for traces */
static int resource_used(void *resource_id)
{
  return 0;                     /* We don't care */
}

static int action_free(surf_action_t action)
{
  action->refcount--;
  if (!action->refcount) {
    xbt_swag_remove(action, action->state_set);
    /* KF: No explicit freeing needed for GTNeTS here */
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
  xbt_assert0(0, "Cannot cancel GTNetS flow");
  return;
}

static void action_recycle(surf_action_t action)
{
  xbt_assert0(0, "Cannot recycle GTNetS flow");
  return;
}

static void action_change_state(surf_action_t action,
                                e_surf_action_state_t state)
{
/*   if((state==SURF_ACTION_DONE) || (state==SURF_ACTION_FAILED)) */
/*     if(((surf_action_network_GTNETS_t)action)->variable) { */
/*       lmm_variable_disable(maxmin_system, ((surf_action_network_GTNETS_t)action)->variable); */
/*       ((surf_action_network_GTNETS_t)action)->variable = NULL; */
/*     } */

  surf_action_change_state(action, state);
  return;
}


/* share_resources() */
static double share_resources(double now)
{
  xbt_swag_t running_actions =
    surf_network_model->common_public.states.running_action_set;

  //get the first relevant value from the running_actions list
  if (!xbt_swag_size(running_actions))
    return -1.0;

  xbt_assert0(time_to_next_flow_completion,
              "Time to next flow completion not initialized!\n");

  time_to_next_flow_completion = gtnets_get_time_to_next_flow_completion();

  return time_to_next_flow_completion;
}

/* delta: by how many time units the simulation must advance */
/* In this function: change the state of actions that terminate */
/* The delta may not come from the network, and thus may be different (smaller)
   than the one returned by the function above */
/* If the delta is a network-caused min, then do not emulate any timer in the
   network simulation, otherwise fake a timer somehow to advance the simulation of min seconds */

static void update_actions_state(double now, double delta)
{
  surf_action_network_GTNETS_t action = NULL;
  //  surf_action_network_GTNETS_t next_action = NULL;
  xbt_swag_t running_actions =
    surf_network_model->common_public.states.running_action_set;

  /* If there are no renning flows, just return */
  if (time_to_next_flow_completion < 0.0) {
    return;
  }

  /*KF: if delta == time_to_next_flow_completion, too. */
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
      double remain = gtnets_get_flow_rx(action);
      DEBUG1("Remain value returned by GTNetS : %f", remain);
      //need to trust this remain value
      if (remain == 0) {
        action->generic_action.remains = 0;
      } else {
        action->generic_action.remains = action->generic_action.cost - remain;
      }
      DEBUG2("Action (%p) remains new value: %f", action,
             action->generic_action.remains);
    }

    for (i = 0; i < num_flows; i++) {
      action = (surf_action_network_GTNETS_t) (metadata[i]);

      action->generic_action.finish = now + time_to_next_flow_completion;
      action_change_state((surf_action_t) action, SURF_ACTION_DONE);
      DEBUG1("----> Action (%p) just terminated", action);
    }


  } else {                      /* run for a given number of seconds */
    if (gtnets_run(delta)) {
      xbt_assert0(0, "Cannot run GTNetS simulation");
    }
  }

  return;
}

/* UNUSED HERE: no traces */
static void update_resource_state(void *id,
                                  tmgr_trace_event_t event_type,
                                  double value, double date)
{
  xbt_assert0(0, "Cannot update model state for GTNetS simulation");
  return;
}

/* KF: Rate not supported */
static surf_action_t communicate(void *src, void *dst, double size,
                                 double rate)
{
  surf_action_network_GTNETS_t action = NULL;
  network_card_GTNETS_t card_src = src;
  network_card_GTNETS_t card_dst = dst;
/*
  int route_size = ROUTE_SIZE(card_src->id, card_dst->id);
  network_link_GTNETS_t *route = ROUTE(card_src->id, card_dst->id);
*/

/*
  xbt_assert2(route_size,"You're trying to send data from %s to %s but there is no connexion between these two cards.", card_src->name, card_dst->name);
*/

  action = xbt_new0(s_surf_action_network_GTNETS_t, 1);

  action->generic_action.refcount = 1;
  action->generic_action.cost = size;
  action->generic_action.remains = size;
  /* Max durations are not supported */
  action->generic_action.max_duration = NO_MAX_DURATION;
  action->generic_action.start = surf_get_clock();
  action->generic_action.finish = -1.0;
  action->generic_action.model_type = surf_network_model;

  action->generic_action.state_set =
    surf_network_model->common_public.states.running_action_set;

  xbt_swag_insert(action, action->generic_action.state_set);

  /* KF: Add a flow to the GTNets Simulation, associated to this action */
  if (gtnets_create_flow(card_src->id, card_dst->id, size, (void *) action)
      < 0) {
    xbt_assert2(0, "Not route between host %s and host %s", card_src->name,
                card_dst->name);
  }

  return (surf_action_t) action;
}

/* Suspend a flow() */
static void action_suspend(surf_action_t action)
{
  xbt_assert0(0,
              "action_suspend() not supported for the GTNets network model");
}

/* Resume a flow() */
static void action_resume(surf_action_t action)
{
  xbt_assert0(0,
              "action_resume() not supported for the GTNets network model");
}

/* Test whether a flow is suspended */
static int action_is_suspended(surf_action_t action)
{
  return 0;
}

static void finalize(void)
{
  xbt_dict_free(&link_set);

  surf_model_exit(surf_network_model);

  free(surf_network_model);
  surf_network_model = NULL;

  gtnets_finalize();
}

static void surf_network_model_init_internal(void)
{
  s_surf_action_t action;

  surf_network_model = xbt_new0(s_surf_network_model_t, 1);

  surf_model_init(surf_network_model);

  surf_network_model->common_public.name_service = name_service;
  surf_network_model->common_public.get_resource_name = get_resource_name;
  surf_network_model->common_public.action_get_state = surf_action_get_state;
  surf_network_model->common_public.action_use = action_use;
  surf_network_model->common_public.action_free = action_free;
  surf_network_model->common_public.action_cancel = action_cancel;
  surf_network_model->common_public.action_recycle = action_recycle;
  surf_network_model->common_public.action_change_state = action_change_state;
  surf_network_model->common_public.action_set_data = surf_action_set_data;
  surf_network_model->common_public.name = "network";

  surf_network_model->model_private->resource_used = resource_used;
  surf_network_model->model_private->share_resources = share_resources;
  surf_network_model->model_private->update_actions_state =
    update_actions_state;
  surf_network_model->model_private->update_resource_state =
    update_resource_state;
  surf_network_model->model_private->finalize = finalize;

  surf_network_model->common_public.suspend = action_suspend;
  surf_network_model->common_public.resume = action_resume;
  surf_network_model->common_public.is_suspended = action_is_suspended;

  surf_network_model->extension.network.communicate = communicate;

  /*for the props of the link */
  surf_network_model->common_public.get_properties = get_properties;

  link_set = xbt_dict_new();

  /* KF: Added the initialization for GTNetS interface */
  if (gtnets_initialize()) {
    xbt_assert0(0, "impossible to initialize GTNetS interface");
  }
}

#ifdef HAVE_GTNETS
void surf_network_model_init_GTNETS(const char *filename)
{
  if (surf_network_model)
    return;
  surf_network_model_init_internal();
  define_callbacks(filename);
  xbt_dynar_push(model_list, &surf_network_model);

  update_model_description(surf_network_model_description,
                           "GTNets", surf_network_model);
}
#endif
