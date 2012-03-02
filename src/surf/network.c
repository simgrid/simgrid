
/*
 * Network with improved management of tasks, IM (Improved Management).
 * Uses a heap to store actions so that the share_resources is faster.
 * This model automatically sets the selective update flag to 1 and is
 * highly dependent on the maxmin lmm module.
 */

/* Copyright (c) 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "network_private.h"
#include "xbt/log.h"
#include "xbt/str.h"

#include "surf_private.h"
#include "xbt/dict.h"
#include "maxmin_private.h"
#include "surf/surfxml_parse_values.h"
#include "surf/surf_resource.h"
#include "surf/surf_resource_lmm.h"

#undef GENERIC_ACTION
#define GENERIC_ACTION(action) action->generic_action

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_network, surf,
				"Logging specific to the SURF network module");

surf_model_t surf_network_model = NULL;
static lmm_system_t network_maxmin_system = NULL;
static void (*network_solve) (lmm_system_t) = NULL;

xbt_dynar_t smpi_bw_factor = NULL;
xbt_dynar_t smpi_lat_factor = NULL;

typedef struct s_smpi_factor *smpi_factor_t;
typedef struct s_smpi_factor {
  long factor;
  double value;
} s_smpi_factor_t;

double sg_sender_gap = 0.0;
double sg_latency_factor = 1.0;	/* default value; can be set by model or from command line */
double sg_bandwidth_factor = 1.0;	/* default value; can be set by model or from command line */
double sg_weight_S_parameter = 0.0;	/* default value; can be set by model or from command line */

double sg_tcp_gamma = 0.0;
int sg_network_crosstraffic = 0;

xbt_dict_t gap_lookup = NULL;

e_UM_t network_update_mechanism = UM_UNDEFINED;
static int net_selective_update = 0;

static int net_action_is_suspended(surf_action_t action);
static void update_action_remaining(double now);

static xbt_swag_t net_modified_set = NULL;
static xbt_heap_t net_action_heap = NULL;

/* added to manage the communication action's heap */
static void net_action_update_index_heap(void *action, int i)
{
  ((surf_action_network_CM02_t) action)->index_heap = i;
}

/* insert action on heap using a given key and a hat (heap_action_type)
 * a hat can be of three types for communications:
 *
 * NORMAL = this is a normal heap entry stating the date to finish transmitting
 * LATENCY = this is a heap entry to warn us when the latency is payed
 * MAX_DURATION =this is a heap entry to warn us when the max_duration limit is reached
 */
static void heap_insert(surf_action_network_CM02_t action, double key,
			enum heap_action_type hat)
{
  action->hat = hat;
  xbt_heap_push(net_action_heap, action, key);
}

static void heap_remove(surf_action_network_CM02_t action)
{
  action->hat = NOTSET;
  if (((surf_action_network_CM02_t) action)->index_heap >= 0) {
    xbt_heap_remove(net_action_heap, action->index_heap);
  }
}

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

static double constant_bandwidth_constraint(double rate, double bound,
					    double size)
{
  return rate;
}

/**********************/
/*   SMPI callbacks   */
/**********************/
static xbt_dynar_t parse_factor(const char *smpi_coef_string)
{
  char *value = NULL;
  unsigned int iter = 0;
  s_smpi_factor_t fact;
  xbt_dynar_t smpi_factor, radical_elements, radical_elements2 = NULL;

  smpi_factor = xbt_dynar_new(sizeof(s_smpi_factor_t), NULL);
  radical_elements = xbt_str_split(smpi_coef_string, ";");
  xbt_dynar_foreach(radical_elements, iter, value) {

    radical_elements2 = xbt_str_split(value, ":");
    if (xbt_dynar_length(radical_elements2) != 2)
      xbt_die("Malformed radical for smpi factor!");
    fact.factor = atol(xbt_dynar_get_as(radical_elements2, 0, char *));
    fact.value = atof(xbt_dynar_get_as(radical_elements2, 1, char *));
    xbt_dynar_push_as(smpi_factor, s_smpi_factor_t, fact);
    XBT_DEBUG("smpi_factor:\t%ld : %f", fact.factor, fact.value);
    xbt_dynar_free(&radical_elements2);
  }
  xbt_dynar_free(&radical_elements);
  return smpi_factor;
}

static double smpi_bandwidth_factor(double size)
{
  if (!smpi_bw_factor)
    smpi_bw_factor =
	parse_factor(xbt_cfg_get_string(_surf_cfg_set, "smpi/bw_factor"));

  unsigned int iter = 0;
  s_smpi_factor_t fact;
  xbt_dynar_foreach(smpi_bw_factor, iter, fact) {
    if (size >= fact.factor) {
      XBT_DEBUG("%lf >= %ld return %f", size, fact.factor, fact.value);
      return fact.value;
    }
  }

  return 1.0;
}

