/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

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
xbt_dict_t link_set = NULL;

double latency_factor = 1.0;    /* default value */
double bandwidth_factor = 1.0;  /* default value */
double weight_S_parameter = 0.0;        /* default value */

int card_number = 0;
int host_number = 0;
link_CM02_t **routing_table = NULL;
int *routing_table_size = NULL;
static link_CM02_t loopback = NULL;
double sg_tcp_gamma = 0.0;


static void create_routing_table(void)
{
  routing_table = xbt_new0(link_CM02_t *,       /*card_number * card_number */
                           host_number * host_number);
  routing_table_size =
    xbt_new0(int, /*card_number * card_number */ host_number * host_number);
}

static void link_free(void *nw_link)
{
  xbt_dict_free(&(((link_CM02_t) nw_link)->properties));
  surf_resource_free(nw_link);
}

static link_CM02_t link_new(char *name,
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
  link_CM02_t nw_link = xbt_new0(s_link_CM02_t, 1);
  xbt_assert1(!xbt_dict_get_or_null(link_set, name),
              "Link '%s' declared several times in the platform file.", name);

  nw_link->generic_resource.model = surf_network_model;
  nw_link->generic_resource.name = name;
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
                       bandwidth_factor * nw_link->bw_current);

  if (policy == SURF_LINK_FATPIPE)
    lmm_constraint_shared(nw_link->constraint);

  nw_link->properties = properties;

  current_property_set = properties;

  xbt_dict_set(link_set, name, nw_link, link_free);

  return nw_link;
}

static int network_card_new(const char *card_name)
{
  network_card_CM02_t card =
    surf_model_resource_by_name(surf_network_model, card_name);

  if (!card) {
    card = xbt_new0(s_network_card_CM02_t, 1);
    card->generic_resource.name = xbt_strdup(card_name);
    card->id = host_number++;
    xbt_dict_set(surf_model_resource_set(surf_network_model), card_name, card,
                 surf_resource_free);
  }
  return card->id;
}

static void route_new(int src_id, int dst_id,
                      link_CM02_t * link_list, int nb_link)
{
  ROUTE_SIZE(src_id, dst_id) = nb_link;
  ROUTE(src_id, dst_id) = link_list =
    xbt_realloc(link_list, sizeof(link_CM02_t) * nb_link);
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

  xbt_assert0((A_surfxml_link_state == A_surfxml_link_state_ON)
              || (A_surfxml_link_state ==
                  A_surfxml_link_state_OFF), "Invalid state");
  if (A_surfxml_link_state == A_surfxml_link_state_ON)
    state_initial_link = SURF_LINK_ON;
  else if (A_surfxml_link_state == A_surfxml_link_state_OFF)
    state_initial_link = SURF_LINK_OFF;

  if (A_surfxml_link_sharing_policy == A_surfxml_link_sharing_policy_SHARED)
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
}

static void parse_route_set_route(void)
{
  char *name;
  if (src_id != -1 && dst_id != -1) {
    name = bprintf("%x#%x", src_id, dst_id);
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
                            SURF_LINK_ON, NULL, SURF_LINK_FATPIPE, NULL);
      ROUTE_SIZE(i, i) = 1;
      ROUTE(i, i) = xbt_new0(link_CM02_t, 1);
      ROUTE(i, i)[0] = loopback;
    }
}

static void add_route(void)
{
  xbt_ex_t e;
  int nb_link = 0;
  unsigned int cpt = 0;
  int link_list_capacity = 0;
  link_CM02_t *link_list = NULL;
  xbt_dict_cursor_t cursor = NULL;
  char *key, *data, *end;
  const char *sep = "#";
  xbt_dynar_t links, keys;

  if (routing_table == NULL)
    create_routing_table();

  xbt_dict_foreach(route_table, cursor, key, data) {
    char *link = NULL;
    nb_link = 0;
    links = (xbt_dynar_t) data;
    keys = xbt_str_split_str(key, sep);

    link_list_capacity = xbt_dynar_length(links);
    link_list = xbt_new(link_CM02_t, link_list_capacity);

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
    route_new(src_id, dst_id, link_list, nb_link);
  }
}

static void count_hosts(void)
{
  host_number++;
}


