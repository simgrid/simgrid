
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

#include "xbt/log.h"
#include "xbt/str.h"
#include "surf_private.h"
#include "xbt/dict.h"
#include "maxmin_private.h"
#include "surf/surf_resource_lmm.h"


#undef GENERIC_ACTION
#define GENERIC_ACTION(action) action->generic_action


XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_network_im, surf,
                                "Logging specific to the SURF network module");


enum heap_action_type{
  LATENCY = 100,
  MAX_DURATION,
  NORMAL,
  NOTSET
};

typedef struct surf_action_network_CM02_im {
  s_surf_action_t generic_action;
  s_xbt_swag_hookup_t action_list_hookup;
  double latency;
  double lat_current;
  double weight;
  lmm_variable_t variable;
  double rate;
#ifdef HAVE_LATENCY_BOUND_TRACKING
  int latency_limited;
#endif
  int suspended;
#ifdef HAVE_TRACING
  char *src_name;
  char *dst_name;
#endif
  int index_heap;
  enum heap_action_type hat;
  double last_update;
} s_surf_action_network_CM02_im_t, *surf_action_network_CM02_im_t;


typedef struct network_link_CM02_im {
  s_surf_resource_lmm_t lmm_resource;   /* must remain first to be added to a trace */

  /* Using this object with the public part of
     model does not make sense */
  double lat_current;
  tmgr_trace_event_t lat_event;
} s_link_CM02_im_t, *link_CM02_im_t;








extern surf_model_t surf_network_model;
static lmm_system_t network_im_maxmin_system = NULL;
static void (*network_im_solve) (lmm_system_t) = NULL;

extern double sg_latency_factor;
extern double sg_bandwidth_factor;
extern double sg_weight_S_parameter;

extern double sg_tcp_gamma;
extern int sg_network_fullduplex;


static void im_net_action_recycle(surf_action_t action);
static int im_net_get_link_latency_limited(surf_action_t action);
static int im_net_action_is_suspended(surf_action_t action);
static double im_net_action_get_remains(surf_action_t action);
static void im_net_action_set_max_duration(surf_action_t action, double duration);
static void im_net_update_actions_state(double now, double delta);
static void update_action_remaining(double now);

static xbt_swag_t im_net_modified_set = NULL;
static xbt_heap_t im_net_action_heap = NULL;
xbt_swag_t keep_track = NULL;
extern int sg_maxmin_selective_update;

/* added to manage the communication action's heap */
static void im_net_action_update_index_heap(void *action, int i)
{
  ((surf_action_network_CM02_im_t) action)->index_heap = i;
}

/* insert action on heap using a given key and a hat (heap_action_type)
 * a hat can be of three types for communications:
 *
 * NORMAL = this is a normal heap entry stating the date to finish transmitting
 * LATENCY = this is a heap entry to warn us when the latency is payed
 * MAX_DURATION =this is a heap entry to warn us when the max_duration limit is reached
 */
static void heap_insert(surf_action_network_CM02_im_t    action, double key, enum heap_action_type hat){
  action->hat = hat;
  xbt_heap_push(im_net_action_heap, action, key);
}

static void heap_remove(surf_action_network_CM02_im_t action){
  action->hat = NONE;
  if(((surf_action_network_CM02_im_t) action)->index_heap >= 0){
      xbt_heap_remove(im_net_action_heap,action->index_heap);
  }
}

/******************************************************************************/
/*                           Factors callbacks                                */
/******************************************************************************/
static double im_constant_latency_factor(double size)
{
  return sg_latency_factor;
}

static double im_constant_bandwidth_factor(double size)
{
  return sg_bandwidth_factor;
}

static double im_constant_bandwidth_constraint(double rate, double bound,
                                            double size)
{
  return rate;
}


static double (*im_latency_factor_callback) (double) =
    &im_constant_latency_factor;
static double (*im_bandwidth_factor_callback) (double) =
    &im_constant_bandwidth_factor;
static double (*im_bandwidth_constraint_callback) (double, double, double) =
    &im_constant_bandwidth_constraint;