static double smpi_latency_factor(double size)
{
  if (!smpi_lat_factor)
    smpi_lat_factor =
	parse_factor(xbt_cfg_get_string(_surf_cfg_set, "smpi/lat_factor"));

  unsigned int iter = 0;
  s_smpi_factor_t fact;
  xbt_dynar_foreach(smpi_lat_factor, iter, fact) {
    if (size >= fact.factor) {
      XBT_DEBUG("%lf >= %ld return %f", size, fact.factor, fact.value);
      return fact.value;
    }
  }

  return 1.0;
}

/**--------- <copy/paste C code snippet in surf/network.c> -----------*/

static double smpi_bandwidth_constraint(double rate, double bound,
					double size)
{
  return rate < 0 ? bound : min(bound, rate * smpi_bandwidth_factor(size));
}

static double (*latency_factor_callback) (double) =
    &constant_latency_factor;
static double (*bandwidth_factor_callback) (double) =
    &constant_bandwidth_factor;
static double (*bandwidth_constraint_callback) (double, double, double) =
    &constant_bandwidth_constraint;

static void (*gap_append) (double, const link_CM02_t,
			   surf_action_network_CM02_t) = NULL;
static void (*gap_remove) (surf_action_network_CM02_t) = NULL;

static void *net_create_resource(const char *name,
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

  xbt_assert(!xbt_lib_get_or_null(link_lib, name, SURF_LINK_LEVEL),
	     "Link '%s' declared several times in the platform file.",
	     name);

  nw_link->lat_current = lat_initial;
  if (lat_trace)
    nw_link->lat_event =
	tmgr_history_add_trace(history, lat_trace, 0.0, 0, nw_link);

  if (policy == SURF_LINK_FATPIPE)
    lmm_constraint_shared(nw_link->lmm_resource.constraint);

  xbt_lib_set(link_lib, name, SURF_LINK_LEVEL, nw_link);

  return nw_link;
}

static void net_parse_link_init(sg_platf_link_cbarg_t link)
{
  if (link->policy == SURF_LINK_FULLDUPLEX) {
    char *link_id;
    link_id = bprintf("%s_UP", link->id);
    net_create_resource(link_id,
			link->bandwidth,
			link->bandwidth_trace,
			link->latency,
			link->latency_trace,
			link->state,
			link->state_trace, link->policy, link->properties);
    xbt_free(link_id);
    link_id = bprintf("%s_DOWN", link->id);
    net_create_resource(link_id,
			link->bandwidth,
			link->bandwidth_trace,
			link->latency,
			link->latency_trace,
			link->state,
			link->state_trace, link->policy, link->properties);
    xbt_free(link_id);
  } else {
    net_create_resource(link->id,
			link->bandwidth,
			link->bandwidth_trace,
			link->latency,
			link->latency_trace,
			link->state,
			link->state_trace, link->policy, link->properties);
  }
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
    link_CM02_t link = xbt_lib_get_or_null(link_lib, elm, SURF_LINK_LEVEL);

    xbt_assert(link, "Cannot connect trace %s to link %s: link undefined",
	       trace_name, elm);
    xbt_assert(trace,
	       "Cannot connect trace %s to link %s: trace undefined",
	       trace_name, elm);

    link->lmm_resource.state_event =
	tmgr_history_add_trace(history, trace, 0.0, 0, link);
  }

  xbt_dict_foreach(trace_connect_list_bandwidth, cursor, trace_name, elm) {
    tmgr_trace_t trace = xbt_dict_get_or_null(traces_set_list, trace_name);
    link_CM02_t link = xbt_lib_get_or_null(link_lib, elm, SURF_LINK_LEVEL);

    xbt_assert(link, "Cannot connect trace %s to link %s: link undefined",
	       trace_name, elm);
    xbt_assert(trace,
	       "Cannot connect trace %s to link %s: trace undefined",
	       trace_name, elm);

    link->lmm_resource.power.event =
	tmgr_history_add_trace(history, trace, 0.0, 0, link);
  }

  xbt_dict_foreach(trace_connect_list_latency, cursor, trace_name, elm) {
    tmgr_trace_t trace = xbt_dict_get_or_null(traces_set_list, trace_name);
    link_CM02_t link = xbt_lib_get_or_null(link_lib, elm, SURF_LINK_LEVEL);

    xbt_assert(link, "Cannot connect trace %s to link %s: link undefined",
	       trace_name, elm);
    xbt_assert(trace,
	       "Cannot connect trace %s to link %s: trace undefined",
	       trace_name, elm);

    link->lat_event = tmgr_history_add_trace(history, trace, 0.0, 0, link);
  }
}

static void net_define_callbacks(void)
{
  /* Figuring out the network links */
  sg_platf_link_add_cb(net_parse_link_init);
  sg_platf_postparse_add_cb(net_add_traces);
}

static int net_resource_used(void *resource_id)
{
  return lmm_constraint_used(network_maxmin_system, ((surf_resource_lmm_t)
						     resource_id)->
			     constraint);
}

