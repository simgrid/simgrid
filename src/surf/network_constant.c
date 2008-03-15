/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_private.h"
#include "xbt/dict.h"
#include "xbt/str.h"
#include "xbt/log.h"

#define CONSTANT_VALUE 1.0
typedef struct network_card_Constant {
  char *name;
  int id;
} s_network_card_Constant_t, *network_card_Constant_t;

typedef struct network_link_Constant {
  surf_model_t model;	/* Any such object, added in a trace
				   should start by this field!!! */
  xbt_dict_t properties;
  /* Using this object with the public part of
     model does not make sense */
  char *name;
  double bw_current;
  tmgr_trace_event_t bw_event;
  double lat_current;
  tmgr_trace_event_t lat_event;
  e_surf_link_state_t state_current;
  tmgr_trace_event_t state_event;
  lmm_constraint_t constraint;
} s_link_Constant_t, *link_Constant_t;

typedef struct surf_action_network_Constant {
  s_surf_action_t generic_action;
  double latency;
  double lat_current;
  lmm_variable_t variable;
  double rate;
  int suspended;
  network_card_Constant_t src;
  network_card_Constant_t dst;
} s_surf_action_network_Constant_t, *surf_action_network_Constant_t;

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_network);

static lmm_system_t network_maxmin_system = NULL;
static void (*network_solve) (lmm_system_t) = NULL;

static xbt_dict_t network_card_set = NULL;

static int card_number = 0;
static int host_number = 0;
static link_Constant_t **routing_table = NULL;
static int *routing_table_size = NULL;
static link_Constant_t loopback = NULL;

#define ROUTE(i,j) routing_table[(i)+(j)*card_number]
#define ROUTE_SIZE(i,j) routing_table_size[(i)+(j)*card_number]

static void create_routing_table(void)
{
  routing_table =
      xbt_new0(link_Constant_t *, /*card_number * card_number */ host_number * host_number);
  routing_table_size = xbt_new0(int, /*card_number * card_number*/ host_number * host_number);
}

static void link_free(void *nw_link)
{
  free(((link_Constant_t) nw_link)->name);
  free(nw_link);
}

static link_Constant_t link_new(char *name,
			    double bw_initial,
			    tmgr_trace_t bw_trace,
			    double lat_initial,
			    tmgr_trace_t lat_trace,
			    e_surf_link_state_t
			    state_initial,
			    tmgr_trace_t state_trace,
			    e_surf_link_sharing_policy_t
			    policy, xbt_dict_t properties)
{
  link_Constant_t nw_link = xbt_new0(s_link_Constant_t, 1);
  xbt_assert1(!xbt_dict_get_or_null(link_set, name), 
	      "Link '%s' declared several times in the platform file.", name);   

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

  nw_link->constraint =
      lmm_constraint_new(network_maxmin_system, nw_link,
			 nw_link->bw_current);

  if (policy == SURF_LINK_FATPIPE)
    lmm_constraint_shared(nw_link->constraint);

  nw_link->properties = properties;

  current_property_set = properties;

  xbt_dict_set(link_set, name, nw_link, link_free);

  return nw_link;
}

static void network_card_free(void *nw_card)
{
  free(((network_card_Constant_t) nw_card)->name);
  free(nw_card);
}

static int network_card_new(const char *card_name)
{
  network_card_Constant_t card =
      xbt_dict_get_or_null(network_card_set, card_name);

  if (!card) {
    card = xbt_new0(s_network_card_Constant_t, 1);
    card->name = xbt_strdup(card_name);
    card->id = card_number++;
    xbt_dict_set(network_card_set, card_name, card, network_card_free);
  }
  return card->id;
}

static void route_new(int src_id, int dst_id,
		      link_Constant_t * link_list, int nb_link)
{
  ROUTE_SIZE(src_id, dst_id) = nb_link;
  ROUTE(src_id, dst_id) = link_list =
      xbt_realloc(link_list, sizeof(link_Constant_t) * nb_link);
}

