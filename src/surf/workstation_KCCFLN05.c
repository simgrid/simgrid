/* 	$Id$	 */

/* Copyright (c) 2005 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/dict.h"
#include "workstation_KCCFLN05_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(workstation_KCCFLN05, surf,
				"Logging specific to the SURF workstation module (KCCFLN05)");


static lmm_system_t maxmin_system_cpu_KCCFLN05 = NULL;
static lmm_system_t maxmin_system_network_KCCFLN05 = NULL;
static xbt_dict_t network_link_set = NULL;
static int nb_workstation = 0;
s_route_KCCFLN05_t *routing_table = NULL;
#define ROUTE(i,j) routing_table[(i)+(j)*nb_workstation]

/**************************************/
/************* CPU object *************/
/**************************************/

/************ workstation creation *********/
static void workstation_free(void *workstation)
{
  xbt_free(((workstation_KCCFLN05_t)workstation)->name);
  xbt_dynar_free(&(((workstation_KCCFLN05_t)workstation)->incomming_communications));
  xbt_dynar_free(&(((workstation_KCCFLN05_t)workstation)->outgoing_communications));
  xbt_free(workstation);
}

static workstation_KCCFLN05_t workstation_new(const char *name,
					      double power_scale,
					      double power_initial,
					      tmgr_trace_t power_trace,
					      e_surf_cpu_state_t state_initial,
					      tmgr_trace_t state_trace,
					      double interference_send,
					      double interference_recv,
					      double interference_send_recv,
					      double max_outgoing_rate)
{
  workstation_KCCFLN05_t workstation = xbt_new0(s_workstation_KCCFLN05_t, 1);

  workstation->resource = (surf_resource_t) surf_workstation_resource;
  workstation->name = xbt_strdup(name);
  workstation->id = nb_workstation++;

  workstation->power_scale = power_scale;
  workstation->power_current = power_initial;
  if (power_trace)
    workstation->power_event =
	tmgr_history_add_trace(history, power_trace, 0.0, 0, workstation);

  workstation->state_current = state_initial;
  if (state_trace)
    workstation->state_event =
	tmgr_history_add_trace(history, state_trace, 0.0, 0, workstation);

  workstation->interference_send=interference_send;
  workstation->interference_recv=interference_recv;
  workstation->interference_send_recv=interference_send_recv;

  workstation->constraint =
      lmm_constraint_new(maxmin_system_cpu_KCCFLN05, workstation,
			 workstation->power_current * workstation->power_scale);
  if(max_outgoing_rate>0) 
    workstation->bus=
      lmm_constraint_new(maxmin_system_cpu_KCCFLN05, workstation,
			 max_outgoing_rate);

  workstation->incomming_communications = 
    xbt_dynar_new(sizeof(surf_action_network_KCCFLN05_t),NULL);
  workstation->outgoing_communications = 
    xbt_dynar_new(sizeof(surf_action_network_KCCFLN05_t),NULL);

  xbt_dict_set(workstation_set, name, workstation, workstation_free);

  return workstation;
}

static void parse_workstation(void)
{
  double power_scale = 0.0;
  double power_initial = 0.0;
  tmgr_trace_t power_trace = NULL;
  e_surf_cpu_state_t state_initial = SURF_CPU_OFF;
  tmgr_trace_t state_trace = NULL;
  double interference_send = 0.0;
  double interference_recv = 0.0;
  double interference_send_recv = 0.0;
  double max_outgoing_rate = -1.0;

  surf_parse_get_double(&power_scale,A_cpu_power);
  surf_parse_get_double(&power_initial,A_cpu_availability);
  surf_parse_get_trace(&power_trace,A_cpu_availability_file);

  xbt_assert0((A_cpu_state==A_cpu_state_ON)||
	      (A_cpu_state==A_cpu_state_OFF),
	      "Invalid state")
  if (A_cpu_state==A_cpu_state_ON) state_initial = SURF_CPU_ON;
  if (A_cpu_state==A_cpu_state_OFF) state_initial = SURF_CPU_OFF;
  surf_parse_get_trace(&state_trace,A_cpu_state_file);

  surf_parse_get_double(&interference_send,A_cpu_interference_send);
  surf_parse_get_double(&interference_recv,A_cpu_interference_recv);
  surf_parse_get_double(&interference_send_recv,A_cpu_interference_send_recv);
  surf_parse_get_double(&max_outgoing_rate,A_cpu_max_outgoing_rate);

  workstation_new(A_cpu_name, power_scale, power_initial, power_trace, state_initial,
		  state_trace, interference_send, interference_recv,
		  interference_send_recv, max_outgoing_rate);
}

/*********** resource management ***********/