static int net_action_unref(surf_action_t action)
{
  action->refcount--;
  if (!action->refcount) {
    xbt_swag_remove(action, action->state_set);
    if (((surf_action_network_CM02_t) action)->variable) {
      lmm_variable_free(network_maxmin_system,
			((surf_action_network_CM02_t) action)->variable);
    }
    if (network_update_mechanism == UM_LAZY) {	// remove action from the heap
      heap_remove((surf_action_network_CM02_t) action);
      xbt_swag_remove(action, net_modified_set);
    }
#ifdef HAVE_TRACING
    xbt_free(((surf_action_network_CM02_t) action)->src_name);
    xbt_free(((surf_action_network_CM02_t) action)->dst_name);
    xbt_free(action->category);
#endif
    surf_action_free(&action);
    return 1;
  }
  return 0;
}



static void net_action_cancel(surf_action_t action)
{
  XBT_DEBUG("cancel action %p", action);
  surf_network_model->action_state_set(action, SURF_ACTION_FAILED);
  if (network_update_mechanism == UM_LAZY) {	// remove action from the heap
    xbt_swag_remove(action, net_modified_set);
    heap_remove((surf_action_network_CM02_t) action);
  }
}

void net_action_recycle(surf_action_t action)
{
  return;
}

#ifdef HAVE_LATENCY_BOUND_TRACKING
int net_get_link_latency_limited(surf_action_t action)
{
  return action->latency_limited;
}
#endif

double net_action_get_remains(surf_action_t action)
{
  if (network_update_mechanism == UM_LAZY)	/* update remains before return it */
    update_action_remaining(surf_get_clock());
  return action->remains;
}

static void update_action_remaining(double now)
{
  surf_action_network_CM02_t action = NULL;
  double delta = 0.0;

  xbt_swag_foreach(action, net_modified_set) {

    if (action->suspended != 0) {
      continue;
    }

    delta = now - action->last_update;

    double_update(&(action->generic_action.remains),
		  lmm_variable_getvalue(action->variable) * delta);

    if (action->generic_action.max_duration != NO_MAX_DURATION)
      double_update(&(action->generic_action.max_duration), delta);

    if ((action->generic_action.remains <= 0) &&
	(lmm_get_variable_weight(action->variable) > 0)) {
      action->generic_action.finish = surf_get_clock();
      surf_network_model->action_state_set((surf_action_t) action,
					   SURF_ACTION_DONE);
      heap_remove(action);
    } else if ((action->generic_action.max_duration != NO_MAX_DURATION)
	       && (action->generic_action.max_duration <= 0)) {
      action->generic_action.finish = surf_get_clock();
      surf_network_model->action_state_set((surf_action_t) action,
					   SURF_ACTION_DONE);
      heap_remove(action);
    }

    action->last_update = now;
  }
}

static double net_share_resources_full(double now)
{
  s_surf_action_network_CM02_t s_action;
  surf_action_network_CM02_t action = NULL;
  xbt_swag_t running_actions =
      surf_network_model->states.running_action_set;
  double min;

  min = generic_maxmin_share_resources(running_actions,
				       xbt_swag_offset(s_action,
						       variable),
				       network_maxmin_system,
				       network_solve);

#define VARIABLE(action) (*((lmm_variable_t*)(((char *) (action)) + xbt_swag_offset(s_action, variable)  )))

  xbt_swag_foreach(action, running_actions) {
#ifdef HAVE_LATENCY_BOUND_TRACKING
    if (lmm_is_variable_limited_by_latency(action->variable)) {
      (action->generic_action).latency_limited = 1;
    } else {
      (action->generic_action).latency_limited = 0;
    }
#endif
    if (action->latency > 0) {
      min = (min < 0) ? action->latency : min(min, action->latency);
    }
  }

  XBT_DEBUG("Min of share resources %f", min);

  return min;
}

static double net_share_resources_lazy(double now)
{
  surf_action_network_CM02_t action = NULL;
  double min = -1;
  double value;

  XBT_DEBUG
      ("Before share resources, the size of modified actions set is %d",
       xbt_swag_size(net_modified_set));
  update_action_remaining(now);

  lmm_solve(network_maxmin_system);

  XBT_DEBUG
      ("After share resources, The size of modified actions set is %d",
       xbt_swag_size(net_modified_set));

  xbt_swag_foreach(action, net_modified_set) {
    int max_dur_flag = 0;

    if (GENERIC_ACTION(action).state_set !=
	surf_network_model->states.running_action_set) {
      continue;
    }

    /* bogus priority, skip it */
    if (GENERIC_ACTION(action).priority <= 0) {
      continue;
    }

    min = -1;
    value = lmm_variable_getvalue(action->variable);
    if (value > 0) {
      if (GENERIC_ACTION(action).remains > 0) {
	value = GENERIC_ACTION(action).remains / value;
	min = now + value;
      } else {
	value = 0.0;
	min = now;
      }
    }

    if ((GENERIC_ACTION(action).max_duration != NO_MAX_DURATION)
	&& (min == -1
	    || GENERIC_ACTION(action).start +
	    GENERIC_ACTION(action).max_duration < min)) {
      min = GENERIC_ACTION(action).start +
	  GENERIC_ACTION(action).max_duration;
      max_dur_flag = 1;
    }

    XBT_DEBUG("Action(%p) Start %lf Finish %lf Max_duration %lf", action,
	      GENERIC_ACTION(action).start, now + value,
	      GENERIC_ACTION(action).max_duration);

    if (action->index_heap >= 0) {
      heap_remove((surf_action_network_CM02_t) action);
    }

    if (min != -1) {
      heap_insert((surf_action_network_CM02_t) action, min,
		  max_dur_flag ? MAX_DURATION : NORMAL);
      XBT_DEBUG("Insert at heap action(%p) min %lf now %lf", action, min,
		now);
    }
  }

  //hereafter must have already the min value for this resource model
  if (xbt_heap_size(net_action_heap) > 0) {
    min = xbt_heap_maxkey(net_action_heap) - now;
  } else {
    min = -1;
  }

  XBT_DEBUG("The minimum with the HEAP %lf", min);

  return min;
}

