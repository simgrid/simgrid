/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "network_private.h"

#define SG_TCP_CTE_GAMMA 20000.0

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(network, surf,
				"Logging specific to the SURF network module");

surf_network_resource_t surf_network_resource = NULL;

static xbt_dict_t network_link_set = NULL;
xbt_dict_t network_card_set = NULL;

int card_number = 0;
network_link_CM02_t **routing_table = NULL;
int *routing_table_size = NULL;
static network_link_CM02_t loopback = NULL;

static void create_routing_table(void)
{
  routing_table = xbt_new0(network_link_CM02_t *, card_number * card_number);
  routing_table_size = xbt_new0(int, card_number * card_number);
}

static void network_link_free(void *nw_link)
{
  free(((network_link_CM02_t)nw_link)->name);
  free(nw_link);
}

static network_link_CM02_t network_link_new(char *name,
				       double bw_initial,
				       tmgr_trace_t bw_trace,
				       double lat_initial,
				       tmgr_trace_t lat_trace,
				       e_surf_network_link_state_t
				       state_initial,
				       tmgr_trace_t state_trace,
				       e_surf_network_link_sharing_policy_t policy)
{
  network_link_CM02_t nw_link = xbt_new0(s_network_link_CM02_t, 1);


  nw_link->resource = (surf_resource_t) surf_network_resource;
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

  nw_link->constraint =
      lmm_constraint_new(maxmin_system, nw_link, nw_link->bw_current);

  if(policy == SURF_NETWORK_LINK_FATPIPE)
    lmm_constraint_shared(nw_link->constraint);

  xbt_dict_set(network_link_set, name, nw_link, network_link_free);

  return nw_link;
}

static void network_card_free(void *nw_card)
{
  free(((network_card_CM02_t)nw_card)->name);
  free(nw_card);
}

static int network_card_new(const char *card_name)
{
  network_card_CM02_t card = NULL;

  xbt_dict_get(network_card_set, card_name, (void *) &card);

  if (!card) {
    card = xbt_new0(s_network_card_CM02_t, 1);
    card->name = xbt_strdup(card_name);
    card->id = card_number++;
    xbt_dict_set(network_card_set, card_name, card, network_card_free);
  }
  return card->id;
}

static void route_new(int src_id, int dst_id, char **links, int nb_link)
{
  network_link_CM02_t *link_list = NULL;
  int i;

  ROUTE_SIZE(src_id, dst_id) = nb_link;
  link_list = (ROUTE(src_id, dst_id) = xbt_new0(network_link_CM02_t, nb_link));
  for (i = 0; i < nb_link; i++) {
    xbt_dict_get(network_link_set, links[i], (void *) &(link_list[i]));
    free(links[i]);
  }
  free(links);
}

static void parse_network_link(void)
{
  char *name;
  double bw_initial;
  tmgr_trace_t bw_trace;
  double lat_initial;
  tmgr_trace_t lat_trace;
  e_surf_network_link_state_t state_initial = SURF_NETWORK_LINK_ON;
  e_surf_network_link_sharing_policy_t policy_initial = SURF_NETWORK_LINK_SHARED;
  tmgr_trace_t state_trace;

  name = xbt_strdup(A_network_link_name);
  surf_parse_get_double(&bw_initial,A_network_link_bandwidth);
  surf_parse_get_trace(&bw_trace, A_network_link_bandwidth_file);
  surf_parse_get_double(&lat_initial,A_network_link_latency);
  surf_parse_get_trace(&lat_trace, A_network_link_latency_file);

  xbt_assert0((A_network_link_state==A_network_link_state_ON)||
	      (A_network_link_state==A_network_link_state_OFF),
	      "Invalid state")
  if (A_network_link_state==A_network_link_state_ON) 
    state_initial = SURF_NETWORK_LINK_ON;
  else if (A_network_link_state==A_network_link_state_OFF) 
    state_initial = SURF_NETWORK_LINK_OFF;

  if (A_network_link_sharing_policy==A_network_link_sharing_policy_SHARED) 
    policy_initial = SURF_NETWORK_LINK_SHARED;
  else if (A_network_link_sharing_policy==A_network_link_sharing_policy_FATPIPE) 
    policy_initial = SURF_NETWORK_LINK_FATPIPE;

  surf_parse_get_trace(&state_trace,A_network_link_state_file);

  network_link_new(name, bw_initial, bw_trace,
		   lat_initial, lat_trace, state_initial, state_trace,
		   policy_initial);
}

static int nb_link = 0;
static char **link_name = NULL;
static int src_id = -1;
static int dst_id = -1;

static void parse_route_set_endpoints(void)
{
  src_id = network_card_new(A_route_src);
  dst_id = network_card_new(A_route_dst);
  nb_link = 0;
  link_name = NULL;
}