static void *name_service(const char *name)
{
  void *workstation = NULL;

  xbt_dict_get(workstation_set, name, &workstation);

  return workstation;
}

static const char *get_resource_name(void *resource_id)
{
  return ((workstation_KCCFLN05_t) resource_id)->name;
}

static int cpu_used(void *resource_id)
{
  return lmm_constraint_used(maxmin_system_cpu_KCCFLN05,
			     ((workstation_KCCFLN05_t) resource_id)->constraint);
}

static int link_used(void *resource_id)
{
  return lmm_constraint_used(maxmin_system_network_KCCFLN05,
			     ((network_link_KCCFLN05_t) resource_id)->constraint);
}

static e_surf_cpu_state_t get_state(void *cpu)
{
  return ((workstation_KCCFLN05_t) cpu)->state_current;
}

static void update_cpu_KCCFLN05_state(void *id,
				      tmgr_trace_event_t event_type,
				      double value)
{
  workstation_KCCFLN05_t cpu = id;

  if (event_type == cpu->power_event) {
    cpu->power_current = value;
    /*** The bound is updated in share_cpu_KCCFLN05_resources ***/
    /*     lmm_update_constraint_bound(maxmin_system_cpu_KCCFLN05, cpu->constraint, */
    /* 				cpu->power_current * cpu->power_scale); */
  } else if (event_type == cpu->state_event) {
    if (value > 0)
      cpu->state_current = SURF_CPU_ON;
    else
      cpu->state_current = SURF_CPU_OFF;
  } else {
    CRITICAL0("Unknown event ! \n");
    xbt_abort();
  }

  return;
}

/**************************************/
/*********** network object ***********/
/**************************************/

static void create_routing_table(void)
{
  routing_table = xbt_new0(s_route_KCCFLN05_t, nb_workstation * nb_workstation);
}

static void network_link_free(void *nw_link)
{
  xbt_free(((network_link_KCCFLN05_t)nw_link)->name);
  xbt_free(nw_link);
}

static network_link_KCCFLN05_t network_link_new(char *name,
						 double bw_initial,
						 tmgr_trace_t bw_trace,
						 e_surf_network_link_state_t
						 state_initial,
						 tmgr_trace_t state_trace)
{
  network_link_KCCFLN05_t nw_link = xbt_new0(s_network_link_KCCFLN05_t, 1);


  nw_link->resource = (surf_resource_t) surf_network_resource;
  nw_link->name = name;
  nw_link->bw_current = bw_initial;
  if (bw_trace)
    nw_link->bw_event =
	tmgr_history_add_trace(history, bw_trace, 0.0, 0, nw_link);
  nw_link->state_current = state_initial;
  if (state_trace)
    nw_link->state_event =
	tmgr_history_add_trace(history, state_trace, 0.0, 0, nw_link);

  nw_link->constraint =
      lmm_constraint_new(maxmin_system_network_KCCFLN05, nw_link, nw_link->bw_current);

  xbt_dict_set(network_link_set, name, nw_link, network_link_free);

  return nw_link;
}

static void parse_network_link(void)
{
  char *name;
  double bw_initial;
  tmgr_trace_t bw_trace;
  e_surf_network_link_state_t state_initial = SURF_NETWORK_LINK_ON;
  tmgr_trace_t state_trace;

  name = xbt_strdup(A_network_link_name);
  surf_parse_get_double(&bw_initial,A_network_link_bandwidth);
  surf_parse_get_trace(&bw_trace, A_network_link_bandwidth_file);

  xbt_assert0((A_network_link_state==A_network_link_state_ON)||
	      (A_network_link_state==A_network_link_state_OFF),
	      "Invalid state")
  if (A_network_link_state==A_network_link_state_ON) 
    state_initial = SURF_NETWORK_LINK_ON;
  if (A_network_link_state==A_network_link_state_OFF) 
    state_initial = SURF_NETWORK_LINK_OFF;
  surf_parse_get_trace(&state_trace,A_network_link_state_file);

  network_link_new(name, bw_initial, bw_trace, state_initial, state_trace);
}

static void route_new(int src_id, int dst_id, char **links, int nb_link,
		      double impact_on_src, double impact_on_dst,
		      double impact_on_src_with_other_recv,
		      double impact_on_dst_with_other_send)
{
  network_link_KCCFLN05_t *link_list = NULL;
  int i;
  route_KCCFLN05_t route = &(ROUTE(src_id,dst_id));

  route->size= nb_link;
  link_list = route->links = xbt_new0(network_link_KCCFLN05_t, nb_link);
  for (i = 0; i < nb_link; i++) {
    xbt_dict_get(network_link_set, links[i], (void *) &(link_list[i]));
    xbt_free(links[i]);
  }
  xbt_free(links);
  route->impact_on_src=impact_on_src;
  route->impact_on_dst=impact_on_src;
  route->impact_on_src_with_other_recv=impact_on_src_with_other_recv; 
  route->impact_on_dst_with_other_send=impact_on_dst_with_other_send;
}