static void net_update_actions_state_full(double now, double delta)
{
  double deltap = 0.0;
  surf_action_network_CM02_t action = NULL;
  surf_action_network_CM02_t next_action = NULL;
  xbt_swag_t running_actions =
      surf_network_model->states.running_action_set;
  /*
     xbt_swag_t failed_actions =
     surf_network_model->states.failed_action_set;
   */

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
      if ((action->latency == 0.0) && !(action->suspended))
	lmm_update_variable_weight(network_maxmin_system, action->variable,
				   action->weight);
    }
#ifdef HAVE_TRACING
    if (TRACE_is_enabled()) {
      xbt_dynar_t route = NULL;
      routing_get_route_and_latency(action->src_name, action->dst_name,
				    &route, NULL);
      link_CM02_t link;
      unsigned int i;
      xbt_dynar_foreach(route, i, link) {
	TRACE_surf_link_set_utilization(link->lmm_resource.
					generic_resource.name,
					action->generic_action.data,
					(surf_action_t) action,
					lmm_variable_getvalue(action->
							      variable),
					now - delta, delta);
      }
    }
#endif
    if (!lmm_get_number_of_cnst_from_var
	(network_maxmin_system, action->variable)) {
      /* There is actually no link used, hence an infinite bandwidth.
       * This happens often when using models like vivaldi.
       * In such case, just make sure that the action completes immediately.
       */
      double_update(&(action->generic_action.remains),
		    action->generic_action.remains);
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

      if (gap_remove)
	gap_remove(action);
    } else if ((action->generic_action.max_duration != NO_MAX_DURATION)
	       && (action->generic_action.max_duration <= 0)) {
      action->generic_action.finish = surf_get_clock();
      surf_network_model->action_state_set((surf_action_t) action,
					   SURF_ACTION_DONE);
      if (gap_remove)
	gap_remove(action);
    }
  }

  return;
}

