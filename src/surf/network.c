/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "network_private.h"
#include "xbt/log.h"
#include "xbt/str.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_network, surf,
                                "Logging specific to the SURF network module");

surf_model_t surf_network_model = NULL;
static lmm_system_t network_maxmin_system = NULL;
static void (*network_solve) (lmm_system_t) = NULL;

double sg_latency_factor = 1.0;    /* default value; can be set by model or from command line */
double sg_bandwidth_factor = 1.0;  /* default value; can be set by model or from command line */
double sg_weight_S_parameter = 0.0;/* default value; can be set by model or from command line */

double sg_tcp_gamma = 0.0;
int sg_network_fullduplex = 0;


/******************************************************************************/
/*                           Factors callbacks                                */
/******************************************************************************/ 
static double constant_latency_factor(double size)
{
  return sg_latency_factor;
}

static double constant_bandwidth_factor(double size)
{
  return sg_bandwidth_factor;
}

static double constant_bandwidth_constraint(double rate, double bound, double size)
{
  return rate;
}

/**********************/
/*   SMPI callbacks   */
/**********************/
static double smpi_latency_factor(double size)
{
  /* 1 B <= size <= 1 KiB */
  if (size <= 1024.0) {
    return 1.0056;
  }

  /* 2 KiB <= size <= 32 KiB */
  if (size <= 32768.0) {
    return 1.8805;
  }

  /* 64 KiB <= size <= 4 MiB */
  return 22.7111;
}

static double smpi_bandwidth_factor(double size)
{
  /* 1 B <= size <= 1 KiB */
  if (size <= 1024.0) {
    return 0.2758;
  }

  /* 2 KiB <= size <= 32 KiB */
  if (size <= 32768.0) {
    return 0.5477;
  }

  /* 64 KiB <= size <= 4 MiB */
  return 0.9359;
}

static double smpi_bandwidth_constraint(double rate, double bound, double size)
{
  return rate < 0 ? bound : min(bound, rate * smpi_bandwidth_factor(size));
}


static double (*latency_factor_callback)(double) = &constant_latency_factor;
static double (*bandwidth_factor_callback)(double) = &constant_bandwidth_factor;
static double (*bandwidth_constraint_callback)(double, double, double) = &constant_bandwidth_constraint;


static link_CM02_t net_link_new(char *name,
                            double bw_initial,
                            tmgr_trace_t bw_trace,
                            double lat_initial,
                            tmgr_trace_t lat_trace,
                            e_surf_resource_state_t
                            state_initial,
                            tmgr_trace_t state_trace,
                            e_surf_link_sharing_policy_t
                            policy, xbt_dict_t properties)
{
  link_CM02_t nw_link = (link_CM02_t)
    surf_resource_lmm_new(sizeof(s_link_CM02_t),
                          surf_network_model, name, properties,
                          network_maxmin_system,
                          sg_bandwidth_factor * bw_initial,
                          history,
                          state_initial, state_trace,
                          bw_initial, bw_trace);

  xbt_assert1(!xbt_dict_get_or_null(surf_network_model->resource_set, name),
              "Link '%s' declared several times in the platform file.", name);

  nw_link->lat_current = lat_initial;
  if (lat_trace)
    nw_link->lat_event =
      tmgr_history_add_trace(history, lat_trace, 0.0, 0, nw_link);

  if (policy == SURF_LINK_FATPIPE)
    lmm_constraint_shared(nw_link->lmm_resource.constraint);

  xbt_dict_set(surf_network_model->resource_set, name, nw_link,
               surf_resource_free);
#ifdef HAVE_TRACING
  TRACE_surf_link_declaration (name, bw_initial, lat_initial);
#endif

  return nw_link;
}

