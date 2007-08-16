/* 	$Id$	 */

/* Copyright (c) 2005 Henri Casanova. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "network_gtnets_private.h"
#include "gtnets/gtnets_interface.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_network_gtnets, surf,
				"Logging specific to the SURF network module");

/* surf_network_model_t surf_network_model = NULL; */
/*static xbt_dict_t network_link_set = NULL;*/

/* xbt_dict_t network_card_set = NULL; */

#if 0
static int card_number = 0;
static network_link_GTNETS_t **routing_table = NULL;
static int *routing_table_size = NULL;

#define ROUTE(i,j) routing_table[(i)+(j)*card_number]
#define ROUTE_SIZE(i,j) routing_table_size[(i)+(j)*card_number]
#endif

/** QUESTIONS for GTNetS integration
 **   1. Check that we did the right thing with name_service and get_model_name
 **   2. Right now there is no "kill flow" in our GTNetS implementation. Do we
 **      need to do something about this?
 **   3. We ignore the fact there is some max_duration on flows (see #2 above)
 **   4. share_models() returns a duration, not a date, right?
 **   5. We don't suppoer "rates"
 **   6. We don't update "remaining" for ongoing flows. Is it bad?
 **/

/* Free memory for a network link */
static void network_link_free(void *nw_link)
{
  free(((network_link_GTNETS_t) nw_link)->name);
  free(nw_link);
}

/* Instantiate a new network link */
/* name: some name for the link, from the XML */
/* bw: The bandwidth value            */
/* lat: The latency value             */
static void network_link_new(char *name, double bw, double lat)
{
  static int link_count = -1;
  network_link_GTNETS_t gtnets_link;

  /* KF: Check that the link wasn't added before */
  if (xbt_dict_get_or_null(network_link_set, name)) {
    return;
  }

  /* KF: Increment the link counter for GTNetS */
  link_count++;

/*
  nw_link->model = (surf_model_t) surf_network_model;
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
  xbt_dict_set(network_link_set, name, gtnets_link, network_link_free);

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

  /* KF: Check that we haven't seen the network card before */
  network_card_GTNETS_t card =
      xbt_dict_get_or_null(network_card_set, name);

  if (!card) {
    /* KF: Increment the card counter for GTNetS */
    card_count++;

    /* KF: just use the dictionary to map link names to link indices */
    card = xbt_new0(s_network_card_GTNETS_t, 1);
    card->name = xbt_strdup(name);
    card->id = card_count;
    xbt_dict_set(network_card_set, name, card, network_card_free);
  }

  /* KF: just return the GTNetS ID as the SURF ID */
  return card->id;
}

/* Instantiate a new route: MODIFY BY KF */
static void route_new(int src_id, int dst_id, char **links, int nb_link)
{
#if 0
  network_link_GTNETS_t *link_list = NULL;
  int i;

  ROUTE_SIZE(src_id, dst_id) = nb_link;
  link_list = (ROUTE(src_id, dst_id) =
	       xbt_new0(network_link_GTNETS_t, nb_link));
  for (i = 0; i < nb_link; i++) {
    link_list[i] = xbt_dict_get_or_null(network_link_set, links[i]);
    free(links[i]);
  }
  free(links);
#endif
  int i;
  int *gtnets_links;

  /* KF: Build the list of gtnets link IDs */
  gtnets_links = (int *) calloc(nb_link, sizeof(int));
  for (i = 0; i < nb_link; i++) {
    gtnets_links[i] =
	((network_link_GTNETS_t)
	 (xbt_dict_get(network_link_set, links[i])))->id;
  }

  /* KF: Create the GTNets route */
  if (gtnets_add_route(src_id, dst_id, gtnets_links, nb_link)) {
    xbt_assert0(0, "Cannot create GTNetS route");
  }
}