static void parse_link_init(void)
{
  char *name_link;
  double bw_initial;
  tmgr_trace_t bw_trace;
  double lat_initial;
  tmgr_trace_t lat_trace;
  e_surf_link_state_t state_initial_link = SURF_LINK_ON;
  e_surf_link_sharing_policy_t policy_initial_link = SURF_LINK_SHARED;
  tmgr_trace_t state_trace;

  name_link = xbt_strdup(A_surfxml_link_id);
  surf_parse_get_double(&bw_initial, A_surfxml_link_bandwidth);
  surf_parse_get_trace(&bw_trace, A_surfxml_link_bandwidth_file);
  surf_parse_get_double(&lat_initial, A_surfxml_link_latency);
  surf_parse_get_trace(&lat_trace, A_surfxml_link_latency_file);

  xbt_assert0((A_surfxml_link_state ==
	       A_surfxml_link_state_ON)
	      || (A_surfxml_link_state ==
		  A_surfxml_link_state_OFF), "Invalid state");
  if (A_surfxml_link_state == A_surfxml_link_state_ON)
    state_initial_link = SURF_LINK_ON;
  else if (A_surfxml_link_state ==
	   A_surfxml_link_state_OFF)
    state_initial_link = SURF_LINK_OFF;

  if (A_surfxml_link_sharing_policy ==
      A_surfxml_link_sharing_policy_SHARED)
    policy_initial_link = SURF_LINK_SHARED;
  else if (A_surfxml_link_sharing_policy ==
	   A_surfxml_link_sharing_policy_FATPIPE)
    policy_initial_link = SURF_LINK_FATPIPE;

  surf_parse_get_trace(&state_trace, A_surfxml_link_state_file);

  link_new(name_link, bw_initial, bw_trace,
		   lat_initial, lat_trace, state_initial_link, state_trace,
		   policy_initial_link, xbt_dict_new());

}

static int src_id = -1;
static int dst_id = -1;

static void parse_route_set_endpoints(void)
{
  src_id = network_card_new(A_surfxml_route_src);
  dst_id = network_card_new(A_surfxml_route_dst);
  route_action = A_surfxml_route_action;
  route_link_list = xbt_dynar_new(sizeof(char *), &free_string);
}

static void parse_route_set_route(void)
{
  char *name;
  if (src_id != -1 && dst_id != -1) {
    name = bprintf("%x#%x",src_id, dst_id);
    manage_route(route_table, name, route_action, 0);
    free(name);    
  }
}

static void add_loopback(void)
{
  int i;
  /* Adding loopback if needed */
  for (i = 0; i < host_number; i++)
    if (!ROUTE_SIZE(i, i)) {
      if (!loopback)
	loopback = link_new(xbt_strdup("__MSG_loopback__"),
				    498000000, NULL, 0.000015, NULL,
				    SURF_LINK_ON, NULL,
				    SURF_LINK_FATPIPE,NULL);
      ROUTE_SIZE(i, i) = 1;
      ROUTE(i, i) = xbt_new0(link_Constant_t, 1);
      ROUTE(i, i)[0] = loopback;
    }
}

static void add_route(void)
{
  xbt_ex_t e;
  int nb_link = 0;
  unsigned int cpt = 0;    
  int link_list_capacity = 0;
  link_Constant_t *link_list = NULL;
  xbt_dict_cursor_t cursor = NULL;
  char *key,*data, *end;
  const char *sep = "#";
  xbt_dynar_t links, keys;

  if (routing_table == NULL) create_routing_table();

  xbt_dict_foreach(route_table, cursor, key, data) {
	  char* link = NULL;
    nb_link = 0;
    links = (xbt_dynar_t)data;
    keys = xbt_str_split_str(key, sep);

    link_list_capacity = xbt_dynar_length(links);
    link_list = xbt_new(link_Constant_t, link_list_capacity);

    src_id = strtol(xbt_dynar_get_as(keys, 0, char*), &end, 16);
    dst_id = strtol(xbt_dynar_get_as(keys, 1, char*), &end, 16);
 
    xbt_dynar_foreach (links, cpt, link) {
      TRY {
	link_list[nb_link++] = xbt_dict_get(link_set, link);
      }
      CATCH(e) {
        RETHROW1("Link %s not found (dict raised this exception: %s)", link);
      }     
    }
    route_new(src_id, dst_id, link_list, nb_link);
   }

   xbt_dict_free(&route_table);

}

static void count_hosts(void)
{
   host_number++;
}