static void net_parse_link_init(void)
{
  char *name_link;
  double bw_initial;
  tmgr_trace_t bw_trace;
  double lat_initial;
  tmgr_trace_t lat_trace;
  e_surf_resource_state_t state_initial_link = SURF_RESOURCE_ON;
  e_surf_link_sharing_policy_t policy_initial_link = SURF_LINK_SHARED;
  tmgr_trace_t state_trace;

  name_link = xbt_strdup(A_surfxml_link_id);
  surf_parse_get_double(&bw_initial, A_surfxml_link_bandwidth);
  bw_trace = tmgr_trace_new(A_surfxml_link_bandwidth_file);
  surf_parse_get_double(&lat_initial, A_surfxml_link_latency);
  lat_trace = tmgr_trace_new(A_surfxml_link_latency_file);

  xbt_assert0((A_surfxml_link_state == A_surfxml_link_state_ON)
              || (A_surfxml_link_state ==
                  A_surfxml_link_state_OFF), "Invalid state");
  if (A_surfxml_link_state == A_surfxml_link_state_ON)
    state_initial_link = SURF_RESOURCE_ON;
  else if (A_surfxml_link_state == A_surfxml_link_state_OFF)
    state_initial_link = SURF_RESOURCE_OFF;

  if (A_surfxml_link_sharing_policy == A_surfxml_link_sharing_policy_SHARED)
    policy_initial_link = SURF_LINK_SHARED;
  else if (A_surfxml_link_sharing_policy ==
           A_surfxml_link_sharing_policy_FATPIPE)
    policy_initial_link = SURF_LINK_FATPIPE;

  state_trace = tmgr_trace_new(A_surfxml_link_state_file);

  net_link_new(name_link, bw_initial, bw_trace,
           lat_initial, lat_trace, state_initial_link, state_trace,
           policy_initial_link, xbt_dict_new());

}

static void net_add_traces(void)
{
  xbt_dict_cursor_t cursor = NULL;
  char *trace_name, *elm;

  static int called = 0;
  if (called)
    return;
  called = 1;

  /* connect all traces relative to network */
  xbt_dict_foreach(trace_connect_list_link_avail, cursor, trace_name, elm) {
    tmgr_trace_t trace = xbt_dict_get_or_null(traces_set_list, trace_name);
    link_CM02_t link =
      xbt_dict_get_or_null(surf_network_model->resource_set, elm);

    xbt_assert2(link, "Cannot connect trace %s to link %s: link undefined",
                trace_name, elm);
    xbt_assert2(trace, "Cannot connect trace %s to link %s: trace undefined",
                trace_name, elm);

    link->lmm_resource.state_event =
      tmgr_history_add_trace(history, trace, 0.0, 0, link);
  }

  xbt_dict_foreach(trace_connect_list_bandwidth, cursor, trace_name, elm) {
    tmgr_trace_t trace = xbt_dict_get_or_null(traces_set_list, trace_name);
    link_CM02_t link =
      xbt_dict_get_or_null(surf_network_model->resource_set, elm);

    xbt_assert2(link, "Cannot connect trace %s to link %s: link undefined",
                trace_name, elm);
    xbt_assert2(trace, "Cannot connect trace %s to link %s: trace undefined",
                trace_name, elm);

    link->lmm_resource.power.event =
      tmgr_history_add_trace(history, trace, 0.0, 0, link);
  }

  xbt_dict_foreach(trace_connect_list_latency, cursor, trace_name, elm) {
    tmgr_trace_t trace = xbt_dict_get_or_null(traces_set_list, trace_name);
    link_CM02_t link =
      xbt_dict_get_or_null(surf_network_model->resource_set, elm);

    xbt_assert2(link, "Cannot connect trace %s to link %s: link undefined",
                trace_name, elm);
    xbt_assert2(trace, "Cannot connect trace %s to link %s: trace undefined",
                trace_name, elm);

    link->lat_event = tmgr_history_add_trace(history, trace, 0.0, 0, link);
  }
}

static void net_define_callbacks(const char *file)
{
  /* Figuring out the network links */
  surfxml_add_callback(STag_surfxml_link_cb_list, &net_parse_link_init);
  surfxml_add_callback(ETag_surfxml_platform_cb_list, &net_add_traces);
}

static int net_resource_used(void *resource_id)
{
  return lmm_constraint_used(network_maxmin_system,
                             ((surf_resource_lmm_t) resource_id)->constraint);
}