static int nb_link = 0;
static char **link_name = NULL;
static int src_id = -1;
static int dst_id = -1;
static double impact_on_src;
static double impact_on_dst;
static double impact_on_src_with_other_recv; 
static double impact_on_dst_with_other_send;

static void parse_route_set_endpoints(void)
{
  src_id = ((workstation_KCCFLN05_t) name_service(A_route_src))->id;
  dst_id = ((workstation_KCCFLN05_t) name_service(A_route_dst))->id;
  surf_parse_get_double(&impact_on_src,A_route_impact_on_src);
  surf_parse_get_double(&impact_on_dst,A_route_impact_on_dst);
  surf_parse_get_double(&impact_on_src_with_other_recv,A_route_impact_on_src_with_other_recv);
  surf_parse_get_double(&impact_on_dst_with_other_send,A_route_impact_on_dst_with_other_send);

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
  route_new(src_id, dst_id, link_name, nb_link, impact_on_src, impact_on_dst,
	    impact_on_src_with_other_recv, impact_on_dst_with_other_send);
}

static void parse_file(const char *file)
{
  /* Figuring out the workstations */
  surf_parse_reset_parser();
  ETag_cpu_fun=parse_workstation;
  surf_parse_open(file);
  xbt_assert1((!surf_parse()),"Parse error in %s",file);
  surf_parse_close();

  create_routing_table();

  /* Figuring out the network links */
  surf_parse_reset_parser();
  ETag_network_link_fun=parse_network_link;
  surf_parse_open(file);
  xbt_assert1((!surf_parse()),"Parse error in %s",file);
  surf_parse_close();

  /* Building the routes */
  surf_parse_reset_parser();
  STag_route_fun=parse_route_set_endpoints;
  ETag_route_element_fun=parse_route_elem;
  ETag_route_fun=parse_route_set_route;
  surf_parse_open(file);
  xbt_assert1((!surf_parse()),"Parse error in %s",file);
  surf_parse_close();
}