static void net_update_actions_state_lazy(double now, double delta)
{
  surf_action_network_CM02_t action = NULL;

  while ((xbt_heap_size(net_action_heap) > 0)
	 && (double_equals(xbt_heap_maxkey(net_action_heap), now))) {
    action = xbt_heap_pop(net_action_heap);
    XBT_DEBUG("Action %p: finish", action);
    GENERIC_ACTION(action).finish = surf_get_clock();

    // if I am wearing a latency hat
    if (action->hat == LATENCY) {
      lmm_update_variable_weight(network_maxmin_system, action->variable,
				 action->weight);
      heap_remove(action);
      action->last_update = surf_get_clock();

      // if I am wearing a max_duration or normal hat
    } else if (action->hat == MAX_DURATION || action->hat == NORMAL) {
      // no need to communicate anymore
      // assume that flows that reached max_duration have remaining of 0
      GENERIC_ACTION(action).remains = 0;
      action->generic_action.finish = surf_get_clock();
      surf_network_model->action_state_set((surf_action_t) action,
					   SURF_ACTION_DONE);
      heap_remove(action);
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
	(nw_link->lmm_resource.power.peak *
	 nw_link->lmm_resource.power.scale);
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
    TRACE_surf_link_set_bandwidth(date,
				  (char
				   *) (((nw_link->lmm_resource).
					generic_resource).name),
				  sg_bandwidth_factor *
				  (nw_link->lmm_resource.power.peak *
				   nw_link->lmm_resource.power.scale));
#endif
    if (sg_weight_S_parameter > 0) {
      while ((var = lmm_get_var_from_cnst
	      (network_maxmin_system, nw_link->lmm_resource.constraint,
	       &elem))) {
	action = lmm_variable_id(var);
	action->weight += delta;
	if (!(action->suspended))
	  lmm_update_variable_weight(network_maxmin_system,
				     action->variable, action->weight);
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
				  sg_tcp_gamma / (2.0 *
						  action->lat_current));
      else {
	lmm_update_variable_bound(network_maxmin_system, action->variable,
				  min(action->rate,
				      sg_tcp_gamma / (2.0 *
						      action->
						      lat_current)));

	if (action->rate < sg_tcp_gamma / (2.0 * action->lat_current)) {
	  XBT_INFO("Flow is limited BYBANDWIDTH");
	} else {
	  XBT_INFO("Flow is limited BYLATENCY, latency of flow is %f",
		   action->lat_current);
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
    XBT_CRITICAL("Unknown event ! \n");
    xbt_abort();
  }

  XBT_DEBUG
      ("There were a resource state event, need to update actions related to the constraint (%p)",
       nw_link->lmm_resource.constraint);
  return;
}


static surf_action_t net_communicate(const char *src_name,
				     const char *dst_name, double size,
				     double rate)
{
  unsigned int i;
  link_CM02_t link;
  int failed = 0;
  surf_action_network_CM02_t action = NULL;
  double bandwidth_bound;
  double latency = 0.0;
  xbt_dynar_t back_route = NULL;
  int constraints_per_variable = 0;

  xbt_dynar_t route = xbt_dynar_new(global_routing->size_of_link, NULL);

  XBT_IN("(%s,%s,%g,%g)", src_name, dst_name, size, rate);

  routing_get_route_and_latency(src_name, dst_name, &route, &latency);
  xbt_assert(!xbt_dynar_is_empty(route) || latency,
	     "You're trying to send data from %s to %s but there is no connection at all between these two hosts.",
	     src_name, dst_name);

  xbt_dynar_foreach(route, i, link) {
    if (link->lmm_resource.state_current == SURF_RESOURCE_OFF) {
      failed = 1;
      break;
    }
  }
  if (sg_network_crosstraffic == 1) {
    routing_get_route_and_latency(dst_name, src_name, &back_route, NULL);
    xbt_dynar_foreach(back_route, i, link) {
      if (link->lmm_resource.state_current == SURF_RESOURCE_OFF) {
	failed = 1;
	break;
      }
    }
  }

  action =
      surf_action_new(sizeof(s_surf_action_network_CM02_t), size,
		      surf_network_model, failed);
#ifdef HAVE_LATENCY_BOUND_TRACKING
  (action->generic_action).latency_limited = 0;
#endif
  action->weight = action->latency = latency;

  xbt_swag_insert(action, action->generic_action.state_set);
  action->rate = rate;
  if (network_update_mechanism == UM_LAZY) {
    action->index_heap = -1;
    action->last_update = surf_get_clock();
  }

  bandwidth_bound = -1.0;
  if (sg_weight_S_parameter > 0) {
    xbt_dynar_foreach(route, i, link) {
      action->weight +=
	  sg_weight_S_parameter /
	  (link->lmm_resource.power.peak * link->lmm_resource.power.scale);
    }
  }
  xbt_dynar_foreach(route, i, link) {
    double bb = bandwidth_factor_callback(size) *
	(link->lmm_resource.power.peak * link->lmm_resource.power.scale);
    bandwidth_bound =
	(bandwidth_bound < 0.0) ? bb : min(bandwidth_bound, bb);
  }

  action->lat_current = action->latency;
  action->latency *= latency_factor_callback(size);
  action->rate =
      bandwidth_constraint_callback(action->rate, bandwidth_bound, size);
  if (gap_append) {
    xbt_assert(!xbt_dynar_is_empty(route),
	       "Using a model with a gap (e.g., SMPI) with a platform without links (e.g. vivaldi)!!!");

    link = *(link_CM02_t *) xbt_dynar_get_ptr(route, 0);
    gap_append(size, link, action);
    XBT_DEBUG("Comm %p: %s -> %s gap=%f (lat=%f)",
	      action, src_name, dst_name, action->sender.gap,
	      action->latency);
  }

  constraints_per_variable = xbt_dynar_length(route);
  if (back_route != NULL)
    constraints_per_variable += xbt_dynar_length(back_route);

  if (action->latency > 0) {
    action->variable =
	lmm_variable_new(network_maxmin_system, action, 0.0, -1.0,
			 constraints_per_variable);
    if (network_update_mechanism == UM_LAZY) {
      // add to the heap the event when the latency is payed
      XBT_DEBUG("Added action (%p) one latency event at date %f", action,
		action->latency + action->last_update);
      heap_insert(action, action->latency + action->last_update,
		  xbt_dynar_is_empty(route) ? NORMAL : LATENCY);
    }
  } else
    action->variable =
	lmm_variable_new(network_maxmin_system, action, 1.0, -1.0,
			 constraints_per_variable);

  if (action->rate < 0) {
    lmm_update_variable_bound(network_maxmin_system, action->variable,
			      (action->lat_current > 0) ?
			      sg_tcp_gamma / (2.0 *
					      action->lat_current) : -1.0);
  } else {
    lmm_update_variable_bound(network_maxmin_system, action->variable,
			      (action->lat_current > 0) ?
			      min(action->rate,
				  sg_tcp_gamma / (2.0 *
						  action->lat_current))
			      : action->rate);
  }

  xbt_dynar_foreach(route, i, link) {
    lmm_expand(network_maxmin_system, link->lmm_resource.constraint,
	       action->variable, 1.0);
  }

  if (sg_network_crosstraffic == 1) {
    XBT_DEBUG("Fullduplex active adding backward flow using 5%%");
    xbt_dynar_foreach(back_route, i, link) {
      lmm_expand(network_maxmin_system, link->lmm_resource.constraint,
		 action->variable, .05);
    }
  }
#ifdef HAVE_TRACING
  if (TRACE_is_enabled()) {
    action->src_name = xbt_strdup(src_name);
    action->dst_name = xbt_strdup(dst_name);
  } else {
    action->src_name = action->dst_name = NULL;
  }
#endif

  xbt_dynar_free(&route);
  XBT_OUT();

  return (surf_action_t) action;
}

static xbt_dynar_t net_get_route(const char *src, const char *dst)
{
  xbt_dynar_t route = NULL;
  routing_get_route_and_latency(src, dst, &route, NULL);
  return route;
}

static double net_get_link_bandwidth(const void *link)
{
  surf_resource_lmm_t lmm = (surf_resource_lmm_t) link;
  return lmm->power.peak * lmm->power.scale;
}

static double net_get_link_latency(const void *link)
{
  return ((link_CM02_t) link)->lat_current;
}

static int net_link_shared(const void *link)
{
  return
      lmm_constraint_is_shared(((surf_resource_lmm_t) link)->constraint);
}

static void net_action_suspend(surf_action_t action)
{
  ((surf_action_network_CM02_t) action)->suspended = 1;
  lmm_update_variable_weight(network_maxmin_system,
			     ((surf_action_network_CM02_t)
			      action)->variable, 0.0);

  if (network_update_mechanism == UM_LAZY)	// remove action from the heap
    heap_remove((surf_action_network_CM02_t) action);
}

static void net_action_resume(surf_action_t action)
{
  if (((surf_action_network_CM02_t) action)->suspended) {
    lmm_update_variable_weight(network_maxmin_system,
			       ((surf_action_network_CM02_t)
				action)->variable,
			       ((surf_action_network_CM02_t)
				action)->weight);
    ((surf_action_network_CM02_t) action)->suspended = 0;
    if (network_update_mechanism == UM_LAZY)	// remove action from the heap
      heap_remove((surf_action_network_CM02_t) action);
  }
}

static int net_action_is_suspended(surf_action_t action)
{
  return ((surf_action_network_CM02_t) action)->suspended;
}

void net_action_set_max_duration(surf_action_t action, double duration)
{
  action->max_duration = duration;
  if (network_update_mechanism == UM_LAZY)	// remove action from the heap
    heap_remove((surf_action_network_CM02_t) action);
}

#ifdef HAVE_TRACING
static void net_action_set_category(surf_action_t action,
				    const char *category)
{
  action->category = xbt_strdup(category);
}
#endif

static void net_finalize(void)
{
  surf_model_exit(surf_network_model);
  surf_network_model = NULL;

  lmm_system_free(network_maxmin_system);
  network_maxmin_system = NULL;

  if (network_update_mechanism == UM_LAZY) {
    xbt_heap_free(net_action_heap);
    xbt_swag_free(net_modified_set);
  }

  if (smpi_bw_factor)
    xbt_dynar_free(&smpi_bw_factor);
  if (smpi_lat_factor)
    xbt_dynar_free(&smpi_lat_factor);
}

static void smpi_gap_append(double size, const link_CM02_t link,
			    surf_action_network_CM02_t action)
{
  const char *src = link->lmm_resource.generic_resource.name;
  xbt_fifo_t fifo;
  surf_action_network_CM02_t last_action;
  double bw;

  if (sg_sender_gap > 0.0) {
    if (!gap_lookup) {
      gap_lookup = xbt_dict_new();
    }
    fifo = (xbt_fifo_t) xbt_dict_get_or_null(gap_lookup, src);
    action->sender.gap = 0.0;
    if (fifo && xbt_fifo_size(fifo) > 0) {
      /* Compute gap from last send */
      last_action =
	  (surf_action_network_CM02_t)
	  xbt_fifo_get_item_content(xbt_fifo_get_last_item(fifo));
      bw = net_get_link_bandwidth(link);
      action->sender.gap =
	  last_action->sender.gap + max(sg_sender_gap,
					last_action->sender.size / bw);
      action->latency += action->sender.gap;
    }
    /* Append action as last send */
    action->sender.link_name = link->lmm_resource.generic_resource.name;
    fifo =
	(xbt_fifo_t) xbt_dict_get_or_null(gap_lookup,
					  action->sender.link_name);
    if (!fifo) {
      fifo = xbt_fifo_new();
      xbt_dict_set(gap_lookup, action->sender.link_name, fifo, NULL);
    }
    action->sender.fifo_item = xbt_fifo_push(fifo, action);
    action->sender.size = size;
  }
}

static void smpi_gap_remove(surf_action_network_CM02_t action)
{
  xbt_fifo_t fifo;
  size_t size;

  if (sg_sender_gap > 0.0 && action->sender.link_name
      && action->sender.fifo_item) {
    fifo =
	(xbt_fifo_t) xbt_dict_get_or_null(gap_lookup,
					  action->sender.link_name);
    xbt_fifo_remove_item(fifo, action->sender.fifo_item);
    size = xbt_fifo_size(fifo);
    if (size == 0) {
      xbt_fifo_free(fifo);
      xbt_dict_remove(gap_lookup, action->sender.link_name);
      size = xbt_dict_length(gap_lookup);
      if (size == 0) {
	xbt_dict_free(&gap_lookup);
      }
    }
  }
}

static void surf_network_model_init_internal(void)
{
  s_surf_action_network_CM02_t comm;
  surf_network_model = surf_model_init();

  surf_network_model->name = "network";
  surf_network_model->action_unref = net_action_unref;
  surf_network_model->action_cancel = net_action_cancel;
  surf_network_model->action_recycle = net_action_recycle;
  surf_network_model->get_remains = net_action_get_remains;
#ifdef HAVE_LATENCY_BOUND_TRACKING
  surf_network_model->get_latency_limited = net_get_link_latency_limited;
#endif
#ifdef HAVE_TRACING
  surf_network_model->set_category = net_action_set_category;
#endif

  surf_network_model->model_private->resource_used = net_resource_used;
  if (network_update_mechanism == UM_LAZY) {
    surf_network_model->model_private->share_resources =
	net_share_resources_lazy;
    surf_network_model->model_private->update_actions_state =
	net_update_actions_state_lazy;
  } else if (network_update_mechanism == UM_FULL) {
    surf_network_model->model_private->share_resources =
	net_share_resources_full;
    surf_network_model->model_private->update_actions_state =
	net_update_actions_state_full;
  }

  surf_network_model->model_private->update_resource_state =
      net_update_resource_state;
  surf_network_model->model_private->finalize = net_finalize;

  surf_network_model->suspend = net_action_suspend;
  surf_network_model->resume = net_action_resume;
  surf_network_model->is_suspended = net_action_is_suspended;
  surf_cpu_model->set_max_duration = net_action_set_max_duration;

  surf_network_model->extension.network.communicate = net_communicate;
  surf_network_model->extension.network.get_route = net_get_route;
  surf_network_model->extension.network.get_link_bandwidth =
      net_get_link_bandwidth;
  surf_network_model->extension.network.get_link_latency =
      net_get_link_latency;
  surf_network_model->extension.network.link_shared = net_link_shared;
  surf_network_model->extension.network.add_traces = net_add_traces;
  surf_network_model->extension.network.create_resource =
      net_create_resource;

  if (!network_maxmin_system)
    network_maxmin_system = lmm_system_new(net_selective_update);

  routing_model_create(sizeof(link_CM02_t),
		       net_create_resource("__loopback__",
					   498000000, NULL, 0.000015, NULL,
					   SURF_RESOURCE_ON, NULL,
					   SURF_LINK_FATPIPE, NULL));

  if (network_update_mechanism == UM_LAZY) {
    net_action_heap = xbt_heap_new(8, NULL);
    xbt_heap_set_update_callback(net_action_heap,
				 net_action_update_index_heap);
    net_modified_set =
	xbt_swag_new(xbt_swag_offset(comm, action_list_hookup));
    network_maxmin_system->keep_track = net_modified_set;
  }
}

static void set_update_mechanism(void)
{
#ifdef HAVE_TRACING
  TRACE_set_network_update_mechanism();
#endif

  char *optim = xbt_cfg_get_string(_surf_cfg_set, "network/optim");
  int select =
      xbt_cfg_get_int(_surf_cfg_set, "network/maxmin_selective_update");

  if (!strcmp(optim, "Full")) {
    network_update_mechanism = UM_FULL;
    net_selective_update = select;
  } else if (!strcmp(optim, "Lazy")) {
    network_update_mechanism = UM_LAZY;
    net_selective_update = 1;
    xbt_assert((select == 1)
	       ||
	       (xbt_cfg_is_default_value
		(_surf_cfg_set, "network/maxmin_selective_update")),
	       "Disabling selective update while using the lazy update mechanism is dumb!");
  } else {
    xbt_die("Unsupported optimization (%s) for this model", optim);
  }
}

/************************************************************************/
/* New model based on LV08 and experimental results of MPI ping-pongs   */
/************************************************************************/
/* @Inproceedings{smpi_ipdps, */
/*  author={Pierre-Nicolas Clauss and Mark Stillwell and Stéphane Genaud and Frédéric Suter and Henri Casanova and Martin Quinson}, */
/*  title={Single Node On-Line Simulation of {MPI} Applications with SMPI}, */
/*  booktitle={25th IEEE International Parallel and Distributed Processing Symposium (IPDPS'11)}, */
/*  address={Anchorage (Alaska) USA}, */
/*  month=may, */
/*  year={2011} */
/*  } */
void surf_network_model_init_SMPI(void)
{

  if (surf_network_model)
    return;
  set_update_mechanism();

  surf_network_model_init_internal();
  latency_factor_callback = &smpi_latency_factor;
  bandwidth_factor_callback = &smpi_bandwidth_factor;
  bandwidth_constraint_callback = &smpi_bandwidth_constraint;
  gap_append = &smpi_gap_append;
  gap_remove = &smpi_gap_remove;
  net_define_callbacks();
  xbt_dynar_push(model_list, &surf_network_model);
  network_solve = lmm_solve;

  xbt_cfg_setdefault_double(_surf_cfg_set, "network/sender_gap", 10e-6);
  xbt_cfg_setdefault_double(_surf_cfg_set, "network/weight_S", 8775);
}

/************************************************************************/
/* New model based on optimizations discussed during Pedro Velho's thesis*/
/************************************************************************/
/* @techreport{VELHO:2011:HAL-00646896:1, */
/*      url = {http://hal.inria.fr/hal-00646896/en/}, */
/*      title = {{Flow-level network models: have we reached the limits?}}, */
/*      author = {Velho, Pedro and Schnorr, Lucas and Casanova, Henri and Legrand, Arnaud}, */
/*      type = {Rapport de recherche}, */
/*      institution = {INRIA}, */
/*      number = {RR-7821}, */
/*      year = {2011}, */
/*      month = Nov, */
/*      pdf = {http://hal.inria.fr/hal-00646896/PDF/rr-validity.pdf}, */
/*  } */
void surf_network_model_init_LegrandVelho(void)
{
  if (surf_network_model)
    return;

  set_update_mechanism();

  surf_network_model_init_internal();
  net_define_callbacks();
  xbt_dynar_push(model_list, &surf_network_model);
  network_solve = lmm_solve;

  xbt_cfg_setdefault_double(_surf_cfg_set, "network/latency_factor",
			    13.01);
  xbt_cfg_setdefault_double(_surf_cfg_set, "network/bandwidth_factor",
			    0.97);
  xbt_cfg_setdefault_double(_surf_cfg_set, "network/weight_S", 20537);
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
void surf_network_model_init_CM02(void)
{

  if (surf_network_model)
    return;

  set_update_mechanism();
  surf_network_model_init_internal();
  net_define_callbacks();
  xbt_dynar_push(model_list, &surf_network_model);
  network_solve = lmm_solve;

  xbt_cfg_setdefault_double(_surf_cfg_set, "network/latency_factor", 1.0);
  xbt_cfg_setdefault_double(_surf_cfg_set, "network/bandwidth_factor",
			    1.0);
  xbt_cfg_setdefault_double(_surf_cfg_set, "network/weight_S", 0.0);
}

/***************************************************************************/
/* The models from Steven H. Low                                           */
/***************************************************************************/
/* @article{Low03,                                                         */
/*   author={Steven H. Low},                                               */
/*   title={A Duality Model of {TCP} and Queue Management Algorithms},     */
/*   year={2003},                                                          */
/*   journal={{IEEE/ACM} Transactions on Networking},                      */
/*    volume={11}, number={4},                                             */
/*  }                                                                      */
void surf_network_model_init_Reno(void)
{
  if (surf_network_model)
    return;

  set_update_mechanism();
  surf_network_model_init_internal();
  net_define_callbacks();

  xbt_dynar_push(model_list, &surf_network_model);
  lmm_set_default_protocol_function(func_reno_f, func_reno_fp,
				    func_reno_fpi);
  network_solve = lagrange_solve;

  xbt_cfg_setdefault_double(_surf_cfg_set, "network/latency_factor", 10.4);
  xbt_cfg_setdefault_double(_surf_cfg_set, "network/bandwidth_factor",
			    0.92);
  xbt_cfg_setdefault_double(_surf_cfg_set, "network/weight_S", 8775);
}


void surf_network_model_init_Reno2(void)
{
  if (surf_network_model)
    return;

  set_update_mechanism();
  surf_network_model_init_internal();
  net_define_callbacks();

  xbt_dynar_push(model_list, &surf_network_model);
  lmm_set_default_protocol_function(func_reno2_f, func_reno2_fp,
				    func_reno2_fpi);
  network_solve = lagrange_solve;

  xbt_cfg_setdefault_double(_surf_cfg_set, "network/latency_factor", 10.4);
  xbt_cfg_setdefault_double(_surf_cfg_set, "network/bandwidth_factor",
			    0.92);
  xbt_cfg_setdefault_double(_surf_cfg_set, "network/weight_S_parameter",
			    8775);
}

void surf_network_model_init_Vegas(void)
{
  if (surf_network_model)
    return;

  set_update_mechanism();
  surf_network_model_init_internal();
  net_define_callbacks();

  xbt_dynar_push(model_list, &surf_network_model);
  lmm_set_default_protocol_function(func_vegas_f, func_vegas_fp,
				    func_vegas_fpi);
  network_solve = lagrange_solve;

  xbt_cfg_setdefault_double(_surf_cfg_set, "network/latency_factor", 10.4);
  xbt_cfg_setdefault_double(_surf_cfg_set, "network/bandwidth_factor",
			    0.92);
  xbt_cfg_setdefault_double(_surf_cfg_set, "network/weight_S", 8775);
}