static int net_action_unref(surf_action_t action)
{
  action->refcount--;
  if (!action->refcount) {
    xbt_swag_remove(action, action->state_set);
    if (((surf_action_network_CM02_t) action)->variable)
      lmm_variable_free(network_maxmin_system,
                        ((surf_action_network_CM02_t) action)->variable);
    free(action);
    return 1;
  }
  return 0;
}

static void net_action_cancel(surf_action_t action)
{
  surf_network_model->action_state_set((surf_action_t) action,
                                       SURF_ACTION_FAILED);
  return;
}

static void net_action_recycle(surf_action_t action)
{
  return;
}

static int net_get_link_latency(surf_action_t action)
{
  return action->latency_limited;
}

static double net_action_get_remains(surf_action_t action)
{
  return action->remains;
}

static double net_share_resources(double now)
{
  s_surf_action_network_CM02_t s_action;
  surf_action_network_CM02_t action = NULL;
  xbt_swag_t running_actions = surf_network_model->states.running_action_set;
  double min;

  min = generic_maxmin_share_resources(running_actions,
                                       xbt_swag_offset(s_action,
                                                       variable),
                                       network_maxmin_system, network_solve);

#define VARIABLE(action) (*((lmm_variable_t*)(((char *) (action)) + xbt_swag_offset(s_action, variable)  )))

  xbt_swag_foreach(action, running_actions) {
	if( lmm_is_variable_limited_by_latency(action->variable) ){
		(action->generic_action).latency_limited = 1;
	}else{
		(action->generic_action).latency_limited = 0;
	}
    if (action->latency > 0) {
      if (min < 0)
        min = action->latency;
      else if (action->latency < min)
        min = action->latency;
    }
  }

  return min;
}