static void add_traces(void)
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
    link_CM02_t link = xbt_dict_get_or_null(link_set, elm);

    xbt_assert2(link, "Cannot connect trace %s to link %s: link undefined",
                trace_name, elm);
    xbt_assert2(trace, "Cannot connect trace %s to link %s: trace undefined",
                trace_name, elm);

    link->state_event = tmgr_history_add_trace(history, trace, 0.0, 0, link);
  }

  xbt_dict_foreach(trace_connect_list_bandwidth, cursor, trace_name, elm) {
    tmgr_trace_t trace = xbt_dict_get_or_null(traces_set_list, trace_name);
    link_CM02_t link = xbt_dict_get_or_null(link_set, elm);

    xbt_assert2(link, "Cannot connect trace %s to link %s: link undefined",
                trace_name, elm);
    xbt_assert2(trace, "Cannot connect trace %s to link %s: trace undefined",
                trace_name, elm);

    link->bw_event = tmgr_history_add_trace(history, trace, 0.0, 0, link);
  }

  xbt_dict_foreach(trace_connect_list_latency, cursor, trace_name, elm) {
    tmgr_trace_t trace = xbt_dict_get_or_null(traces_set_list, trace_name);
    link_CM02_t link = xbt_dict_get_or_null(link_set, elm);

    xbt_assert2(link, "Cannot connect trace %s to link %s: link undefined",
                trace_name, elm);
    xbt_assert2(trace, "Cannot connect trace %s to link %s: trace undefined",
                trace_name, elm);

    link->lat_event = tmgr_history_add_trace(history, trace, 0.0, 0, link);
  }
}

static void define_callbacks(const char *file)
{
  /* Figuring out the network links */
  surfxml_add_callback(STag_surfxml_host_cb_list, &count_hosts);
  surfxml_add_callback(STag_surfxml_link_cb_list, &parse_link_init);
  surfxml_add_callback(STag_surfxml_route_cb_list,
                       &parse_route_set_endpoints);
  surfxml_add_callback(ETag_surfxml_route_cb_list, &parse_route_set_route);
  surfxml_add_callback(ETag_surfxml_platform_cb_list, &add_traces);
  surfxml_add_callback(ETag_surfxml_platform_cb_list, &add_route);
  surfxml_add_callback(ETag_surfxml_platform_cb_list, &add_loopback);
}

static int resource_used(void *resource_id)
{
  return lmm_constraint_used(network_maxmin_system,
                             ((link_CM02_t) resource_id)->constraint);
}

static int action_unref(surf_action_t action)
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

static void action_cancel(surf_action_t action)
{
  return;
}

static void action_recycle(surf_action_t action)
{
  return;
}

static double share_resources(double now)
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
  surf_action_network_CM02_t action = NULL;
  surf_action_network_CM02_t next_action = NULL;
  xbt_swag_t running_actions = surf_network_model->states.running_action_set;
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
    double_update(&(action->generic_action.remains),
                  lmm_variable_getvalue(action->variable) * deltap);
    if (action->generic_action.max_duration != NO_MAX_DURATION)
      double_update(&(action->generic_action.max_duration), delta);

    if ((action->generic_action.remains <= 0) &&
        (lmm_get_variable_weight(action->variable) > 0)) {
      action->generic_action.finish = surf_get_clock();
      surf_network_model->action_state_set((surf_action_t) action, SURF_ACTION_DONE);
    } else if ((action->generic_action.max_duration != NO_MAX_DURATION) &&
               (action->generic_action.max_duration <= 0)) {
      action->generic_action.finish = surf_get_clock();
      surf_network_model->action_state_set((surf_action_t) action, SURF_ACTION_DONE);
    }
  }

  return;
}