static void update_network_KCCFLN05_state(void *id,
					  tmgr_trace_event_t event_type,
					  double value)
{
  network_link_KCCFLN05_t nw_link = id;

  if (event_type == nw_link->bw_event) {
    nw_link->bw_current = value;
    lmm_update_constraint_bound(maxmin_system_network_KCCFLN05, nw_link->constraint,
				nw_link->bw_current);
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

/**************************************/
/*************** actions **************/
/**************************************/
/*************** network **************/
static void action_network_KCCFLN05_free(surf_action_t action)
{
  int cpt;
  surf_action_t act = NULL;
  workstation_KCCFLN05_t src = ((surf_action_network_KCCFLN05_t) action)->src;
  workstation_KCCFLN05_t dst = ((surf_action_network_KCCFLN05_t) action)->dst;

  xbt_swag_remove(action, action->state_set);
  lmm_variable_free(maxmin_system_network_KCCFLN05, 
		    ((surf_action_network_KCCFLN05_t) action)->variable);

  xbt_dynar_foreach (src->outgoing_communications,cpt,act) {
    if(act==action) {
      xbt_dynar_remove_at(src->outgoing_communications, cpt, &act);
      break;
    }
  }

  xbt_dynar_foreach (dst->incomming_communications,cpt,act) {
    if(act==action) {
      xbt_dynar_remove_at(dst->incomming_communications, cpt, &act);
      break;
    }
  }

  xbt_free(action);
}

static double share_network_KCCFLN05_resources(double now)
{
 s_surf_action_network_KCCFLN05_t action;
 return generic_maxmin_share_resources2(surf_network_resource->common_public->
					states.running_action_set,
					xbt_swag_offset(action, variable),
					maxmin_system_network_KCCFLN05);
}

static void action_network_KCCFLN05_change_state(surf_action_t action,
						 e_surf_action_state_t state)
{
  surf_action_change_state(action, state);
  return;
}

static void update_actions_network_KCCFLN05_state(double now, double delta)
{
  surf_action_network_KCCFLN05_t action = NULL;
  surf_action_network_KCCFLN05_t next_action = NULL;
  xbt_swag_t running_actions =
      surf_network_resource->common_public->states.running_action_set;
  xbt_swag_t failed_actions =
      surf_network_resource->common_public->states.failed_action_set;

  xbt_swag_foreach_safe(action, next_action, running_actions) {
    surf_double_update(&(action->generic_action.remains),
		       lmm_variable_getvalue(action->variable) * delta);
    if (action->generic_action.max_duration != NO_MAX_DURATION)
          surf_double_update(&(action->generic_action.max_duration), delta);
    if ((action->generic_action.remains <= 0.0) && 
	(lmm_get_variable_weight(action->variable)>0)) {
      action->generic_action.finish = surf_get_clock();
      action_network_KCCFLN05_change_state((surf_action_t) action, SURF_ACTION_DONE);
    } else if ((action->generic_action.max_duration != NO_MAX_DURATION) &&
	       (action->generic_action.max_duration <= 0.0)) {
      action->generic_action.finish = surf_get_clock();
      action_network_KCCFLN05_change_state((surf_action_t) action, SURF_ACTION_DONE);
    } else {			/* Need to check that none of the resource has failed */
      /* FIXME : FAILURES ARE NOT HANDLED YET */
    }
  }

  xbt_swag_foreach_safe(action, next_action, failed_actions) {
    lmm_variable_disable(maxmin_system_network_KCCFLN05, action->variable);
  }

  return;
}

static surf_action_t communicate_KCCFLN05(void *src, void *dst, double size,
					  double rate)
{
  surf_action_network_KCCFLN05_t action = NULL;
  workstation_KCCFLN05_t card_src = src;
  workstation_KCCFLN05_t card_dst = dst;
  route_KCCFLN05_t route = &ROUTE(card_src->id, card_dst->id);
  int i;

  action = xbt_new0(s_surf_action_network_KCCFLN05_t, 1);

  action->generic_action.cost = size;
  action->generic_action.remains = size;
  action->generic_action.max_duration = NO_MAX_DURATION;
  action->generic_action.start = -1.0;
  action->generic_action.finish = -1.0;
  action->generic_action.resource_type =
      (surf_resource_t) surf_network_resource;

  action->generic_action.state_set =
      surf_network_resource->common_public->states.running_action_set;

  xbt_swag_insert(action, action->generic_action.state_set);

  if(rate>0)
    action->variable = lmm_variable_new(maxmin_system_network_KCCFLN05, action, 1.0, rate,
					route->size+1);
  else
    action->variable = lmm_variable_new(maxmin_system_network_KCCFLN05, action, 1.0, -1.0,
					route->size+1);

  for (i = 0; i < route->size; i++)
    lmm_expand(maxmin_system_network_KCCFLN05, route->links[i]->constraint, 
	       action->variable, 1.0);

  if(card_src->bus) 
    lmm_expand(maxmin_system_network_KCCFLN05, card_src->bus, 
	       action->variable, 1.0);

  action->src=src;
  action->dst=dst;

  xbt_dynar_push(card_src->outgoing_communications,&action);
  xbt_dynar_push(card_dst->incomming_communications,&action);

  return (surf_action_t) action;
}

static void network_KCCFLN05_action_suspend(surf_action_t action)
{
  lmm_update_variable_weight(maxmin_system_network_KCCFLN05,
			     ((surf_action_network_KCCFLN05_t) action)->variable, 0.0);
}

static void network_KCCFLN05_action_resume(surf_action_t action)
{
  lmm_update_variable_weight(maxmin_system_network_KCCFLN05,
			     ((surf_action_network_KCCFLN05_t) action)->variable, 1.0);
}

static int network_KCCFLN05_action_is_suspended(surf_action_t action)
{
  return (lmm_get_variable_weight(((surf_action_network_KCCFLN05_t) action)->variable) == 0.0);
}

/***************** CPU ****************/
static void action_cpu_KCCFLN05_free(surf_action_t action)
{
  xbt_swag_remove(action, action->state_set);
  lmm_variable_free(maxmin_system_cpu_KCCFLN05, ((surf_action_cpu_KCCFLN05_t)action)->variable);
  xbt_free(action);
}

/* #define WARNING(format, ...) (fprintf(stderr, "[%s , %s : %d] ", __FILE__, __FUNCTION__, __LINE__),\ */
/*                               fprintf(stderr, format, ## __VA_ARGS__), \ */
/*                               fprintf(stderr, "\n")) */
/* #define VOIRP(expr) WARNING("  {" #expr " = %p }", expr) */
/* #define VOIRD(expr) WARNING("  {" #expr " = %d }", expr) */
/* #define VOIRG(expr) WARNING("  {" #expr " = %lg }", expr) */

static double share_cpu_KCCFLN05_resources(double now)
{
  s_surf_action_cpu_KCCFLN05_t s_cpu_action;
  lmm_constraint_t cnst = NULL;
  workstation_KCCFLN05_t workstation = NULL;
  double W=0.0;
  double scale=0.0;
  int cpt;
  surf_action_network_KCCFLN05_t action;

  for(cnst = lmm_get_first_active_constraint(maxmin_system_cpu_KCCFLN05);
      cnst;
      cnst= lmm_get_next_active_constraint(maxmin_system_cpu_KCCFLN05, cnst))
    {
      workstation = lmm_constraint_id(cnst);
      W=workstation->power_current * workstation->power_scale;
      
      if((!xbt_dynar_length(workstation->incomming_communications)) &&
	 (!xbt_dynar_length(workstation->outgoing_communications))) {
	scale = 1.0;
      } else if((!xbt_dynar_length(workstation->incomming_communications)) &&
		(xbt_dynar_length(workstation->outgoing_communications))) {
	scale = workstation->interference_send;
	xbt_dynar_foreach (workstation->outgoing_communications,cpt,action) {
/* 	  VOIRD(action->src->id); */
/* 	  VOIRD(action->dst->id); */
/* 	  VOIRP(&ROUTE(action->src->id,action->dst->id)); */
/* 	  VOIRG(ROUTE(action->src->id,action->dst->id).impact_on_src); */
	  scale -= ROUTE(action->src->id,action->dst->id).impact_on_src *
	    lmm_variable_getvalue(action->variable);
	}
	if(scale<0.0) scale=0.0;
	xbt_assert0(scale>=0.0,"Negative interference !");
      } else if((xbt_dynar_length(workstation->incomming_communications)) &&
		(!xbt_dynar_length(workstation->outgoing_communications))) {
	scale = workstation->interference_recv;
	xbt_dynar_foreach (workstation->incomming_communications,cpt,action) {
	  scale -= ROUTE(action->src->id,action->dst->id).impact_on_dst *
	    lmm_variable_getvalue(action->variable);
	}
	if(scale<0.0) scale=0.0;
	xbt_assert0(scale>=0.0,"Negative interference !");
      } else {
	scale = workstation->interference_send_recv;
	xbt_dynar_foreach (workstation->outgoing_communications,cpt,action) {
	  scale -= ROUTE(action->src->id,action->dst->id).impact_on_src_with_other_recv *
	    lmm_variable_getvalue(action->variable);
	}
	xbt_dynar_foreach (workstation->incomming_communications,cpt,action) {
	  scale -= ROUTE(action->src->id,action->dst->id).impact_on_dst_with_other_send *
	    lmm_variable_getvalue(action->variable);
	}
	if(scale<0.0) scale=0.0;
	xbt_assert0(scale>=0.0,"Negative interference !");
      }
      lmm_update_constraint_bound(maxmin_system_cpu_KCCFLN05,workstation->constraint,
				  W*scale);
    }

  return generic_maxmin_share_resources2(surf_cpu_resource->common_public->
					 states.running_action_set,
					 xbt_swag_offset(s_cpu_action, variable),
					 maxmin_system_cpu_KCCFLN05);
}

static void update_actions_cpu_KCCFLN05_state(double now, double delta)
{
  surf_action_cpu_KCCFLN05_t action = NULL;
  surf_action_cpu_KCCFLN05_t next_action = NULL;
  xbt_swag_t running_actions =
      surf_cpu_resource->common_public->states.running_action_set;
  xbt_swag_t failed_actions =
      surf_cpu_resource->common_public->states.failed_action_set;

  xbt_swag_foreach_safe(action, next_action, running_actions) {
    surf_double_update(&(action->generic_action.remains),
	lmm_variable_getvalue(action->variable) * delta);
    if (action->generic_action.max_duration != NO_MAX_DURATION)
      action->generic_action.max_duration -= delta;
    if ((action->generic_action.remains <= 0) && 
	(lmm_get_variable_weight(action->variable)>0)) {
      action->generic_action.finish = surf_get_clock();
      surf_action_change_state((surf_action_t) action, SURF_ACTION_DONE);
    } else if ((action->generic_action.max_duration != NO_MAX_DURATION) &&
	       (action->generic_action.max_duration <= 0)) {
      action->generic_action.finish = surf_get_clock();
      surf_action_change_state((surf_action_t) action, SURF_ACTION_DONE);
    } else {			/* Need to check that none of the resource has failed */
      lmm_constraint_t cnst = NULL;
      int i = 0;
      workstation_KCCFLN05_t cpu = NULL;

      while ((cnst =
	      lmm_get_cnst_from_var(maxmin_system_cpu_KCCFLN05, action->variable,
				    i++))) {
	cpu = lmm_constraint_id(cnst);
	if (cpu->state_current == SURF_CPU_OFF) {
	  action->generic_action.finish = surf_get_clock();
	  surf_action_change_state((surf_action_t) action, SURF_ACTION_FAILED);
	  break;
	}
      }
    }
  }

  xbt_swag_foreach_safe(action, next_action, failed_actions) {
    lmm_variable_disable(maxmin_system_cpu_KCCFLN05, action->variable);
  }

  return;
}

static surf_action_t execute_KCCFLN05(void *cpu, double size)
{
  surf_action_cpu_KCCFLN05_t action = NULL;
  workstation_KCCFLN05_t CPU = cpu;

  action = xbt_new0(s_surf_action_cpu_KCCFLN05_t, 1);

  action->generic_action.cost = size;
  action->generic_action.remains = size;
  action->generic_action.max_duration = NO_MAX_DURATION;
  action->generic_action.start = surf_get_clock();
  action->generic_action.finish = -1.0;
  action->generic_action.resource_type =
      (surf_resource_t) surf_cpu_resource;

  if (CPU->state_current == SURF_CPU_ON)
    action->generic_action.state_set =
	surf_cpu_resource->common_public->states.running_action_set;
  else
    action->generic_action.state_set =
	surf_cpu_resource->common_public->states.failed_action_set;
  xbt_swag_insert(action, action->generic_action.state_set);

  action->variable = lmm_variable_new(maxmin_system_cpu_KCCFLN05, action, 1.0, -1.0, 1);
  lmm_expand(maxmin_system_cpu_KCCFLN05, CPU->constraint, action->variable,
	     1.0);

  return (surf_action_t) action;
}

static void cpu_KCCFLN05_action_suspend(surf_action_t action)
{
  lmm_update_variable_weight(maxmin_system_cpu_KCCFLN05,
			     ((surf_action_cpu_KCCFLN05_t) action)->variable, 0.0);
}

static void cpu_KCCFLN05_action_resume(surf_action_t action)
{
  lmm_update_variable_weight(maxmin_system_cpu_KCCFLN05,
			     ((surf_action_cpu_KCCFLN05_t) action)->variable, 1.0);
}

static int cpu_KCCFLN05_action_is_suspended(surf_action_t action)
{
  return (lmm_get_variable_weight(((surf_action_cpu_KCCFLN05_t) action)->variable) == 0.0);
}

/************* workstation ************/
static void action_change_state(surf_action_t action,
				e_surf_action_state_t state)
{
  xbt_assert0(0, "Workstation is a virtual resource. I should not be there!");
  surf_action_change_state(action, state);
  return;
}

static void action_free(surf_action_t action)
{
  xbt_assert0(0, "Workstation is a virtual resource. I should not be there!");
  return;
}

static int resource_used(void *resource_id)
{
  xbt_assert0(0, "Workstation is a virtual resource. I should not be there!");
  return 0;
}

static double share_resources(double now)
{
  return -1.0;
}

static void update_actions_state(double now, double delta)
{
  return;
}

static void update_resource_state(void *id,
				  tmgr_trace_event_t event_type,
				  double value)
{
  xbt_assert0(0, "Workstation is a virtual resource. I should not be there!");
  return;
}

static void action_suspend(surf_action_t action)
{
  if(action->resource_type==(surf_resource_t)surf_network_resource) 
    surf_network_resource->common_public->suspend(action);
  else if(action->resource_type==(surf_resource_t)surf_cpu_resource) 
    surf_cpu_resource->common_public->suspend(action);
  else DIE_IMPOSSIBLE;
}

static void action_resume(surf_action_t action)
{
  if(action->resource_type==(surf_resource_t)surf_network_resource)
    surf_network_resource->common_public->resume(action);
  else if(action->resource_type==(surf_resource_t)surf_cpu_resource)
    surf_cpu_resource->common_public->resume(action);
  else DIE_IMPOSSIBLE;
}

static int action_is_suspended(surf_action_t action)
{
  if(action->resource_type==(surf_resource_t)surf_network_resource) 
    return surf_network_resource->common_public->is_suspended(action);
  if(action->resource_type==(surf_resource_t)surf_cpu_resource) 
    return surf_cpu_resource->common_public->is_suspended(action);
  DIE_IMPOSSIBLE;
}

/**************************************/
/********* Module  creation ***********/
/**************************************/
static void cpu_KCCFLN05_finalize(void)
{
  xbt_dict_free(&workstation_set);
  xbt_swag_free(surf_cpu_resource->common_public->states.ready_action_set);
  xbt_swag_free(surf_cpu_resource->common_public->states.
		running_action_set);
  xbt_swag_free(surf_cpu_resource->common_public->states.
		failed_action_set);
  xbt_swag_free(surf_cpu_resource->common_public->states.done_action_set);
  xbt_free(surf_cpu_resource->common_public);
  xbt_free(surf_cpu_resource->common_private);
  xbt_free(surf_cpu_resource->extension_public);

  xbt_free(surf_cpu_resource);
  surf_cpu_resource = NULL;

  if (maxmin_system_cpu_KCCFLN05) {
    lmm_system_free(maxmin_system_cpu_KCCFLN05);
    maxmin_system_cpu_KCCFLN05 = NULL;
  }
}

static void cpu_KCCFLN05_resource_init_internal(void)
{
  s_surf_action_t action;

  surf_cpu_resource = xbt_new0(s_surf_cpu_resource_t, 1);

  surf_cpu_resource->common_private =
      xbt_new0(s_surf_resource_private_t, 1);
  surf_cpu_resource->common_public = 
    xbt_new0(s_surf_resource_public_t, 1);

  surf_cpu_resource->extension_public =
      xbt_new0(s_surf_cpu_resource_extension_public_t, 1);

  surf_cpu_resource->common_public->states.ready_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_cpu_resource->common_public->states.running_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_cpu_resource->common_public->states.failed_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_cpu_resource->common_public->states.done_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));

  surf_cpu_resource->common_public->name_service = name_service;
  surf_cpu_resource->common_public->get_resource_name = get_resource_name; 
  surf_cpu_resource->common_public->action_get_state =
      surf_action_get_state;
  surf_cpu_resource->common_public->action_free = action_cpu_KCCFLN05_free;
  surf_cpu_resource->common_public->action_cancel = NULL;
  surf_cpu_resource->common_public->action_recycle = NULL;
  surf_cpu_resource->common_public->action_change_state = surf_action_change_state;
  surf_cpu_resource->common_public->action_set_data = surf_action_set_data;
  surf_cpu_resource->common_public->name = "CPU KCCFLN05";

  surf_cpu_resource->common_private->resource_used = cpu_used;
  surf_cpu_resource->common_private->share_resources = share_cpu_KCCFLN05_resources;
  surf_cpu_resource->common_private->update_actions_state =
    update_actions_cpu_KCCFLN05_state;
  surf_cpu_resource->common_private->update_resource_state =
      update_cpu_KCCFLN05_state;
  surf_cpu_resource->common_private->finalize = cpu_KCCFLN05_finalize;

  surf_cpu_resource->extension_public->execute = execute_KCCFLN05;
/*FIXME*//*   surf_cpu_resource->extension_public->sleep = action_sleep; */

  surf_cpu_resource->common_public->suspend = cpu_KCCFLN05_action_suspend;
  surf_cpu_resource->common_public->resume = cpu_KCCFLN05_action_resume;
  surf_cpu_resource->common_public->is_suspended = cpu_KCCFLN05_action_is_suspended;

  surf_cpu_resource->extension_public->get_state = get_state;

  workstation_set = xbt_dict_new();

  maxmin_system_cpu_KCCFLN05 = lmm_system_new();
}