static void parse_route_elem(void)
{
  nb_link++;
  link_name = xbt_realloc(link_name, (nb_link) * sizeof(char *));
  link_name[(nb_link) - 1] = xbt_strdup(A_route_element_name);
}

static void parse_route_set_route(void)
{
  route_new(src_id, dst_id, link_name, nb_link);
}

static void parse_file(const char *file)
{
  int i;

  /* Figuring out the network links */
  surf_parse_reset_parser();
  ETag_network_link_fun=parse_network_link;
  surf_parse_open(file);
  xbt_assert1((!surf_parse()),"Parse error in %s",file);
  surf_parse_close();

  /* Figuring out the network cards used */
  surf_parse_reset_parser();
  STag_route_fun=parse_route_set_endpoints;
  surf_parse_open(file);
  xbt_assert1((!surf_parse()),"Parse error in %s",file);
  surf_parse_close();

  create_routing_table();

  /* Building the routes */
  surf_parse_reset_parser();
  STag_route_fun=parse_route_set_endpoints;
  ETag_route_element_fun=parse_route_elem;
  ETag_route_fun=parse_route_set_route;
  surf_parse_open(file);
  xbt_assert1((!surf_parse()),"Parse error in %s",file);
  surf_parse_close();

  /* Adding loopback if needed */
    
  for (i = 0; i < card_number; i++) 
    if(!ROUTE_SIZE(i,i)) {
      if(!loopback)
	loopback = network_link_new(xbt_strdup("__MSG_loopback__"), 
				   498.00, NULL, 0.000015, NULL, 
				   SURF_NETWORK_LINK_ON, NULL,
				   SURF_NETWORK_LINK_FATPIPE);
      ROUTE_SIZE(i,i)=1;
      ROUTE(i,i) = xbt_new0(network_link_CM02_t, 1);
      ROUTE(i,i)[0] = loopback;
    }
}

static void *name_service(const char *name)
{
  network_card_CM02_t card = NULL;

  xbt_dict_get(network_card_set, name, (void *) &card);

  return card;
}

static const char *get_resource_name(void *resource_id)
{
  return ((network_card_CM02_t) resource_id)->name;
}

static int resource_used(void *resource_id)
{
  return lmm_constraint_used(maxmin_system,
			     ((network_link_CM02_t) resource_id)->constraint);
}