static void net_update_actions_state(double now, double delta)
{
  double deltap = 0.0;
  surf_action_network_CM02_t action = NULL;
  surf_action_network_CM02_t next_action = NULL;
  xbt_swag_t running_actions = surf_network_model->states.running_action_set;
  /*
     xbt_swag_t failed_actions =
     surf_network_model->states.failed_action_set;
   */

  xbt_swag_foreach_safe(action, next_action, running_actions) {

#ifdef HAVE_TRACING
    xbt_dynar_t route = used_routing->get_route(action->src, action->dst);
    link_CM02_t link;
    unsigned int i;
    xbt_dynar_foreach(route, i, link) {
    	TRACE_surf_link_set_utilization (link->lmm_resource.generic_resource.name,
          action->generic_action.data, lmm_variable_getvalue(action->variable), now-delta, delta);
    }
#endif

    deltap = delta;
    if (action->latency > 0) {
      if (action->latency > deltap) {
        double_update(&(action->latency), deltap);
        deltap = 0.0;
      } else {
        double_update(&(deltap), action->latency);
        action->latency = 0.0;
      }
      if ((action->latency == 0.0) && !(action->suspended))
        lmm_update_variable_weight(network_maxmin_system, action->variable,
                                   action->weight);
    }
    double_update(&(action->generic_action.remains),
                  lmm_variable_getvalue(action->variable) * deltap);
    if (action->generic_action.max_duration != NO_MAX_DURATION)
      double_update(&(action->generic_action.max_duration), delta);

    if ((action->generic_action.remains <= 0) &&
        (lmm_get_variable_weight(action->variable) > 0)) {
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

  return;
}

static void net_update_resource_state(void *id,
                                  tmgr_trace_event_t event_type,
                                  double value, double date)
{
  link_CM02_t nw_link = id;
  /*   printf("[" "%lg" "] Asking to update network card \"%s\" with value " */
  /*     "%lg" " for event %p\n", surf_get_clock(), nw_link->name, */
  /*     value, event_type); */

  if (event_type == nw_link->lmm_resource.power.event) {
    double delta =
      sg_weight_S_parameter / value - sg_weight_S_parameter /
      (nw_link->lmm_resource.power.peak * nw_link->lmm_resource.power.scale);
    lmm_variable_t var = NULL;
    lmm_element_t elem = NULL;
    surf_action_network_CM02_t action = NULL;

    nw_link->lmm_resource.power.peak = value;
    lmm_update_constraint_bound(network_maxmin_system,
                                nw_link->lmm_resource.constraint,
                                sg_bandwidth_factor *
                                (nw_link->lmm_resource.power.peak *
                                 nw_link->lmm_resource.power.scale));
#ifdef HAVE_TRACING
    TRACE_surf_link_set_bandwidth (date, nw_link->lmm_resource.generic_resource.name, sg_bandwidth_factor * (nw_link->lmm_resource.power.peak * nw_link->lmm_resource.power.scale));
#endif
    if (sg_weight_S_parameter > 0) {
      while ((var = lmm_get_var_from_cnst
              (network_maxmin_system, nw_link->lmm_resource.constraint,
               &elem))) {
        action = lmm_variable_id(var);
        action->weight += delta;
        if (!(action->suspended))
          lmm_update_variable_weight(network_maxmin_system, action->variable,
                                     action->weight);
      }
    }
    if (tmgr_trace_event_free(event_type))
      nw_link->lmm_resource.power.event = NULL;
  } else if (event_type == nw_link->lat_event) {
    double delta = value - nw_link->lat_current;
    lmm_variable_t var = NULL;
    lmm_element_t elem = NULL;
    surf_action_network_CM02_t action = NULL;

    nw_link->lat_current = value;
    while ((var = lmm_get_var_from_cnst
            (network_maxmin_system, nw_link->lmm_resource.constraint,
             &elem))) {
      action = lmm_variable_id(var);
      action->lat_current += delta;
      action->weight += delta;
      if (action->rate < 0)
        lmm_update_variable_bound(network_maxmin_system, action->variable,
                                  sg_tcp_gamma / (2.0 * action->lat_current));
      else{
        lmm_update_variable_bound(network_maxmin_system, action->variable,
                                  min(action->rate,
                                      sg_tcp_gamma / (2.0 *
                                                      action->lat_current)));

        if(action->rate < sg_tcp_gamma / (2.0 * action->lat_current) ){
        	INFO0("Flow is limited BYBANDWIDTH");
        }else{
        	INFO1("Flow is limited BYLATENCY, latency of flow is %f",action->lat_current);
        }
      }
      if (!(action->suspended))
        lmm_update_variable_weight(network_maxmin_system, action->variable,
                                   action->weight);

    }
    if (tmgr_trace_event_free(event_type))
      nw_link->lat_event = NULL;
  } else if (event_type == nw_link->lmm_resource.state_event) {
    if (value > 0)
      nw_link->lmm_resource.state_current = SURF_RESOURCE_ON;
    else {
      lmm_constraint_t cnst = nw_link->lmm_resource.constraint;
      lmm_variable_t var = NULL;
      lmm_element_t elem = NULL;

      nw_link->lmm_resource.state_current = SURF_RESOURCE_OFF;
      while ((var = lmm_get_var_from_cnst
              (network_maxmin_system, cnst, &elem))) {
        surf_action_t action = lmm_variable_id(var);

        if (surf_action_state_get(action) == SURF_ACTION_RUNNING ||
            surf_action_state_get(action) == SURF_ACTION_READY) {
          action->finish = date;
          surf_network_model->action_state_set(action, SURF_ACTION_FAILED);
        }
      }
    }
    if (tmgr_trace_event_free(event_type))
      nw_link->lmm_resource.state_event = NULL;
  } else {
    CRITICAL0("Unknown event ! \n");
    xbt_abort();
  }

  return;
}


static surf_action_t net_communicate(const char *src_name, const char *dst_name,
                                 int src, int dst, double size, double rate)
{
  unsigned int i;
  link_CM02_t link;
  int failed = 0;
  surf_action_network_CM02_t action = NULL;
  double bandwidth_bound;
  /* LARGE PLATFORMS HACK:
     Add a link_CM02_t *link and a int link_nb to network_card_CM02_t. It will represent local links for this node
     Use the cluster_id for ->id */
  xbt_dynar_t route = used_routing->get_route(src, dst);
  xbt_dynar_t back_route = NULL;
  int constraints_per_variable = 0;

  if( sg_network_fullduplex == 1){
	  back_route = used_routing->get_route(dst, src);
  }

  /* LARGE PLATFORMS HACK:
     total_route_size = route_size + src->link_nb + dst->nb */

  XBT_IN4("(%s,%s,%g,%g)", src_name, dst_name, size, rate);
  /* LARGE PLATFORMS HACK:
     assert on total_route_size */
  xbt_assert2(xbt_dynar_length(route),
              "You're trying to send data from %s to %s but there is no connection between these two hosts.",
              src_name, dst_name);

  xbt_dynar_foreach(route, i, link) {
    if (link->lmm_resource.state_current == SURF_RESOURCE_OFF) {
      failed = 1;
      break;
    }
  }
  action =
    surf_action_new(sizeof(s_surf_action_network_CM02_t), size,
                    surf_network_model, failed);
  (action->generic_action).latency_limited = 0;

  xbt_swag_insert(action, action->generic_action.state_set);
  action->rate = rate;

  action->latency = 0.0;
  action->weight = 0.0;
  bandwidth_bound = -1.0;
  xbt_dynar_foreach(route, i, link) {
    action->latency += link->lat_current;
    action->weight +=
      link->lat_current +
      sg_weight_S_parameter /
      (link->lmm_resource.power.peak * link->lmm_resource.power.scale);
    if(bandwidth_bound < 0.0)
      bandwidth_bound = (*bandwidth_factor_callback)(size) * (link->lmm_resource.power.peak * link->lmm_resource.power.scale);
    else
      bandwidth_bound = min(bandwidth_bound, (*bandwidth_factor_callback)(size) * (link->lmm_resource.power.peak * link->lmm_resource.power.scale));
  }
  /* LARGE PLATFORMS HACK:
     Add src->link and dst->link latencies */
  action->lat_current = action->latency;
  action->latency *= (*latency_factor_callback)(size);
  action->rate = (*bandwidth_constraint_callback)(action->rate, bandwidth_bound, size);

  /* LARGE PLATFORMS HACK:
     lmm_variable_new(..., total_route_size) */
  if(back_route != NULL){
	  constraints_per_variable = xbt_dynar_length(route)+xbt_dynar_length(back_route);
  }else{
	  constraints_per_variable = xbt_dynar_length(route);
  }

  if (action->latency > 0)
    action->variable =
      lmm_variable_new(network_maxmin_system, action, 0.0, -1.0,constraints_per_variable);
  else
    action->variable =
      lmm_variable_new(network_maxmin_system, action, 1.0, -1.0,constraints_per_variable);

  if (action->rate < 0) {
    if (action->lat_current > 0)
      lmm_update_variable_bound(network_maxmin_system, action->variable,
                                sg_tcp_gamma / (2.0 * action->lat_current));
    else
      lmm_update_variable_bound(network_maxmin_system, action->variable,
                                -1.0);
  } else {
    if (action->lat_current > 0)
      lmm_update_variable_bound(network_maxmin_system, action->variable,
                                min(action->rate,
                                    sg_tcp_gamma / (2.0 *
                                                    action->lat_current)));
    else
      lmm_update_variable_bound(network_maxmin_system, action->variable,
                                action->rate);
  }

  xbt_dynar_foreach(route, i, link) {
    lmm_expand(network_maxmin_system, link->lmm_resource.constraint,
               action->variable, 1.0);
  }

  if( sg_network_fullduplex == 1){
	  DEBUG1("Fullduplex active adding backward flow using 5%c", '%');
	  xbt_dynar_foreach(back_route, i, link) {
		  lmm_expand(network_maxmin_system, link->lmm_resource.constraint,
               action->variable, .05);
	  }
  }  /* LARGE PLATFORMS HACK:
     expand also with src->link and dst->link */

  /* saving the src and dst of this communication */
  action->src = src;
  action->dst = dst;

  XBT_OUT;

  return (surf_action_t) action;
}

static double net_get_link_bandwidth(const void *link)
{
  surf_resource_lmm_t lmm = (surf_resource_lmm_t) link;
  return lmm->power.peak * lmm->power.scale;
}

static int net_link_shared(const void *link)
{
  return lmm_constraint_is_shared(((surf_resource_lmm_t) link)->constraint);
}

static void net_action_suspend(surf_action_t action)
{
  ((surf_action_network_CM02_t) action)->suspended = 1;
  lmm_update_variable_weight(network_maxmin_system,
                             ((surf_action_network_CM02_t) action)->variable,
                             0.0);
}

static void net_action_resume(surf_action_t action)
{
  if (((surf_action_network_CM02_t) action)->suspended) {
    lmm_update_variable_weight(network_maxmin_system,
                               ((surf_action_network_CM02_t)
                                action)->variable,
                               ((surf_action_network_CM02_t) action)->weight);
    ((surf_action_network_CM02_t) action)->suspended = 0;
  }
}

static int net_action_is_suspended(surf_action_t action)
{
  return ((surf_action_network_CM02_t) action)->suspended;
}

static void net_action_set_max_duration(surf_action_t action, double duration)
{
  action->max_duration = duration;
}


/**
 * FIXME : this should be done in the binding code !!
 */
void network_create_resource(char *name,
        double initial_bandwidth,double initial_latency)
{

	char* name_link;
	double bw_initial;
	tmgr_trace_t bw_trace;
	double lat_initial;
	tmgr_trace_t lat_trace;
	e_surf_resource_state_t state_initial_link = SURF_RESOURCE_ON;
	e_surf_link_sharing_policy_t policy_initial_link = SURF_LINK_SHARED;
	tmgr_trace_t state_trace;

	name_link = xbt_strdup(name);
	bw_initial = initial_bandwidth;
	bw_trace = tmgr_trace_new("");
	lat_initial = initial_latency;
	lat_trace = tmgr_trace_new("");
	// FIXME Hard Coded Values
	//state_initial_link = SURF_RESOURCE_ON;
	//policy_initial_link = SURF_LINK_SHARED;
	state_trace = tmgr_trace_new("");

	net_link_new(name_link, bw_initial, bw_trace,
	           lat_initial, lat_trace, state_initial_link, state_trace,
	           policy_initial_link, xbt_dict_new());
}

static void net_finalize(void)
{
  surf_model_exit(surf_network_model);
  surf_network_model = NULL;

  used_routing->finalize();
  lmm_system_free(network_maxmin_system);
  network_maxmin_system = NULL;
}

static void surf_network_model_init_internal(void)
{
  surf_network_model = surf_model_init();

  surf_network_model->name = "network";
  surf_network_model->action_unref = net_action_unref;
  surf_network_model->action_cancel = net_action_cancel;
  surf_network_model->action_recycle = net_action_recycle;
  surf_network_model->get_remains = net_action_get_remains;
  surf_network_model->get_latency_limited = net_get_link_latency;

  surf_network_model->model_private->resource_used = net_resource_used;
  surf_network_model->model_private->share_resources = net_share_resources;
  surf_network_model->model_private->update_actions_state =
    net_update_actions_state;
  surf_network_model->model_private->update_resource_state =
    net_update_resource_state;
  surf_network_model->model_private->finalize = net_finalize;

  surf_network_model->suspend = net_action_suspend;
  surf_network_model->resume = net_action_resume;
  surf_network_model->is_suspended = net_action_is_suspended;
  surf_cpu_model->set_max_duration = net_action_set_max_duration;

  surf_network_model->extension.network.communicate = net_communicate;
  surf_network_model->extension.network.get_link_bandwidth =
    net_get_link_bandwidth;
  surf_network_model->extension.network.link_shared = net_link_shared;
  surf_network_model->extension.network.create_resource = network_create_resource;
  surf_network_model->extension.network.add_traces = net_add_traces;

  if (!network_maxmin_system)
    network_maxmin_system = lmm_system_new();

  routing_model_create(sizeof(link_CM02_t),
                       net_link_new(xbt_strdup("__loopback__"),
                                498000000, NULL, 0.000015, NULL,
                                SURF_RESOURCE_ON, NULL, SURF_LINK_FATPIPE,
                                NULL));
}



/************************************************************************/
/* New model based on LV08 and experimental results of MPI ping-pongs   */
/************************************************************************/
void surf_network_model_init_SMPI(const char *filename)
{

  if (surf_network_model)
    return;
  surf_network_model_init_internal();
  latency_factor_callback = &smpi_latency_factor;
  bandwidth_factor_callback = &smpi_bandwidth_factor;
  bandwidth_constraint_callback = &smpi_bandwidth_constraint;
  net_define_callbacks(filename);
  xbt_dynar_push(model_list, &surf_network_model);
  network_solve = lmm_solve;

  xbt_cfg_setdefault_double(_surf_cfg_set,"network/weight_S", 8775);

  update_model_description(surf_network_model_description,
                           "SMPI", surf_network_model);
}

/************************************************************************/
/* New model based on optimizations discussed during this thesis        */
/************************************************************************/
void surf_network_model_init_LegrandVelho(const char *filename)
{

  if (surf_network_model)
    return;
  surf_network_model_init_internal();
  net_define_callbacks(filename);
  xbt_dynar_push(model_list, &surf_network_model);
  network_solve = lmm_solve;

  xbt_cfg_setdefault_double(_surf_cfg_set,"network/latency_factor",10.4);
  xbt_cfg_setdefault_double(_surf_cfg_set,"network/bandwidth_factor", 0.92);
  xbt_cfg_setdefault_double(_surf_cfg_set,"network/weight_S", 8775);

  update_model_description(surf_network_model_description,
                           "LV08", surf_network_model);
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
void surf_network_model_init_CM02(const char *filename)
{

  if (surf_network_model)
    return;
  surf_network_model_init_internal();
  net_define_callbacks(filename);
  xbt_dynar_push(model_list, &surf_network_model);
  network_solve = lmm_solve;

  update_model_description(surf_network_model_description,
                           "CM02", surf_network_model);
}

void surf_network_model_init_Reno(const char *filename)
{
  if (surf_network_model)
    return;
  surf_network_model_init_internal();
  net_define_callbacks(filename);

  xbt_dynar_push(model_list, &surf_network_model);
  lmm_set_default_protocol_function(func_reno_f, func_reno_fp, func_reno_fpi);
  network_solve = lagrange_solve;

  xbt_cfg_setdefault_double(_surf_cfg_set,"network/latency_factor", 10.4);
  xbt_cfg_setdefault_double(_surf_cfg_set,"network/bandwidth_factor", 0.92);
  xbt_cfg_setdefault_double(_surf_cfg_set,"network/weight_S", 8775);

  update_model_description(surf_network_model_description,
                           "Reno", surf_network_model);
}


void surf_network_model_init_Reno2(const char *filename)
{
  if (surf_network_model)
    return;
  surf_network_model_init_internal();
  net_define_callbacks(filename);

  xbt_dynar_push(model_list, &surf_network_model);
  lmm_set_default_protocol_function(func_reno2_f, func_reno2_fp,
                                    func_reno2_fpi);
  network_solve = lagrange_solve;

  xbt_cfg_setdefault_double(_surf_cfg_set,"network/latency_factor", 10.4);
  xbt_cfg_setdefault_double(_surf_cfg_set,"network/bandwidth_factor", 0.92);
  xbt_cfg_setdefault_double(_surf_cfg_set,"network/weight_S_parameter", 8775);

  update_model_description(surf_network_model_description,
                           "Reno2", surf_network_model);
}

void surf_network_model_init_Vegas(const char *filename)
{
  if (surf_network_model)
    return;
  surf_network_model_init_internal();
  net_define_callbacks(filename);

  xbt_dynar_push(model_list, &surf_network_model);
  lmm_set_default_protocol_function(func_vegas_f, func_vegas_fp,
                                    func_vegas_fpi);
  network_solve = lagrange_solve;

  xbt_cfg_setdefault_double(_surf_cfg_set,"network/latency_factor", 10.4);
  xbt_cfg_setdefault_double(_surf_cfg_set,"network/bandwidth_factor", 0.92);
  xbt_cfg_setdefault_double(_surf_cfg_set,"network/weight_S", 8775);

  update_model_description(surf_network_model_description,
                           "Vegas", surf_network_model);
}