static void network_KCCFLN05_finalize(void)
{
  int i,j;

  xbt_dict_free(&network_link_set);
  xbt_swag_free(surf_network_resource->common_public->states.
		ready_action_set);
  xbt_swag_free(surf_network_resource->common_public->states.
		running_action_set);
  xbt_swag_free(surf_network_resource->common_public->states.
		failed_action_set);
  xbt_swag_free(surf_network_resource->common_public->states.
		done_action_set);
  xbt_free(surf_network_resource->common_public);
  xbt_free(surf_network_resource->common_private);
  xbt_free(surf_network_resource->extension_public);

  xbt_free(surf_network_resource);
  surf_network_resource = NULL;

  for (i = 0; i < nb_workstation; i++) 
    for (j = 0; j < nb_workstation; j++) 
      xbt_free(ROUTE(i,j).links);
  xbt_free(routing_table);
  routing_table = NULL;
  nb_workstation = 0;

  if (maxmin_system_network_KCCFLN05) {
    lmm_system_free(maxmin_system_network_KCCFLN05);
    maxmin_system_network_KCCFLN05 = NULL;
  }
}

static void network_KCCFLN05_resource_init_internal(void)
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
  surf_network_resource->common_public->get_resource_name = get_resource_name;
  surf_network_resource->common_public->action_get_state =
      surf_action_get_state;
  surf_network_resource->common_public->action_free = action_network_KCCFLN05_free;
  surf_network_resource->common_public->action_cancel = NULL;
  surf_network_resource->common_public->action_recycle = NULL;
  surf_network_resource->common_public->action_change_state = action_network_KCCFLN05_change_state;
  surf_network_resource->common_public->action_set_data = surf_action_set_data;
  surf_network_resource->common_public->name = "network KCCFLN05";

  surf_network_resource->common_private->resource_used = link_used;
  surf_network_resource->common_private->share_resources = share_network_KCCFLN05_resources;
  surf_network_resource->common_private->update_actions_state =
    update_actions_network_KCCFLN05_state;
  surf_network_resource->common_private->update_resource_state =
    update_network_KCCFLN05_state;
  surf_network_resource->common_private->finalize = network_KCCFLN05_finalize;

  surf_network_resource->common_public->suspend = network_KCCFLN05_action_suspend;
  surf_network_resource->common_public->resume = network_KCCFLN05_action_resume;
  surf_network_resource->common_public->is_suspended = network_KCCFLN05_action_is_suspended;

  surf_network_resource->extension_public->communicate = communicate_KCCFLN05;

  network_link_set = xbt_dict_new();


  maxmin_system_network_KCCFLN05 = lmm_system_new();
}