static void add_traces(void) {
   xbt_dict_cursor_t cursor=NULL;
   char *trace_name,*elm;
   
   static int called = 0;
   if (called) return;
   called = 1;

   /* connect all traces relative to network */
   xbt_dict_foreach(trace_connect_list_link_avail, cursor, trace_name, elm) {
      tmgr_trace_t trace = xbt_dict_get_or_null(traces_set_list, trace_name);
      link_Constant_t link = xbt_dict_get_or_null(link_set, elm);
      
      xbt_assert1(link, "Link %s undefined", elm);
      xbt_assert1(trace, "Trace %s undefined", trace_name);
      
      link->state_event = tmgr_history_add_trace(history, trace, 0.0, 0, link);
   }

   xbt_dict_foreach(trace_connect_list_bandwidth, cursor, trace_name, elm) {
      tmgr_trace_t trace = xbt_dict_get_or_null(traces_set_list, trace_name);
      link_Constant_t link = xbt_dict_get_or_null(link_set, elm);
      
      xbt_assert1(link, "Link %s undefined", elm);
      xbt_assert1(trace, "Trace %s undefined", trace_name);
      
      link->bw_event = tmgr_history_add_trace(history, trace, 0.0, 0, link);
   }
   
   xbt_dict_foreach(trace_connect_list_latency, cursor, trace_name, elm) {
      tmgr_trace_t trace = xbt_dict_get_or_null(traces_set_list, trace_name);
      link_Constant_t link = xbt_dict_get_or_null(link_set, elm);
      
      xbt_assert1(link, "Link %s undefined", elm);
      xbt_assert1(trace, "Trace %s undefined", trace_name);
      
      link->lat_event = tmgr_history_add_trace(history, trace, 0.0, 0, link);
   }

   xbt_dict_free(&trace_connect_list_host_avail);
   xbt_dict_free(&trace_connect_list_power);
   xbt_dict_free(&trace_connect_list_link_avail);
   xbt_dict_free(&trace_connect_list_bandwidth);
   xbt_dict_free(&trace_connect_list_latency);
   
   xbt_dict_free(&traces_set_list); 
}

static void define_callbacks(const char *file)
{
  /* Figuring out the network links */
  surfxml_add_callback(STag_surfxml_host_cb_list, &count_hosts);
  surfxml_add_callback(STag_surfxml_link_cb_list, &parse_link_init);
  surfxml_add_callback(STag_surfxml_prop_cb_list, &parse_properties);
  surfxml_add_callback(STag_surfxml_route_cb_list, &parse_route_set_endpoints);
  surfxml_add_callback(ETag_surfxml_link_c_ctn_cb_list, &parse_route_elem);
  surfxml_add_callback(ETag_surfxml_route_cb_list, &parse_route_set_route);
  surfxml_add_callback(STag_surfxml_platform_cb_list, &init_data);
  surfxml_add_callback(ETag_surfxml_platform_cb_list, &add_traces);
  surfxml_add_callback(ETag_surfxml_platform_cb_list, &add_route);
  surfxml_add_callback(ETag_surfxml_platform_cb_list, &add_loopback);
  surfxml_add_callback(STag_surfxml_set_cb_list, &parse_sets);
  surfxml_add_callback(STag_surfxml_route_c_multi_cb_list, &parse_route_multi_set_endpoints);
  surfxml_add_callback(ETag_surfxml_route_c_multi_cb_list, &parse_route_multi_set_route);
  surfxml_add_callback(STag_surfxml_foreach_cb_list, &parse_foreach);
  surfxml_add_callback(STag_surfxml_cluster_cb_list, &parse_cluster);
  surfxml_add_callback(STag_surfxml_trace_cb_list, &parse_trace_init);
  surfxml_add_callback(ETag_surfxml_trace_cb_list, &parse_trace_finalize);
  surfxml_add_callback(STag_surfxml_trace_c_connect_cb_list, &parse_trace_c_connect);
}

static void *name_service(const char *name)
{
  network_card_Constant_t card = xbt_dict_get_or_null(network_card_set, name);
  return card;
}

static const char *get_resource_name(void *resource_id)
{
  return ((network_card_Constant_t) resource_id)->name;
}

static int resource_used(void *resource_id)
{
  return lmm_constraint_used(network_maxmin_system,
			     ((link_Constant_t) resource_id)->
			     constraint);
}