static void update_resource_state(void *id,
                                  tmgr_trace_event_t event_type,
                                  double value, double date)
{
  link_CM02_t nw_link = id;
  /*   printf("[" "%lg" "] Asking to update network card \"%s\" with value " */
  /*     "%lg" " for event %p\n", surf_get_clock(), nw_link->name, */
  /*     value, event_type); */

  if (event_type == nw_link->bw_event) {
    double delta =
      weight_S_parameter / value - weight_S_parameter / nw_link->bw_current;
    lmm_variable_t var = NULL;
    lmm_element_t elem = NULL;
    surf_action_network_CM02_t action = NULL;

    nw_link->bw_current = value;
    lmm_update_constraint_bound(network_maxmin_system, nw_link->constraint,
                                bandwidth_factor * nw_link->bw_current);
    if (weight_S_parameter > 0) {
      while ((var = lmm_get_var_from_cnst
              (network_maxmin_system, nw_link->constraint, &elem))) {
        action = lmm_variable_id(var);
        action->weight += delta;
        if (!(action->suspended))
          lmm_update_variable_weight(network_maxmin_system, action->variable,
                                     action->weight);
      }
    }
  } else if (event_type == nw_link->lat_event) {
    double delta = value - nw_link->lat_current;
    lmm_variable_t var = NULL;
    lmm_element_t elem = NULL;
    surf_action_network_CM02_t action = NULL;

    nw_link->lat_current = value;
    while ((var = lmm_get_var_from_cnst
            (network_maxmin_system, nw_link->constraint, &elem))) {
      action = lmm_variable_id(var);
      action->lat_current += delta;
      action->weight += delta;
      if (action->rate < 0)
        lmm_update_variable_bound(network_maxmin_system, action->variable,
                                  sg_tcp_gamma / (2.0 * action->lat_current));
      else
        lmm_update_variable_bound(network_maxmin_system, action->variable,
                                  min(action->rate,
                                      sg_tcp_gamma / (2.0 *
                                                      action->lat_current)));
      if (!(action->suspended))
        lmm_update_variable_weight(network_maxmin_system, action->variable,
                                   action->weight);

    }
  } else if (event_type == nw_link->state_event) {
    if (value > 0)
      nw_link->state_current = SURF_LINK_ON;
    else {
      lmm_constraint_t cnst = nw_link->constraint;
      lmm_variable_t var = NULL;
      lmm_element_t elem = NULL;

      nw_link->state_current = SURF_LINK_OFF;
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
  } else {
    CRITICAL0("Unknown event ! \n");
    xbt_abort();
  }

  return;
}

static surf_action_t communicate(void *src, void *dst, double size,
                                 double rate)
{
  surf_action_network_CM02_t action = NULL;
  /* LARGE PLATFORMS HACK:
     Add a link_CM02_t *link and a int link_nb to network_card_CM02_t. It will represent local links for this node
     Use the cluster_id for ->id */
  network_card_CM02_t card_src = src;
  network_card_CM02_t card_dst = dst;
  int route_size = ROUTE_SIZE(card_src->id, card_dst->id);
  link_CM02_t *route = ROUTE(card_src->id, card_dst->id);
  /* LARGE PLATFORMS HACK:
     total_route_size = route_size + src->link_nb + dst->nb */
  int i;

  XBT_IN4("(%s,%s,%g,%g)", card_src->generic_resource.name, card_dst->generic_resource.name, size, rate);
  /* LARGE PLATFORMS HACK:
     assert on total_route_size */
  xbt_assert2(route_size,
              "You're trying to send data from %s to %s but there is no connexion between these two cards.",
              card_src->generic_resource.name, card_dst->generic_resource.name);

  action = xbt_new0(s_surf_action_network_CM02_t, 1);

  action->generic_action.refcount = 1;
  action->generic_action.cost = size;
  action->generic_action.remains = size;
  action->generic_action.max_duration = NO_MAX_DURATION;
  action->generic_action.start = surf_get_clock();
  action->generic_action.finish = -1.0;
  action->generic_action.model_type = surf_network_model;
  action->suspended = 0;        /* Should be useless because of the
                                   calloc but it seems to help valgrind... */
  action->generic_action.state_set =
    surf_network_model->states.running_action_set;
  for (i = 0; i < route_size; i++)
    if (route[i]->state_current == SURF_LINK_OFF) {
      action->generic_action.state_set =
        surf_network_model->states.failed_action_set;
      break;
    }

  xbt_swag_insert(action, action->generic_action.state_set);
  action->rate = rate;

  action->latency = 0.0;
  action->weight = 0.0;
  for (i = 0; i < route_size; i++) {
    action->latency += route[i]->lat_current;
    action->weight +=
      route[i]->lat_current + weight_S_parameter / route[i]->bw_current;
  }
  /* LARGE PLATFORMS HACK:
     Add src->link and dst->link latencies */
  action->lat_current = action->latency;
  action->latency *= latency_factor;

  /* LARGE PLATFORMS HACK:
     lmm_variable_new(..., total_route_size) */
  if (action->latency > 0)
    action->variable =
      lmm_variable_new(network_maxmin_system, action, 0.0, -1.0, route_size);
  else
    action->variable =
      lmm_variable_new(network_maxmin_system, action, 1.0, -1.0, route_size);

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

  for (i = 0; i < route_size; i++)
    lmm_expand(network_maxmin_system, route[i]->constraint,
               action->variable, 1.0);
  /* LARGE PLATFORMS HACK:
     expand also with src->link and dst->link */

  XBT_OUT;

  return (surf_action_t) action;
}

/* returns an array of link_CM02_t */
static const void **get_route(void *src, void *dst)
{
  network_card_CM02_t card_src = src;
  network_card_CM02_t card_dst = dst;
  return (const void **) ROUTE(card_src->id, card_dst->id);
}

static int get_route_size(void *src, void *dst)
{
  network_card_CM02_t card_src = src;
  network_card_CM02_t card_dst = dst;
  return ROUTE_SIZE(card_src->id, card_dst->id);
}

static double get_link_bandwidth(const void *link)
{
  return ((link_CM02_t) link)->bw_current;
}

static double get_link_latency(const void *link)
{
  return ((link_CM02_t) link)->lat_current;
}

static int link_shared(const void *link)
{
  return lmm_constraint_is_shared(((link_CM02_t) link)->constraint);
}

static xbt_dict_t get_properties(void *link)
{
  return ((link_CM02_t) link)->properties;
}

static void action_suspend(surf_action_t action)
{
  ((surf_action_network_CM02_t) action)->suspended = 1;
  lmm_update_variable_weight(network_maxmin_system,
                             ((surf_action_network_CM02_t) action)->variable,
                             0.0);
}

static void action_resume(surf_action_t action)
{
  if (((surf_action_network_CM02_t) action)->suspended) {
    lmm_update_variable_weight(network_maxmin_system,
                               ((surf_action_network_CM02_t)
                                action)->variable,
                               ((surf_action_network_CM02_t) action)->weight);
    ((surf_action_network_CM02_t) action)->suspended = 0;
  }
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
  int i, j;

  xbt_dict_free(&link_set);

  surf_model_exit(surf_network_model);
  surf_network_model = NULL;

  loopback = NULL;
  for (i = 0; i < host_number; i++)
    for (j = 0; j < host_number; j++)
      free(ROUTE(i, j));
  free(routing_table);
  routing_table = NULL;
  free(routing_table_size);
  routing_table_size = NULL;
  host_number = 0;
  lmm_system_free(network_maxmin_system);
  network_maxmin_system = NULL;
}

static void surf_network_model_init_internal(void)
{
  surf_network_model = surf_model_init();

  surf_network_model->name = "network";
  surf_network_model->action_unref = action_unref;
  surf_network_model->action_cancel = action_cancel;
  surf_network_model->action_recycle = action_recycle;

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
  surf_cpu_model->set_max_duration = action_set_max_duration;

  surf_network_model->extension.network.communicate = communicate;
  surf_network_model->extension.network.get_route = get_route;
  surf_network_model->extension.network.get_route_size = get_route_size;
  surf_network_model->extension.network.get_link_bandwidth =
    get_link_bandwidth;
  surf_network_model->extension.network.get_link_latency = get_link_latency;
  surf_network_model->extension.network.link_shared = link_shared;

  surf_network_model->get_properties = get_properties;

  link_set = xbt_dict_new();

  if (!network_maxmin_system)
    network_maxmin_system = lmm_system_new();
}

/************************************************************************/
/* New model based on optimizations discussed during this thesis        */
/************************************************************************/
void surf_network_model_init_LegrandVelho(const char *filename)
{

  if (surf_network_model)
    return;
  surf_network_model_init_internal();
  define_callbacks(filename);
  xbt_dynar_push(model_list, &surf_network_model);
  network_solve = lmm_solve;

  latency_factor = 10.4;
  bandwidth_factor = 0.92;
  weight_S_parameter = 8775;

  update_model_description(surf_network_model_description,
                           "LegrandVelho", surf_network_model);
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
  define_callbacks(filename);
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
  define_callbacks(filename);

  xbt_dynar_push(model_list, &surf_network_model);
  lmm_set_default_protocol_function(func_reno_f, func_reno_fp, func_reno_fpi);
  network_solve = lagrange_solve;

  latency_factor = 10.4;
  bandwidth_factor = 0.92;
  weight_S_parameter = 8775;

  update_model_description(surf_network_model_description,
                           "Reno", surf_network_model);
}


void surf_network_model_init_Reno2(const char *filename)
{
  if (surf_network_model)
    return;
  surf_network_model_init_internal();
  define_callbacks(filename);

  xbt_dynar_push(model_list, &surf_network_model);
  lmm_set_default_protocol_function(func_reno2_f, func_reno2_fp,
                                    func_reno2_fpi);
  network_solve = lagrange_solve;

  latency_factor = 10.4;
  bandwidth_factor = 0.92;
  weight_S_parameter = 8775;

  update_model_description(surf_network_model_description,
                           "Reno2", surf_network_model);
}

void surf_network_model_init_Vegas(const char *filename)
{
  if (surf_network_model)
    return;
  surf_network_model_init_internal();
  define_callbacks(filename);

  xbt_dynar_push(model_list, &surf_network_model);
  lmm_set_default_protocol_function(func_vegas_f, func_vegas_fp,
                                    func_vegas_fpi);
  network_solve = lagrange_solve;

  latency_factor = 10.4;
  bandwidth_factor = 0.92;
  weight_S_parameter = 8775;

  update_model_description(surf_network_model_description,
                           "Vegas", surf_network_model);
}

#ifdef HAVE_SDP
void surf_network_model_init_SDP(const char *filename)
{
  if (surf_network_model)
    return;
  surf_network_model_init_internal();
  define_callbacks(filename);

  xbt_dynar_push(model_list, &surf_network_model);
  network_solve = sdp_solve;

  update_model_description(surf_network_model_description,
                           "SDP", surf_network_model);
}
#endif