static void workstation_KCCFLN05_finalize(void)
{
  xbt_swag_free(surf_workstation_resource->common_public->states.ready_action_set);
  xbt_swag_free(surf_workstation_resource->common_public->states.running_action_set);
  xbt_swag_free(surf_workstation_resource->common_public->states.failed_action_set);
  xbt_swag_free(surf_workstation_resource->common_public->states.done_action_set);

  xbt_free(surf_workstation_resource->common_public);
  xbt_free(surf_workstation_resource->common_private);
  xbt_free(surf_workstation_resource->extension_public);

  xbt_free(surf_workstation_resource);
  surf_workstation_resource = NULL;
}

static void workstation_KCCFLN05_resource_init_internal(void)
{
  s_surf_action_t action;

  surf_workstation_resource = xbt_new0(s_surf_workstation_resource_t, 1);

  surf_workstation_resource->common_private =
      xbt_new0(s_surf_resource_private_t, 1);
  surf_workstation_resource->common_public =
      xbt_new0(s_surf_resource_public_t, 1);
  surf_workstation_resource->extension_public =
      xbt_new0(s_surf_workstation_resource_extension_public_t, 1);

  surf_workstation_resource->common_public->states.ready_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_workstation_resource->common_public->states.running_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_workstation_resource->common_public->states.failed_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_workstation_resource->common_public->states.done_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));

  surf_workstation_resource->common_public->name_service = name_service;
  surf_workstation_resource->common_public->get_resource_name =
      get_resource_name;
  surf_workstation_resource->common_public->action_get_state =
      surf_action_get_state;
  surf_workstation_resource->common_public->action_free = action_free;
  surf_workstation_resource->common_public->action_cancel = NULL;
  surf_workstation_resource->common_public->action_recycle = NULL;
  surf_workstation_resource->common_public->action_change_state = action_change_state;
  surf_workstation_resource->common_public->action_set_data = surf_action_set_data;
  surf_workstation_resource->common_public->name = "Workstation KCCFLN05";

  surf_workstation_resource->common_private->resource_used = resource_used;
  surf_workstation_resource->common_private->share_resources = share_resources;
  surf_workstation_resource->common_private->update_actions_state = update_actions_state;
  surf_workstation_resource->common_private->update_resource_state = update_resource_state;
  surf_workstation_resource->common_private->finalize = workstation_KCCFLN05_finalize;


  surf_workstation_resource->common_public->suspend = action_suspend;
  surf_workstation_resource->common_public->resume = action_resume;
  surf_workstation_resource->common_public->is_suspended = action_is_suspended;

  surf_workstation_resource->extension_public->execute = execute_KCCFLN05;
/*FIXME*//*  surf_workstation_resource->extension_public->sleep = action_sleep; */
  surf_workstation_resource->extension_public->get_state = get_state;
  surf_workstation_resource->extension_public->communicate = communicate_KCCFLN05;
}

/**************************************/
/*************** Generic **************/
/**************************************/

void surf_workstation_resource_init_KCCFLN05(const char *filename)
{
  xbt_assert0(!surf_cpu_resource,"CPU resource type already defined");
  cpu_KCCFLN05_resource_init_internal();
  xbt_assert0(!surf_network_resource,"network resource type already defined");
  network_KCCFLN05_resource_init_internal();
  workstation_KCCFLN05_resource_init_internal();
  parse_file(filename);

  xbt_dynar_push(resource_list, &surf_network_resource);
  xbt_dynar_push(resource_list, &surf_cpu_resource);
  xbt_dynar_push(resource_list, &surf_workstation_resource);

  if (maxmin_system) { /* To avoid using this useless system */
    lmm_system_free(maxmin_system);
    maxmin_system = NULL;
  }
}