/* Instantiate a new route: MODIFY BY KF */
static void route_onehop_new(int src_id, int dst_id, char **links,
			     int nb_link)
{
  int linkid;

  if (nb_link != 1) {
    xbt_assert0(0, "In onehop_new, nb_link should be 1");
  }

  /* KF: Build the list of gtnets link IDs */
  linkid =
      ((network_link_GTNETS_t)
       (xbt_dict_get(network_link_set, links[0])))->id;

  /* KF: Create the GTNets route */
  if (gtnets_add_onehop_route(src_id, dst_id, linkid)) {
    xbt_assert0(0, "Cannot create GTNetS route");
  }
}

/* Parse the XML for a network link */
static void parse_network_link(void)
{
  char *name;
  double bw;
  double lat;
  e_surf_network_link_state_t state;

  name = xbt_strdup(A_surfxml_network_link_name);
  surf_parse_get_double(&bw, A_surfxml_network_link_bandwidth);
  surf_parse_get_double(&lat, A_surfxml_network_link_latency);
  state = SURF_NETWORK_LINK_ON;

  /* Print values when no traces are specified */
  {
    tmgr_trace_t bw_trace;
    tmgr_trace_t state_trace;
    tmgr_trace_t lat_trace;

    surf_parse_get_trace(&bw_trace, A_surfxml_network_link_bandwidth_file);
    surf_parse_get_trace(&lat_trace, A_surfxml_network_link_latency_file);
    surf_parse_get_trace(&state_trace, A_surfxml_network_link_state_file);

    /*TODO Where is WARNING0 defined??? */
#if 0
    if (bw_trace)
      WARNING0
	  ("The GTNetS network model doesn't support bandwidth state traces");
    if (lat_trace)
      WARNING0
	  ("The GTNetS network model doesn't support latency state traces");
    if (state_trace)
      WARNING0
	  ("The GTNetS network model doesn't support link state traces");
#endif
  }


  /* KF: remove several arguments to network_link_new */
  network_link_new(name, bw, lat);
}

static int nb_link = 0;
static char **link_name = NULL;
static int src_id = -1;
static int dst_id = -1;

/* Parses a route from the XML: UNMODIFIED BY HC */
static void parse_route_set_endpoints(void)
{
  src_id = network_card_new(A_surfxml_route_src);
  dst_id = network_card_new(A_surfxml_route_dst);
  nb_link = 0;
  link_name = NULL;
}

/* KF*/
static void parse_route_set_routers(void)
{
  int id = network_card_new(A_surfxml_router_name);

  /* KF: Create the GTNets router */
  if (gtnets_add_router(id)) {
    xbt_assert0(0, "Cannot add GTNetS router");
  }
}

/* Parses a route element from the XML: UNMODIFIED BY HC */
static void parse_route_elem(void)
{
  nb_link++;
  link_name = xbt_realloc(link_name, (nb_link) * sizeof(char *));
  link_name[(nb_link) - 1] = xbt_strdup(A_surfxml_route_element_name);
}

/* Create the route (more than one hops): MODIFIED BY KF */
static void parse_route_set_route(void)
{
  if (nb_link > 1)
    route_new(src_id, dst_id, link_name, nb_link);
}

/* Create the one-hope route: BY KF */
static void parse_route_set_onehop_route(void)
{
  if (nb_link == 1)
    route_onehop_new(src_id, dst_id, link_name, nb_link);
}