static int action_free(surf_action_t action)
{
  action->using--;
  if (!action->using) {
    xbt_swag_remove(action, action->state_set);
    if (((surf_action_network_Constant_t) action)->variable)
      lmm_variable_free(network_maxmin_system,
			((surf_action_network_Constant_t) action)->variable);
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
/*     if(((surf_action_network_Constant_t)action)->variable) { */
/*       lmm_variable_disable(network_maxmin_system, ((surf_action_network_Constant_t)action)->variable); */
/*       ((surf_action_network_Constant_t)action)->variable = NULL; */
/*     } */

  surf_action_change_state(action, state);
  return;
}

static double share_resources(double now)
{
  s_surf_action_network_Constant_t s_action;
  surf_action_network_Constant_t action = NULL;
  xbt_swag_t running_actions =
      surf_network_model->common_public->states.running_action_set;
  double min;

  min = generic_maxmin_share_resources(running_actions,
				       xbt_swag_offset(s_action,
						       variable),
				       network_maxmin_system,
				       network_solve);

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
  double deltap = 0.0;
  surf_action_network_Constant_t action = NULL;
  surf_action_network_Constant_t next_action = NULL;
  xbt_swag_t running_actions =
      surf_network_model->common_public->states.running_action_set;

  xbt_swag_foreach_safe(action, next_action, running_actions) {
    deltap = delta;
    if (action->latency > 0) {
      if (action->latency > deltap) {
	double_update(&(action->latency), deltap);
	deltap = 0.0;
      } else {
	double_update(&(deltap), action->latency);
	action->latency = 0.0;
      }
    }
    double_update(&(action->generic_action.remains),
		  action->generic_action.cost * deltap/ CONSTANT_VALUE);
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
				  double value)
{
  link_Constant_t nw_link = id;
  /*   printf("[" "%lg" "] Asking to update network card \"%s\" with value " */
  /*     "%lg" " for event %p\n", surf_get_clock(), nw_link->name, */
  /*     value, event_type); */

  if (event_type == nw_link->bw_event) {
    nw_link->bw_current = value;
    lmm_update_constraint_bound(network_maxmin_system, nw_link->constraint,
				nw_link->bw_current);
  } else if (event_type == nw_link->lat_event) {
    double delta = value - nw_link->lat_current;
    lmm_variable_t var = NULL;
    surf_action_network_Constant_t action = NULL;

    nw_link->lat_current = value;
    while (lmm_get_var_from_cnst
	   (network_maxmin_system, nw_link->constraint, &var)) {
      action = lmm_variable_id(var);
      action->lat_current += delta;
      if (action->rate < 0)
	lmm_update_variable_bound(network_maxmin_system, action->variable,
				  SG_TCP_CTE_GAMMA / (2.0 *
						      action->
						      lat_current));
      else
	lmm_update_variable_bound(network_maxmin_system, action->variable,
				  min(action->rate,
				      SG_TCP_CTE_GAMMA / (2.0 *
							  action->
							  lat_current)));
      if (!(action->suspended))
	lmm_update_variable_weight(network_maxmin_system, action->variable,
				   action->lat_current);
      lmm_update_variable_latency(network_maxmin_system, action->variable,
				  delta);
    }
  } else if (event_type == nw_link->state_event) {
    if (value > 0)
      nw_link->state_current = SURF_LINK_ON;
    else
      nw_link->state_current = SURF_LINK_OFF;
  } else {
    CRITICAL0("Unknown event ! \n");
    xbt_abort();
  }

  return;
}

static surf_action_t communicate(void *src, void *dst, double size,
				 double rate)
{
  surf_action_network_Constant_t action = NULL;
  network_card_Constant_t card_src = src;
  network_card_Constant_t card_dst = dst;

  XBT_IN4("(%s,%s,%g,%g)", card_src->name, card_dst->name, size, rate);

  action = xbt_new0(s_surf_action_network_Constant_t, 1);

  action->generic_action.using = 1;
  action->generic_action.cost = size;
  action->generic_action.remains = size;
  action->generic_action.max_duration = NO_MAX_DURATION;
  action->generic_action.start = surf_get_clock();
  action->generic_action.finish = -1.0;
  action->generic_action.model_type =
      (surf_model_t) surf_network_model;
  action->suspended = 0;
  action->generic_action.state_set =
      surf_network_model->common_public->states.running_action_set;

  xbt_swag_insert(action, action->generic_action.state_set);
  action->rate = rate;

  action->latency = CONSTANT_VALUE;
  action->lat_current = action->latency;

  XBT_OUT;

  return (surf_action_t) action;
}

/* returns an array of link_Constant_t */
static const void **get_route(void *src, void *dst)
{
  network_card_Constant_t card_src = src;
  network_card_Constant_t card_dst = dst;
  return (const void **) ROUTE(card_src->id, card_dst->id);
}

static int get_route_size(void *src, void *dst)
{
  network_card_Constant_t card_src = src;
  network_card_Constant_t card_dst = dst;
  return ROUTE_SIZE(card_src->id, card_dst->id);
}

static const char *get_link_name(const void *link)
{
  return ((link_Constant_t) link)->name;
}

static double get_link_bandwidth(const void *link)
{
  return ((link_Constant_t) link)->bw_current;
}

static double get_link_latency(const void *link)
{
  return ((link_Constant_t) link)->lat_current;
}

static xbt_dict_t get_properties(void *link)
{
 return ((link_Constant_t) link)->properties;
}

static void action_suspend(surf_action_t action)
{
  ((surf_action_network_Constant_t) action)->suspended = 1;
  lmm_update_variable_weight(network_maxmin_system,
			     ((surf_action_network_Constant_t) action)->
			     variable, 0.0);
}

static void action_resume(surf_action_t action)
{
  if (((surf_action_network_Constant_t) action)->suspended) {
    lmm_update_variable_weight(network_maxmin_system,
			       ((surf_action_network_Constant_t) action)->
			       variable,
			       ((surf_action_network_Constant_t) action)->
			       lat_current);
    ((surf_action_network_Constant_t) action)->suspended = 0;
  }
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
  int i, j;

  xbt_dict_free(&network_card_set);
  xbt_dict_free(&link_set);
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

  loopback = NULL;
  for (i = 0; i < card_number; i++)
    for (j = 0; j < card_number; j++)
      free(ROUTE(i, j));
  free(routing_table);
  routing_table = NULL;
  free(routing_table_size);
  routing_table_size = NULL;
  card_number = 0;
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
  surf_network_model->common_public->get_resource_name =
      get_resource_name;
  surf_network_model->common_public->action_get_state =
      surf_action_get_state;
  surf_network_model->common_public->action_get_start_time =
      surf_action_get_start_time;
  surf_network_model->common_public->action_get_finish_time =
      surf_action_get_finish_time;
  surf_network_model->common_public->action_free = action_free;
  surf_network_model->common_public->action_use = action_use;
  surf_network_model->common_public->action_cancel = action_cancel;
  surf_network_model->common_public->action_recycle = action_recycle;
  surf_network_model->common_public->action_change_state =
      action_change_state;
  surf_network_model->common_public->action_set_data =
      surf_action_set_data;
  surf_network_model->common_public->name = "network";

  surf_network_model->common_private->resource_used = resource_used;
  surf_network_model->common_private->share_resources = share_resources;
  surf_network_model->common_private->update_actions_state =
      update_actions_state;
  surf_network_model->common_private->update_resource_state =
      update_resource_state;
  surf_network_model->common_private->finalize = finalize;

  surf_network_model->common_public->suspend = action_suspend;
  surf_network_model->common_public->resume = action_resume;
  surf_network_model->common_public->is_suspended = action_is_suspended;
  surf_cpu_model->common_public->set_max_duration =
      action_set_max_duration;

  surf_network_model->extension_public->communicate = communicate;
  surf_network_model->extension_public->get_route = get_route;
  surf_network_model->extension_public->get_route_size = get_route_size;
  surf_network_model->extension_public->get_link_name = get_link_name;
  surf_network_model->extension_public->get_link_bandwidth =
      get_link_bandwidth;
  surf_network_model->extension_public->get_link_latency =
      get_link_latency;

  surf_network_model->common_public->get_properties =  get_properties;

  link_set = xbt_dict_new();
  network_card_set = xbt_dict_new();

  if (!network_maxmin_system)
    network_maxmin_system = lmm_system_new();
}

void surf_network_model_init_Constant(const char *filename)
{

  if (surf_network_model)
    return;
  surf_network_model_init_internal();
  define_callbacks(filename);
  xbt_dynar_push(model_list, &surf_network_model);
  network_solve = lmm_solve;

  update_model_description(surf_network_model_description,
			      surf_network_model_description_size,
			      "Constant",
			      (surf_model_t) surf_network_model);
}