static void* im_net_create_resource(const char *name,
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
  link_CM02_im_t nw_link = (link_CM02_im_t)
      surf_resource_lmm_new(sizeof(s_link_CM02_im_t),
                            surf_network_model, name, properties,
                            network_im_maxmin_system,
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

static void im_net_parse_link_init(sg_platf_link_cbarg_t link)
{
  if(link->policy == SURF_LINK_FULLDUPLEX)
  {
    char *name = bprintf("%s_UP",link->id);
	  im_net_create_resource(name, link->bandwidth, link->bandwidth_trace,
	               link->latency, link->latency_trace, link->state, link->state_trace,
	               link->policy, link->properties);
	  xbt_free(name);
	  name = bprintf("%s_DOWN",link->id);
	  im_net_create_resource(name, link->bandwidth, link->bandwidth_trace,
            link->latency, link->latency_trace, link->state, link->state_trace,
            link->policy, NULL); // FIXME: We need to deep copy the properties or we won't be able to free it
	  xbt_free(name);
  }
  else
  {
	  im_net_create_resource(link->id, link->bandwidth, link->bandwidth_trace,
    		link->latency, link->latency_trace, link->state, link->state_trace,
	               link->policy, link->properties);
  }
}

static void im_net_add_traces(void)
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
    link_CM02_im_t link =
    		xbt_lib_get_or_null(link_lib, elm, SURF_LINK_LEVEL);

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
    link_CM02_im_t link =
    		xbt_lib_get_or_null(link_lib, elm, SURF_LINK_LEVEL);

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
    link_CM02_im_t link =
    		xbt_lib_get_or_null(link_lib, elm, SURF_LINK_LEVEL);

    xbt_assert(link, "Cannot connect trace %s to link %s: link undefined",
                trace_name, elm);
    xbt_assert(trace,
                "Cannot connect trace %s to link %s: trace undefined",
                trace_name, elm);

    link->lat_event = tmgr_history_add_trace(history, trace, 0.0, 0, link);
  }
}

static void im_net_define_callbacks(void)
{
  /* Figuring out the network links */
  sg_platf_link_add_cb(im_net_parse_link_init);
  sg_platf_postparse_add_cb(im_net_add_traces);
}

static int im_net_resource_used(void *resource_id)
{
  return lmm_constraint_used(network_im_maxmin_system,
                             ((surf_resource_lmm_t)
                              resource_id)->constraint);
}

static int im_net_action_unref(surf_action_t action)
{
  action->refcount--;
  if (!action->refcount) {
    xbt_swag_remove(action, action->state_set);
    if (((surf_action_network_CM02_im_t) action)->variable){
      lmm_variable_free(network_im_maxmin_system,
                        ((surf_action_network_CM02_im_t) action)->variable);
    }
    // remove action from the heap
    heap_remove((surf_action_network_CM02_im_t) action);

    xbt_swag_remove(action, im_net_modified_set);
#ifdef HAVE_TRACING
    xbt_free(((surf_action_network_CM02_im_t) action)->src_name);
    xbt_free(((surf_action_network_CM02_im_t) action)->dst_name);
    xbt_free(action->category);
#endif
    surf_action_free(&action);
    return 1;
  }
  return 0;
}



static void im_net_action_cancel(surf_action_t action)
{
  surf_network_model->action_state_set(action, SURF_ACTION_FAILED);

  xbt_swag_remove(action, im_net_modified_set);
  // remove action from the heap
  heap_remove((surf_action_network_CM02_im_t) action);
}

static void im_net_action_recycle(surf_action_t action)
{
  return;
}

#ifdef HAVE_LATENCY_BOUND_TRACKING
static int im_net_get_link_latency_limited(surf_action_t action)
{
  return action->latency_limited;
}
#endif

static double im_net_action_get_remains(surf_action_t action)
{
  /* update remains before return it */
  update_action_remaining(surf_get_clock());
  return action->remains;
}