/* Main XML parsing */
static void parse_file(const char *file)
{
  /* Figuring out the network links */
  surf_parse_reset_parser();
  ETag_surfxml_network_link_fun = parse_network_link;
  surf_parse_open(file);
  xbt_assert1((!surf_parse()), "Parse error in %s", file);
  surf_parse_close();

  /* Figuring out the network cards used */
  /* KF
     surf_parse_reset_parser();
     STag_surfxml_route_fun=parse_route_set_endpoints;
     surf_parse_open(file);
     xbt_assert1((!surf_parse()),"Parse error in %s",file);
     surf_parse_close();
   */

  /* KF: Figuring out the router (considered as part of
     network cards) used. */
  surf_parse_reset_parser();
  STag_surfxml_router_fun = parse_route_set_routers;
  surf_parse_open(file);
  xbt_assert1((!surf_parse()), "Parse error in %s", file);
  surf_parse_close();

  /* Building the one-hop routes */
  surf_parse_reset_parser();
  STag_surfxml_route_fun = parse_route_set_endpoints;
  ETag_surfxml_route_element_fun = parse_route_elem;
  ETag_surfxml_route_fun = parse_route_set_onehop_route;
  surf_parse_open(file);
  xbt_assert1((!surf_parse()), "Parse error in %s", file);
  surf_parse_close();

  /* Building the routes */
  surf_parse_reset_parser();
  STag_surfxml_route_fun = parse_route_set_endpoints;
  ETag_surfxml_route_element_fun = parse_route_elem;
  ETag_surfxml_route_fun = parse_route_set_route;
  surf_parse_open(file);
  xbt_assert1((!surf_parse()), "Parse error in %s", file);
  surf_parse_close();
}

static void *name_service(const char *name)
{
  return xbt_dict_get_or_null(network_card_set, name);
}

static const char *get_model_name(void *model_id)
{
  return ((network_card_GTNETS_t) model_id)->name;
}

/* We do not care about this: only used for traces */
static int model_used(void *model_id)
{
  return 0;			/* We don't care */
}

static int action_free(surf_action_t action)
{
  action->using--;
  if (!action->using) {
    xbt_swag_remove(action, action->state_set);
    /* KF: No explicit freeing needed for GTNeTS here */
    free(action);
    return 1;
  }
  return 0;
}

static void action_use(surf_action_t action)
{
  action->using++;
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


/* share_models() */
static double share_models(double now)
{
#if 0
  s_surf_action_network_GTNETS_t s_action;
  surf_action_network_GTNETS_t action = NULL;
  xbt_swag_t running_actions =
      surf_network_model->common_public->states.running_action_set;
#endif

  return gtnets_get_time_to_next_flow_completion();
}

/* delta: by how many time units the simulation must advance */
/* In this function: change the state of actions that terminate */
/* The delta may not come from the network, and thus may be different (smaller) 
   than the one returned by the function above */
/* If the delta is a network-caused min, then do not emulate any timer in the
   network simulation, otherwise fake a timer somehow to advance the simulation of min seconds */

static void update_actions_state(double now, double delta)
{
#if 0
  surf_action_network_GTNETS_t action = NULL;
  surf_action_network_GTNETS_t next_action = NULL;
  xbt_swag_t running_actions =
      surf_network_model->common_public->states.running_action_set;
#endif

  double time_to_next_flow_completion =
      gtnets_get_time_to_next_flow_completion();

  /* If there are no renning flows, just return */
  if (time_to_next_flow_completion < 0.0) {
    return;
  }

  /*KF: if delta == time_to_next_flow_completion, too. */
  if (time_to_next_flow_completion <= delta) {	/* run until the first flow completes */
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

    for (i = 0; i < num_flows; i++) {
      surf_action_network_GTNETS_t action =
	  (surf_action_network_GTNETS_t) (metadata[i]);

      action->generic_action.remains = 0;
      action->generic_action.finish = now + time_to_next_flow_completion;
      action_change_state((surf_action_t) action, SURF_ACTION_DONE);
      /* TODO: Anything else here? */
    }
  } else {			/* run for a given number of seconds */
    if (gtnets_run(delta)) {
      xbt_assert0(0, "Cannot run GTNetS simulation");
    }
  }

  return;
}

/* UNUSED HERE: no traces */
static void update_model_state(void *id,
				  tmgr_trace_event_t event_type,
				  double value)
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

  action->generic_action.using = 1;
  action->generic_action.cost = size;
  action->generic_action.remains = size;
  /* Max durations are not supported */
  action->generic_action.max_duration = NO_MAX_DURATION;
  action->generic_action.start = surf_get_clock();
  action->generic_action.finish = -1.0;
  action->generic_action.model_type =
      (surf_model_t) surf_network_model;

  action->generic_action.state_set =
      surf_network_model->common_public->states.running_action_set;

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
#if 0
  int i, j;
#endif
  xbt_dict_free(&network_card_set);
  xbt_dict_free(&network_link_set);
  xbt_swag_free(surf_network_model->common_public->states.
		ready_action_set);
  xbt_swag_free(surf_network_model->common_public->states.
		running_action_set);
  xbt_swag_free(surf_network_model->common_public->states.
		failed_action_set);
  xbt_swag_free(surf_network_model->common_public->states.
		done_action_set);
  free(surf_network_model->common_public);
  free(surf_network_model->common_private);
  free(surf_network_model->extension_public);

  free(surf_network_model);
  surf_network_model = NULL;

#if 0
  for (i = 0; i < card_number; i++)
    for (j = 0; j < card_number; j++)
      free(ROUTE(i, j));
  free(routing_table);
  routing_table = NULL;
  free(routing_table_size);
  routing_table_size = NULL;
  card_number = 0;
#endif

  /* ADDED BY KF */
  gtnets_finalize();
  /* END ADDITION */
}