static int action_free(surf_action_t action)
{
  action->using--;
  if(!action->using) {
    xbt_swag_remove(action, action->state_set);
    if(((surf_action_network_CM02_t)action)->variable)
      lmm_variable_free(maxmin_system, ((surf_action_network_CM02_t)action)->variable);
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
  return;
}

static void action_recycle(surf_action_t action)
{
  return;
}

static void action_change_state(surf_action_t action,
				e_surf_action_state_t state)
{
/*   if((state==SURF_ACTION_DONE) || (state==SURF_ACTION_FAILED)) */
/*     if(((surf_action_network_CM02_t)action)->variable) { */
/*       lmm_variable_disable(maxmin_system, ((surf_action_network_CM02_t)action)->variable); */
/*       ((surf_action_network_CM02_t)action)->variable = NULL; */
/*     } */

  surf_action_change_state(action, state);
  return;
}

static double share_resources(double now)
{
  s_surf_action_network_CM02_t s_action;
  surf_action_network_CM02_t action = NULL;
  xbt_swag_t running_actions = surf_network_resource->common_public->states.running_action_set;
  double min = generic_maxmin_share_resources(running_actions,
					      xbt_swag_offset(s_action, variable));

  xbt_swag_foreach(action, running_actions) {
    if(action->latency>0) {
      if(min<0) min = action->latency;
      else if (action->latency<min) min = action->latency;
    }
  }

  return min;
}

static void update_actions_state(double now, double delta)
{
  double deltap = 0.0;
  surf_action_network_CM02_t action = NULL;
  surf_action_network_CM02_t next_action = NULL;
  xbt_swag_t running_actions =
      surf_network_resource->common_public->states.running_action_set;
  xbt_swag_t failed_actions =
      surf_network_resource->common_public->states.failed_action_set;

  xbt_swag_foreach_safe(action, next_action, running_actions) {
    deltap = delta;
    if (action->latency > 0) {
      if (action->latency > deltap) {
	surf_double_update(&(action->latency),deltap);
	deltap = 0.0;
      } else {
	surf_double_update(&(deltap), action->latency);
	action->latency = 0.0;
      }
      if ((action->latency == 0.0) && !(action->suspended)) 
	lmm_update_variable_weight(maxmin_system, action->variable, 1.0);
    }
    surf_double_update(&(action->generic_action.remains),
	lmm_variable_getvalue(action->variable) * deltap);
    if (action->generic_action.max_duration != NO_MAX_DURATION)
      surf_double_update(&(action->generic_action.max_duration), delta);

    /*   if(action->generic_action.remains<.00001) action->generic_action.remains=0; */

    if (action->generic_action.remains <= 0) {
      action->generic_action.finish = surf_get_clock();
      action_change_state((surf_action_t) action, SURF_ACTION_DONE);
    } else if ((action->generic_action.max_duration != NO_MAX_DURATION) &&
	       (action->generic_action.max_duration <= 0)) {
      action->generic_action.finish = surf_get_clock();
      action_change_state((surf_action_t) action, SURF_ACTION_DONE);
    } else {			/* Need to check that none of the resource has failed */
      lmm_constraint_t cnst = NULL;
      int i = 0;
      network_link_CM02_t nw_link = NULL;

      while ((cnst =
	      lmm_get_cnst_from_var(maxmin_system, action->variable,
				    i++))) {
	nw_link = lmm_constraint_id(cnst);
	if (nw_link->state_current == SURF_NETWORK_LINK_OFF) {
	  action->generic_action.finish = surf_get_clock();
	  action_change_state((surf_action_t) action, SURF_ACTION_FAILED);
	  break;
	}
      }
    }
  }

  return;
}

static void update_resource_state(void *id,
				  tmgr_trace_event_t event_type,
				  double value)
{
  network_link_CM02_t nw_link = id;
  /*   printf("[" "%lg" "] Asking to update network card \"%s\" with value " */
  /* 	 "%lg" " for event %p\n", surf_get_clock(), nw_link->name, */
  /* 	 value, event_type); */

  if (event_type == nw_link->bw_event) {
    nw_link->bw_current = value;
    lmm_update_constraint_bound(maxmin_system, nw_link->constraint,
				nw_link->bw_current);
  } else if (event_type == nw_link->lat_event) {
    double delta = value - nw_link->lat_current;
    lmm_variable_t var = NULL;
    surf_action_network_CM02_t action = NULL;

    nw_link->lat_current = value;
    while (lmm_get_var_from_cnst(maxmin_system, nw_link->constraint, &var)) {
      action = lmm_variable_id(var);
      action->lat_current += delta;
      if(action->rate<0)
	lmm_update_variable_bound(maxmin_system, action->variable,
				  SG_TCP_CTE_GAMMA / action->lat_current);
      else 
	lmm_update_variable_bound(maxmin_system, action->variable,
				  min(action->rate,SG_TCP_CTE_GAMMA / action->lat_current));
    }
  } else if (event_type == nw_link->state_event) {
    if (value > 0)
      nw_link->state_current = SURF_NETWORK_LINK_ON;
    else
      nw_link->state_current = SURF_NETWORK_LINK_OFF;
  } else {
    CRITICAL0("Unknown event ! \n");
    xbt_abort();
  }

  return;
}

static surf_action_t communicate(void *src, void *dst, double size, double rate)
{
  surf_action_network_CM02_t action = NULL;
  network_card_CM02_t card_src = src;
  network_card_CM02_t card_dst = dst;
  int route_size = ROUTE_SIZE(card_src->id, card_dst->id);
  network_link_CM02_t *route = ROUTE(card_src->id, card_dst->id);
  int i;

  action = xbt_new0(s_surf_action_network_CM02_t, 1);

  action->generic_action.using = 1;
  action->generic_action.cost = size;
  action->generic_action.remains = size;
  action->generic_action.max_duration = NO_MAX_DURATION;
  action->generic_action.start = -1.0;
  action->generic_action.finish = -1.0;
  action->generic_action.resource_type =
      (surf_resource_t) surf_network_resource;
  action->suspended = 0;  /* Should be useless because of the 
			     calloc but it seems to help valgrind... */
  action->generic_action.state_set =
      surf_network_resource->common_public->states.running_action_set;

  xbt_swag_insert(action, action->generic_action.state_set);
  action->rate = rate;

  action->latency = 0.0;
  for (i = 0; i < route_size; i++)
    action->latency += route[i]->lat_current;
  action->lat_current = action->latency;

  if(action->latency>0)
    action->variable = lmm_variable_new(maxmin_system, action, 0.0, -1.0,
					route_size);
  else 
    action->variable = lmm_variable_new(maxmin_system, action, 1.0, -1.0,
					route_size);

  if(action->rate<0) {
    if(action->lat_current>0)
      lmm_update_variable_bound(maxmin_system, action->variable,
				SG_TCP_CTE_GAMMA / action->lat_current);
    else
      lmm_update_variable_bound(maxmin_system, action->variable, -1.0);
  } else {
    if(action->lat_current>0)
      lmm_update_variable_bound(maxmin_system, action->variable,
				min(action->rate,SG_TCP_CTE_GAMMA / action->lat_current));
    else
      lmm_update_variable_bound(maxmin_system, action->variable, action->rate);
  }

  for (i = 0; i < route_size; i++)
    lmm_expand(maxmin_system, route[i]->constraint, action->variable, 1.0);

  if(route_size == 0) {
    action_change_state((surf_action_t) action, SURF_ACTION_DONE);
  }

  return (surf_action_t) action;
}

static void action_suspend(surf_action_t action)
{
  ((surf_action_network_CM02_t) action)->suspended = 1;
  lmm_update_variable_weight(maxmin_system,
			     ((surf_action_network_CM02_t) action)->variable, 0.0);
}

static void action_resume(surf_action_t action)
{
  lmm_update_variable_weight(maxmin_system,
			     ((surf_action_network_CM02_t) action)->variable, 1.0);
  ((surf_action_network_CM02_t) action)->suspended = 0;
}

static int action_is_suspended(surf_action_t action)
{
  return ((surf_action_network_CM02_t) action)->suspended;
}

static void action_set_max_duration(surf_action_t action, double duration)
{
  action->max_duration = duration;
}

static void finalize(void)
{
  int i,j;

  xbt_dict_free(&network_card_set);
  xbt_dict_free(&network_link_set);
  xbt_swag_free(surf_network_resource->common_public->states.
		ready_action_set);
  xbt_swag_free(surf_network_resource->common_public->states.
		running_action_set);
  xbt_swag_free(surf_network_resource->common_public->states.
		failed_action_set);
  xbt_swag_free(surf_network_resource->common_public->states.
		done_action_set);
  free(surf_network_resource->common_public);
  free(surf_network_resource->common_private);
  free(surf_network_resource->extension_public);

  free(surf_network_resource);
  surf_network_resource = NULL;

  loopback = NULL;
  for (i = 0; i < card_number; i++) 
    for (j = 0; j < card_number; j++) 
      free(ROUTE(i,j));
  free(routing_table);
  routing_table = NULL;
  free(routing_table_size);
  routing_table_size = NULL;
  card_number = 0;
}

static void surf_network_resource_init_internal(void)
{
  s_surf_action_t action;

  surf_network_resource = xbt_new0(s_surf_network_resource_t, 1);

  surf_network_resource->common_private =
      xbt_new0(s_surf_resource_private_t, 1);
  surf_network_resource->common_public =
      xbt_new0(s_surf_resource_public_t, 1);
  surf_network_resource->extension_public =
      xbt_new0(s_surf_network_resource_extension_public_t, 1);

  surf_network_resource->common_public->states.ready_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_network_resource->common_public->states.running_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_network_resource->common_public->states.failed_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_network_resource->common_public->states.done_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));

  surf_network_resource->common_public->name_service = name_service;
  surf_network_resource->common_public->get_resource_name =
      get_resource_name;
  surf_network_resource->common_public->action_get_state =
      surf_action_get_state;
  surf_network_resource->common_public->action_free = action_free;
  surf_network_resource->common_public->action_use = action_use;
  surf_network_resource->common_public->action_cancel = action_cancel;
  surf_network_resource->common_public->action_recycle = action_recycle;
  surf_network_resource->common_public->action_change_state =
      action_change_state;
  surf_network_resource->common_public->action_set_data = surf_action_set_data;
  surf_network_resource->common_public->name = "network";

  surf_network_resource->common_private->resource_used = resource_used;
  surf_network_resource->common_private->share_resources = share_resources;
  surf_network_resource->common_private->update_actions_state =
      update_actions_state;
  surf_network_resource->common_private->update_resource_state =
      update_resource_state;
  surf_network_resource->common_private->finalize = finalize;

  surf_network_resource->common_public->suspend = action_suspend;
  surf_network_resource->common_public->resume = action_resume;
  surf_network_resource->common_public->is_suspended = action_is_suspended;
  surf_cpu_resource->common_public->set_max_duration = action_set_max_duration;

  surf_network_resource->extension_public->communicate = communicate;

  network_link_set = xbt_dict_new();
  network_card_set = xbt_dict_new();

  xbt_assert0(maxmin_system, "surf_init has to be called first!");
}

/***************************************************************************/
/* The nice TCP sharing model designed by Loris Marchal and Henri Casanova */
/***************************************************************************/
/* @TechReport{      rr-lip2002-40, */
/*   author        = {Henri Casanova and Loris Marchal}, */
/*   institution   = {LIP}, */
/*   title         = {A Network Model for Simulation of Grid Application}, */
/*   number        = {2002-40}, */
/*   month         = {oct}, */
/*   year          = {2002} */
/* } */
void surf_network_resource_init_CM02(const char *filename)
{
  if (surf_network_resource)
    return;
  surf_network_resource_init_internal();
  parse_file(filename);
  xbt_dynar_push(resource_list, &surf_network_resource);
}