static void update_action_remaining(double now){
  surf_action_network_CM02_im_t action = NULL;
  double delta = 0.0;

  xbt_swag_foreach(action, im_net_modified_set) {

    if(action->suspended != 0){
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

static double im_net_share_resources(double now)
{
  surf_action_network_CM02_im_t action = NULL;
  double min=-1;
  double value;

  XBT_DEBUG("Before share resources, the size of modified actions set is %d", xbt_swag_size(im_net_modified_set));
  update_action_remaining(now);

  keep_track = im_net_modified_set;
  lmm_solve(network_im_maxmin_system);
  keep_track = NULL;

  XBT_DEBUG("After share resources, The size of modified actions set is %d", xbt_swag_size(im_net_modified_set));

   xbt_swag_foreach(action, im_net_modified_set) {
     if (GENERIC_ACTION(action).state_set != surf_network_model->states.running_action_set){
       continue;
     }

     /* bogus priority, skip it */
     if (GENERIC_ACTION(action).priority <= 0){
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
             GENERIC_ACTION(action).max_duration < min)){
       min =   GENERIC_ACTION(action).start +
               GENERIC_ACTION(action).max_duration;
     }

     XBT_DEBUG("Action(%p) Start %lf Finish %lf Max_duration %lf", action,
                GENERIC_ACTION(action).start, now + value,
                GENERIC_ACTION(action).max_duration);



     if (action->index_heap >= 0) {
         heap_remove((surf_action_network_CM02_im_t) action);
     }

     if (min != -1) {
         heap_insert((surf_action_network_CM02_im_t) action, min, NORMAL);
         XBT_DEBUG("Insert at heap action(%p) min %lf now %lf", action, min, now);
     }
   }

   //hereafter must have already the min value for this resource model
   if(xbt_heap_size(im_net_action_heap) > 0 ){
       min = xbt_heap_maxkey(im_net_action_heap) - now ;
   }else{
       min = -1;
   }

   XBT_DEBUG("The minimum with the HEAP %lf", min);


  return min;
}

static void im_net_update_actions_state(double now, double delta)
{
  surf_action_network_CM02_im_t action = NULL;

  while ((xbt_heap_size(im_net_action_heap) > 0)
         && (double_equals(xbt_heap_maxkey(im_net_action_heap), now))) {
    action = xbt_heap_pop(im_net_action_heap);
    XBT_DEBUG("Action %p: finish", action);
    GENERIC_ACTION(action).finish = surf_get_clock();

    // if I am wearing a latency heat
    if( action->hat ==  LATENCY){
        lmm_update_variable_weight(network_im_maxmin_system, action->variable,
                                           action->weight);
        heap_remove(action);
        action->last_update = surf_get_clock();

        XBT_DEBUG("Action (%p) is not limited by latency anymore", action);
#ifdef HAVE_LATENCY_BOUND_TRACKING
          GENERIC_ACTION(action).latency_limited = 0;
#endif

    // if I am wearing a max_duration or normal hat
    }else if( action->hat == MAX_DURATION || action->hat == NORMAL ){
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

static void im_net_update_resource_state(void *id,
                                      tmgr_trace_event_t event_type,
                                      double value, double date)
{
  link_CM02_im_t nw_link = id;
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
    surf_action_network_CM02_im_t action = NULL;

    nw_link->lmm_resource.power.peak = value;
    lmm_update_constraint_bound(network_im_maxmin_system,
                                nw_link->lmm_resource.constraint,
                                sg_bandwidth_factor *
                                (nw_link->lmm_resource.power.peak *
                                 nw_link->lmm_resource.power.scale));
#ifdef HAVE_TRACING
    TRACE_surf_link_set_bandwidth(date, (char *)(((nw_link->lmm_resource).generic_resource).name),
                                  sg_bandwidth_factor *
                                  (nw_link->lmm_resource.power.peak *
                                   nw_link->lmm_resource.power.scale));
#endif
    if (sg_weight_S_parameter > 0) {
      while ((var = lmm_get_var_from_cnst
              (network_im_maxmin_system, nw_link->lmm_resource.constraint,
               &elem))) {
        action = lmm_variable_id(var);
        action->weight += delta;
        if (!(action->suspended))
          lmm_update_variable_weight(network_im_maxmin_system,
                                     action->variable, action->weight);
      }
    }
    if (tmgr_trace_event_free(event_type))
      nw_link->lmm_resource.power.event = NULL;
  } else if (event_type == nw_link->lat_event) {
    double delta = value - nw_link->lat_current;
    lmm_variable_t var = NULL;
    lmm_element_t elem = NULL;
    surf_action_network_CM02_im_t action = NULL;

    nw_link->lat_current = value;
    while ((var = lmm_get_var_from_cnst
            (network_im_maxmin_system, nw_link->lmm_resource.constraint,
             &elem))) {
      action = lmm_variable_id(var);
      action->lat_current += delta;
      action->weight += delta;
      if (action->rate < 0)
        lmm_update_variable_bound(network_im_maxmin_system, action->variable,
                                  sg_tcp_gamma / (2.0 *
                                                  action->lat_current));
      else {
        lmm_update_variable_bound(network_im_maxmin_system, action->variable,
                                  min(action->rate,
                                      sg_tcp_gamma / (2.0 *
                                                      action->lat_current)));

        if (action->rate < sg_tcp_gamma / (2.0 * action->lat_current)) {
          XBT_INFO("Flow is limited BYBANDWIDTH");
        } else {
          XBT_INFO("Flow is limited BYLATENCY, latency of flow is %f",
                action->lat_current);
        }
      }
      if (!(action->suspended))
        lmm_update_variable_weight(network_im_maxmin_system, action->variable,
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
              (network_im_maxmin_system, cnst, &elem))) {
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

  XBT_DEBUG("There were a resource state event, need to update actions related to the constraint (%p)", nw_link->lmm_resource.constraint);
  return;
}


static surf_action_t im_net_communicate(const char *src_name,
                                     const char *dst_name, double size,
                                     double rate)
{
  unsigned int i;
  link_CM02_im_t link;
  int failed = 0;
  surf_action_network_CM02_im_t action = NULL;
  double bandwidth_bound;
  /* LARGE PLATFORMS HACK:
     Add a link_CM02_im_t *link and a int link_nb to network_card_CM02_im_t. It will represent local links for this node
     Use the cluster_id for ->id */

  xbt_dynar_t back_route = NULL;
  int constraints_per_variable = 0;
  // I need to have the forward and backward routes at the same time, so allocate "route". That way, the routing wont clean it up
  xbt_dynar_t route=xbt_dynar_new(global_routing->size_of_link,NULL);
  routing_get_route_and_latency(src_name, dst_name,&route,NULL);


  if (sg_network_fullduplex == 1) {
    routing_get_route_and_latency(dst_name, src_name, &back_route, NULL);
  }

  /* LARGE PLATFORMS HACK:
     total_route_size = route_size + src->link_nb + dst->nb */

  XBT_IN("(%s,%s,%g,%g)", src_name, dst_name, size, rate);
  /* LARGE PLATFORMS HACK:
     assert on total_route_size */
  xbt_assert(xbt_dynar_length(route),
              "You're trying to send data from %s to %s but there is no connection between these two hosts.",
              src_name, dst_name);

  xbt_dynar_foreach(route, i, link) {
    if (link->lmm_resource.state_current == SURF_RESOURCE_OFF) {
      failed = 1;
      break;
    }
  }
  action =
      surf_action_new(sizeof(s_surf_action_network_CM02_im_t), size,
                      surf_network_model, failed);


#ifdef HAVE_LATENCY_BOUND_TRACKING
  (action->generic_action).latency_limited = 0;
#endif

  xbt_swag_insert(action, action->generic_action.state_set);
  action->rate = rate;
  action->index_heap = -1;
  action->latency = 0.0;
  action->weight = 0.0;
  action->last_update = surf_get_clock();

  bandwidth_bound = -1.0;
  xbt_dynar_foreach(route, i, link) {
    action->latency += link->lat_current;
    action->weight +=
        link->lat_current +
        sg_weight_S_parameter /
        (link->lmm_resource.power.peak * link->lmm_resource.power.scale);
    if (bandwidth_bound < 0.0)
      bandwidth_bound =
          im_bandwidth_factor_callback(size) *
          (link->lmm_resource.power.peak * link->lmm_resource.power.scale);
    else
      bandwidth_bound =
          min(bandwidth_bound,
              im_bandwidth_factor_callback(size) *
              (link->lmm_resource.power.peak *
               link->lmm_resource.power.scale));
  }
  /* LARGE PLATFORMS HACK:
     Add src->link and dst->link latencies */
  action->lat_current = action->latency;
  action->latency *= im_latency_factor_callback(size);
  action->rate =
      im_bandwidth_constraint_callback(action->rate, bandwidth_bound,
                                        size);

  /* LARGE PLATFORMS HACK:
     lmm_variable_new(..., total_route_size) */
  if (back_route != NULL) {
    constraints_per_variable =
        xbt_dynar_length(route) + xbt_dynar_length(back_route);
  } else {
    constraints_per_variable = xbt_dynar_length(route);
  }

  if (action->latency > 0){
      action->variable =
        lmm_variable_new(network_im_maxmin_system, action, 0.0, -1.0,
                         constraints_per_variable);
    // add to the heap the event when the latency is payed
    XBT_DEBUG("Added action (%p) one latency event at date %f", action, action->latency + action->last_update);
    heap_insert(action, action->latency + action->last_update, LATENCY);
#ifdef HAVE_LATENCY_BOUND_TRACKING
        (action->generic_action).latency_limited = 1;
#endif
  }
  else
    action->variable =
        lmm_variable_new(network_im_maxmin_system, action, 1.0, -1.0,
                         constraints_per_variable);

  if (action->rate < 0) {
    if (action->lat_current > 0)
      lmm_update_variable_bound(network_im_maxmin_system, action->variable,
                                sg_tcp_gamma / (2.0 *
                                                action->lat_current));
    else
      lmm_update_variable_bound(network_im_maxmin_system, action->variable,
                                -1.0);
  } else {
    if (action->lat_current > 0)
      lmm_update_variable_bound(network_im_maxmin_system, action->variable,
                                min(action->rate,
                                    sg_tcp_gamma / (2.0 *
                                                    action->lat_current)));
    else
      lmm_update_variable_bound(network_im_maxmin_system, action->variable,
                                action->rate);
  }

  xbt_dynar_foreach(route, i, link) {
    lmm_expand(network_im_maxmin_system, link->lmm_resource.constraint,
               action->variable, 1.0);
  }

  if (sg_network_fullduplex == 1) {
    XBT_DEBUG("Fullduplex active adding backward flow using 5%c", '%');
    xbt_dynar_foreach(back_route, i, link) {
      lmm_expand(network_im_maxmin_system, link->lmm_resource.constraint,
                 action->variable, .05);
    }
  }

  /* LARGE PLATFORMS HACK:
     expand also with src->link and dst->link */
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

static xbt_dynar_t im_net_get_route(const char *src, const char *dst)
{
  xbt_dynar_t route=NULL;
  routing_get_route_and_latency(src, dst,&route,NULL);
  return route;
}

static double im_net_get_link_bandwidth(const void *link)
{
  surf_resource_lmm_t lmm = (surf_resource_lmm_t) link;
  return lmm->power.peak * lmm->power.scale;
}

static double im_net_get_link_latency(const void *link)
{
  return ((link_CM02_im_t) link)->lat_current;
}

static int im_net_link_shared(const void *link)
{
  return
      lmm_constraint_is_shared(((surf_resource_lmm_t) link)->constraint);
}

static void im_net_action_suspend(surf_action_t action)
{
  ((surf_action_network_CM02_im_t) action)->suspended = 1;
  lmm_update_variable_weight(network_im_maxmin_system,
                             ((surf_action_network_CM02_im_t)
                              action)->variable, 0.0);

  // remove action from the heap
  heap_remove((surf_action_network_CM02_im_t) action);
}

static void im_net_action_resume(surf_action_t action)
{
  if (((surf_action_network_CM02_im_t) action)->suspended) {
    lmm_update_variable_weight(network_im_maxmin_system,
                               ((surf_action_network_CM02_im_t)
                                action)->variable,
                               ((surf_action_network_CM02_im_t)
                                action)->weight);
    ((surf_action_network_CM02_im_t) action)->suspended = 0;
    // remove action from the heap
    heap_remove((surf_action_network_CM02_im_t) action);
  }
}

static int im_net_action_is_suspended(surf_action_t action)
{
  return ((surf_action_network_CM02_im_t) action)->suspended;
}

static void im_net_action_set_max_duration(surf_action_t action, double duration)
{
  action->max_duration = duration;
  // remove action from the heap
  heap_remove((surf_action_network_CM02_im_t) action);
}


static void im_net_finalize(void)
{
  surf_model_exit(surf_network_model);
  surf_network_model = NULL;

  lmm_system_free(network_im_maxmin_system);
  network_im_maxmin_system = NULL;

  xbt_heap_free(im_net_action_heap);
  xbt_swag_free(im_net_modified_set);

}

static void im_surf_network_model_init_internal(void)
{
  s_surf_action_network_CM02_im_t comm;

  surf_network_model = surf_model_init();

  surf_network_model->name = "network";
  surf_network_model->action_unref = im_net_action_unref;
  surf_network_model->action_cancel = im_net_action_cancel;
  surf_network_model->action_recycle = im_net_action_recycle;
  surf_network_model->get_remains = im_net_action_get_remains;
#ifdef HAVE_LATENCY_BOUND_TRACKING
  surf_network_model->get_latency_limited = im_net_get_link_latency_limited;
#endif

  surf_network_model->model_private->resource_used = im_net_resource_used;
  surf_network_model->model_private->share_resources = im_net_share_resources;
  surf_network_model->model_private->update_actions_state =
		  im_net_update_actions_state;
  surf_network_model->model_private->update_resource_state =
		  im_net_update_resource_state;
  surf_network_model->model_private->finalize = im_net_finalize;

  surf_network_model->suspend = im_net_action_suspend;
  surf_network_model->resume = im_net_action_resume;
  surf_network_model->is_suspended = im_net_action_is_suspended;
  surf_cpu_model->set_max_duration = im_net_action_set_max_duration;

  surf_network_model->extension.network.communicate = im_net_communicate;
  surf_network_model->extension.network.get_route = im_net_get_route;
  surf_network_model->extension.network.get_link_bandwidth =
		  im_net_get_link_bandwidth;
  surf_network_model->extension.network.get_link_latency =
		  im_net_get_link_latency;
  surf_network_model->extension.network.link_shared = im_net_link_shared;
  surf_network_model->extension.network.add_traces = im_net_add_traces;
  surf_network_model->extension.network.create_resource =
		  im_net_create_resource;


  if (!network_im_maxmin_system){
	sg_maxmin_selective_update = 1;
    network_im_maxmin_system = lmm_system_new();
  }
  im_net_action_heap = xbt_heap_new(8,NULL);

  xbt_heap_set_update_callback(im_net_action_heap, im_net_action_update_index_heap);

  routing_model_create(sizeof(link_CM02_im_t),
      im_net_create_resource("__loopback__",
          498000000, NULL, 0.000015, NULL,
          SURF_RESOURCE_ON, NULL,
          SURF_LINK_FATPIPE, NULL));
  im_net_modified_set =
      xbt_swag_new(xbt_swag_offset(comm, action_list_hookup));
}



/************************************************************************/
/* New model based on optimizations discussed during this thesis        */
/************************************************************************/
void im_surf_network_model_init_LegrandVelho(void)
{

  if (surf_network_model)
    return;
  im_surf_network_model_init_internal();
  im_net_define_callbacks();
  xbt_dynar_push(model_list, &surf_network_model);
  network_im_solve = lmm_solve;

  xbt_cfg_setdefault_double(_surf_cfg_set, "network/latency_factor", 10.4);
  xbt_cfg_setdefault_double(_surf_cfg_set, "network/bandwidth_factor",
                            0.92);
  xbt_cfg_setdefault_double(_surf_cfg_set, "network/weight_S", 8775);
}