static void surf_network_model_init_internal(void)
{
  s_surf_action_t action;

  surf_network_model = xbt_new0(s_surf_network_model_t, 1);

  surf_network_model->common_private =
      xbt_new0(s_surf_model_private_t, 1);
  surf_network_model->common_public =
      xbt_new0(s_surf_model_public_t, 1);
  surf_network_model->extension_public =
      xbt_new0(s_surf_network_model_extension_public_t, 1);

  surf_network_model->common_public->states.ready_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_network_model->common_public->states.running_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_network_model->common_public->states.failed_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_network_model->common_public->states.done_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));

  surf_network_model->common_public->name_service = name_service;
  surf_network_model->common_public->get_model_name =
      get_model_name;
  surf_network_model->common_public->action_get_state =
      surf_action_get_state;
  surf_network_model->common_public->action_use = action_use;
  surf_network_model->common_public->action_free = action_free;
  surf_network_model->common_public->action_cancel = action_cancel;
  surf_network_model->common_public->action_recycle = action_recycle;
  surf_network_model->common_public->action_change_state =
      action_change_state;
  surf_network_model->common_public->action_set_data =
      surf_action_set_data;
  surf_network_model->common_public->name = "network";

  surf_network_model->common_private->model_used = model_used;
  surf_network_model->common_private->share_models = share_models;
  surf_network_model->common_private->update_actions_state =
      update_actions_state;
  surf_network_model->common_private->update_model_state =
      update_model_state;
  surf_network_model->common_private->finalize = finalize;

  surf_network_model->common_public->suspend = action_suspend;
  surf_network_model->common_public->resume = action_resume;
  surf_network_model->common_public->is_suspended = action_is_suspended;

  surf_network_model->extension_public->communicate = communicate;

  network_link_set = xbt_dict_new();
  network_card_set = xbt_dict_new();

  /* HC: I am assuming that this stays in for simulation of compute tasks */
  xbt_assert0(maxmin_system, "surf_init has to be called first!");

  /* KF: Added the initialization for GTNetS interface */
  if (gtnets_initialize()) {
    xbt_assert0(0, "impossible to initialize GTNetS interface");
  }
}

void surf_network_model_init_GTNETS(const char *filename)
{
  if (surf_network_model)
    return;
  surf_network_model_init_internal();
  parse_file(filename);
  xbt_dynar_push(model_list, &surf_network_model);

  update_model_description(surf_network_model_description,
			      surf_network_model_description_size,
			      "GTNets",
			      (surf_model_t) surf_network_model);
}
