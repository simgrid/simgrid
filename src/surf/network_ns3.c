/* Copyright (c) 2007, 2008, 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_private.h"
#include "surf/maxmin.h"
#include "surf/ns3/ns3_interface.h"
#include "xbt/lib.h"
#include "surf/network_ns3_private.h"
#include "xbt/str.h"

extern xbt_lib_t host_lib;
extern xbt_lib_t link_lib;
extern xbt_lib_t as_router_lib;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_network_ns3, surf,
                                "Logging specific to the SURF network NS3 module");

extern routing_platf_t routing_platf;
extern xbt_dict_t dict_socket;

static double time_to_next_flow_completion = -1;

static double ns3_share_resources(double min);
static void ns3_update_actions_state(double now, double delta);
static void finalize(void);
static surf_action_t ns3_communicate(sg_routing_edge_t src_elm,
                                     sg_routing_edge_t dst_elm,
                                     double size, double rate);
static void action_suspend(surf_action_t action);
static void action_resume(surf_action_t action);
static int action_is_suspended(surf_action_t action);
static int action_unref(surf_action_t action);

xbt_dynar_t IPV4addr;

static void replace_bdw_ns3(char ** bdw)
{
  char *temp = xbt_strdup(*bdw);
  xbt_free(*bdw);
  *bdw = bprintf("%fBps",atof(temp));
  xbt_free(temp);

}

static void replace_lat_ns3(char ** lat)
{
  char *temp = xbt_strdup(*lat);
  xbt_free(*lat);
  *lat = bprintf("%fs",atof(temp));
  xbt_free(temp);
}

static void parse_ns3_add_host(sg_platf_host_cbarg_t host)
{
  XBT_DEBUG("NS3_ADD_HOST '%s'",host->id);
  xbt_lib_set(host_lib,
              host->id,
              NS3_HOST_LEVEL,
              ns3_add_host(host->id)
    );
}

static void parse_ns3_add_link(sg_platf_link_cbarg_t link)
{
  XBT_DEBUG("NS3_ADD_LINK '%s'",link->id);

  if(!IPV4addr) IPV4addr = xbt_dynar_new(sizeof(char*),free);

  tmgr_trace_t bw_trace;
  tmgr_trace_t state_trace;
  tmgr_trace_t lat_trace;

  bw_trace = link->bandwidth_trace;
  lat_trace = link->latency_trace;
  state_trace = link->state_trace;

  if (bw_trace)
    XBT_INFO("The NS3 network model doesn't support bandwidth state traces");
  if (lat_trace)
    XBT_INFO("The NS3 network model doesn't support latency state traces");
  if (state_trace)
    XBT_INFO("The NS3 network model doesn't support link state traces");

  ns3_link_t link_ns3 = xbt_new0(s_ns3_link_t,1);;
  link_ns3->id = xbt_strdup((char*)(link->id));
  link_ns3->bdw = bprintf("%f",link->bandwidth);
  link_ns3->lat = bprintf("%f",link->latency);

  surf_ns3_link_t l = xbt_new0(s_surf_ns3_link_t,1);
  l->generic_resource.name = xbt_strdup(link->id);
  l->generic_resource.properties = current_property_set;
  l->data = link_ns3;
  l->created = 1;

  xbt_lib_set(link_lib,link->id,NS3_LINK_LEVEL,link_ns3);
  xbt_lib_set(link_lib,link->id,SURF_LINK_LEVEL,l);
}

static void parse_ns3_add_router(sg_platf_router_cbarg_t router)
{
  XBT_DEBUG("NS3_ADD_ROUTER '%s'",router->id);
  xbt_lib_set(as_router_lib,
              router->id,
              NS3_ASR_LEVEL,
              ns3_add_router(router->id)
    );
}

static void parse_ns3_add_AS(const char*id, const char*routing)
{
  XBT_DEBUG("NS3_ADD_AS '%s'",id);
  xbt_lib_set(as_router_lib,
              id,
              NS3_ASR_LEVEL,
              ns3_add_AS(id)
    );
}

static void parse_ns3_add_cluster(sg_platf_cluster_cbarg_t cluster)
{
  const char *cluster_prefix = cluster->prefix;
  const char *cluster_suffix = cluster->suffix;
  const char *cluster_radical = cluster->radical;
  const char *cluster_bb_bw = bprintf("%f",cluster->bb_bw);
  const char *cluster_bb_lat = bprintf("%f",cluster->bb_lat);
  const char *cluster_bw = bprintf("%f",cluster->bw);
  const char *cluster_lat = bprintf("%f",cluster->lat);
  const char *groups = NULL;

  int start, end, i;
  unsigned int iter;

  xbt_dynar_t radical_elements;
  xbt_dynar_t radical_ends;
  xbt_dynar_t tab_elements_num = xbt_dynar_new(sizeof(int), NULL);

  char *router_id,*host_id;

  radical_elements = xbt_str_split(cluster_radical, ",");
  xbt_dynar_foreach(radical_elements, iter, groups) {
    radical_ends = xbt_str_split(groups, "-");

    switch (xbt_dynar_length(radical_ends)) {
    case 1:
      start = surf_parse_get_int(xbt_dynar_get_as(radical_ends, 0, char *));
      xbt_dynar_push_as(tab_elements_num, int, start);
      router_id = bprintf("ns3_%s%d%s", cluster_prefix, start, cluster_suffix);
      xbt_lib_set(host_lib,
                  router_id,
                  NS3_HOST_LEVEL,
                  ns3_add_host_cluster(router_id)
        );
      XBT_DEBUG("NS3_ADD_ROUTER '%s'",router_id);
      free(router_id);
      break;

    case 2:
      start = surf_parse_get_int(xbt_dynar_get_as(radical_ends, 0, char *));
      end = surf_parse_get_int(xbt_dynar_get_as(radical_ends, 1, char *));
      for (i = start; i <= end; i++){
        xbt_dynar_push_as(tab_elements_num, int, i);
        router_id = bprintf("ns3_%s%d%s", cluster_prefix, i, cluster_suffix);
        xbt_lib_set(host_lib,
                    router_id,
                    NS3_HOST_LEVEL,
                    ns3_add_host_cluster(router_id)
          );
        XBT_DEBUG("NS3_ADD_ROUTER '%s'",router_id);
        free(router_id);
      }
      break;

    default:
      XBT_DEBUG("Malformed radical");
    }
  }

  //Create links
  unsigned int cpt;
  int elmts;
  char * lat = xbt_strdup(cluster_lat);
  char * bw =  xbt_strdup(cluster_bw);
  replace_lat_ns3(&lat);
  replace_bdw_ns3(&bw);

  xbt_dynar_foreach(tab_elements_num,cpt,elmts)
  {
    host_id   = bprintf("%s%d%s", cluster_prefix, elmts, cluster_suffix);
    router_id = bprintf("ns3_%s%d%s", cluster_prefix, elmts, cluster_suffix);
    XBT_DEBUG("Create link from '%s' to '%s'",host_id,router_id);

    ns3_nodes_t host_src = xbt_lib_get_or_null(host_lib,host_id,  NS3_HOST_LEVEL);
    ns3_nodes_t host_dst = xbt_lib_get_or_null(host_lib,router_id,NS3_HOST_LEVEL);

    if(host_src && host_dst){}
    else xbt_die("\tns3_add_link from %d to %d",host_src->node_num,host_dst->node_num);

    ns3_add_link(host_src->node_num,host_src->type,
                 host_dst->node_num,host_dst->type,
                 bw,lat);

    free(router_id);
    free(host_id);
  }
  xbt_dynar_free(&tab_elements_num);


  //Create link backbone
  lat = xbt_strdup(cluster_bb_lat);
  bw =  xbt_strdup(cluster_bb_bw);
  replace_lat_ns3(&lat);
  replace_bdw_ns3(&bw);
  ns3_add_cluster(bw,lat,cluster->id);
  xbt_free(lat);
  xbt_free(bw);  
}

static double ns3_get_link_latency (const void *link)
{
  double lat;
  //XBT_DEBUG("link_id:%s link_lat:%s link_bdw:%s",((surf_ns3_link_t)link)->data->id,((surf_ns3_link_t)link)->data->lat,((surf_ns3_link_t)link)->data->bdw);
  sscanf(((surf_ns3_link_t)link)->data->lat,"%lg",&lat);
  return lat;
}
static double ns3_get_link_bandwidth (const void *link)
{
  double bdw;
  //XBT_DEBUG("link_id:%s link_lat:%s link_bdw:%s",((surf_ns3_link_t)link)->data->id,((surf_ns3_link_t)link)->data->lat,((surf_ns3_link_t)link)->data->bdw);
  sscanf(((surf_ns3_link_t)link)->data->bdw,"%lg",&bdw);
  return bdw;
}

static xbt_dynar_t ns3_get_route(void *src_card, void *dst_card)
{
  xbt_dynar_t route = NULL;
  routing_get_route_and_latency(src_card, dst_card, &route, NULL);
  return route;
}

static void parse_ns3_end_platform(void)
{
  ns3_end_platform();
}

/* Create the ns3 topology based on routing strategy */
static void create_ns3_topology(void)
{
  XBT_DEBUG("Starting topology generation");

  xbt_dynar_shrink(IPV4addr,0);

  //get the onelinks from the parsed platform
  xbt_dynar_t onelink_routes = routing_platf->get_onelink_routes();
  if (!onelink_routes)
    xbt_die("There is no routes!");
  XBT_DEBUG("Have get_onelink_routes, found %ld routes",onelink_routes->used);
  //save them in trace file
  onelink_t onelink;
  unsigned int iter;
  xbt_dynar_foreach(onelink_routes, iter, onelink) {
    char *src = onelink->src->name;
    char *dst = onelink->dst->name;
    void *link = onelink->link_ptr;
    if( strcmp(src,dst) && ((surf_ns3_link_t)link)->created){
      XBT_DEBUG("Route from '%s' to '%s' with link '%s'",src,dst,((surf_ns3_link_t)link)->data->id);
      char * link_bdw = xbt_strdup(((surf_ns3_link_t)link)->data->bdw);
      char * link_lat = xbt_strdup(((surf_ns3_link_t)link)->data->lat);
      replace_lat_ns3(&link_lat);
      replace_bdw_ns3(&link_bdw);
      ((surf_ns3_link_t)link)->created = 0;

      //   XBT_DEBUG("src (%s), dst (%s), src_id = %d, dst_id = %d",src,dst, src_id, dst_id);
      XBT_DEBUG("\tLink (%s) bdw:%s lat:%s",((surf_ns3_link_t)link)->data->id,
                link_bdw,
                link_lat
        );

      //create link ns3
      ns3_nodes_t host_src = xbt_lib_get_or_null(host_lib,src,NS3_HOST_LEVEL);
      if(!host_src) host_src = xbt_lib_get_or_null(as_router_lib,src,NS3_ASR_LEVEL);
      ns3_nodes_t host_dst = xbt_lib_get_or_null(host_lib,dst,NS3_HOST_LEVEL);
      if(!host_dst) host_dst = xbt_lib_get_or_null(as_router_lib,dst,NS3_ASR_LEVEL);

      if(host_src && host_dst){}
      else xbt_die("\tns3_add_link from %d to %d",host_src->node_num,host_dst->node_num);

      ns3_add_link(host_src->node_num,host_src->type,host_dst->node_num,host_dst->type,link_bdw,link_lat);

      xbt_free(link_bdw);
      xbt_free(link_lat);
    }
  }
}

static void define_callbacks_ns3(void)
{
  sg_platf_host_add_cb (&parse_ns3_add_host);
  sg_platf_router_add_cb (&parse_ns3_add_router);
  sg_platf_link_add_cb (&parse_ns3_add_link);
  sg_platf_cluster_add_cb (&parse_ns3_add_cluster);
  sg_platf_AS_begin_add_cb (&parse_ns3_add_AS);
  sg_platf_postparse_add_cb(&create_ns3_topology); //get_one_link_routes
  sg_platf_postparse_add_cb(&parse_ns3_end_platform); //InitializeRoutes
}

static void free_ns3_link(void * elmts)
{
  ns3_link_t link = elmts;
  free(link->id);
  free(link->bdw);
  free(link->lat);
  free(link);
}

static void free_ns3_host(void * elmts)
{
  ns3_nodes_t host = elmts;
  free(host);
}

#ifdef HAVE_LATENCY_BOUND_TRACKING
static int ns3_get_link_latency_limited(surf_action_t action)
{
  return 0;
}
#endif

#ifdef HAVE_TRACING
static void ns3_action_set_category(surf_action_t action, const char *category)
{
  action->category = xbt_strdup (category);
}
#endif

void surf_network_model_init_NS3()
{
  if (surf_network_model)
    return;

  surf_network_model = surf_model_init();
  surf_network_model->name = "network NS3";
  surf_network_model->extension.network.get_link_latency = ns3_get_link_latency;
  surf_network_model->extension.network.get_link_bandwidth = ns3_get_link_bandwidth;
  surf_network_model->extension.network.get_route = ns3_get_route;

  surf_network_model->model_private->share_resources = ns3_share_resources;
  surf_network_model->model_private->update_actions_state = ns3_update_actions_state;
  surf_network_model->model_private->finalize = finalize;

  surf_network_model->suspend = action_suspend;
  surf_network_model->resume = action_resume;
  surf_network_model->is_suspended = action_is_suspended;
  surf_network_model->action_unref = action_unref;
  surf_network_model->extension.network.communicate = ns3_communicate;

#ifdef HAVE_TRACING
  surf_network_model->set_category = ns3_action_set_category;
#endif

  /* Added the initialization for NS3 interface */

  if (ns3_initialize(xbt_cfg_get_string(_surf_cfg_set,"ns3/TcpModel"))) {
    xbt_die("Impossible to initialize NS3 interface");
  }

  routing_model_create(NULL);
  define_callbacks_ns3();

  NS3_HOST_LEVEL = xbt_lib_add_level(host_lib,(void_f_pvoid_t)free_ns3_host);
  NS3_ASR_LEVEL  = xbt_lib_add_level(as_router_lib,(void_f_pvoid_t)free_ns3_host);
  NS3_LINK_LEVEL = xbt_lib_add_level(link_lib,(void_f_pvoid_t)free_ns3_link);

  xbt_dynar_push(model_list, &surf_network_model);

#ifdef HAVE_LATENCY_BOUND_TRACKING
  surf_network_model->get_latency_limited = ns3_get_link_latency_limited;
#endif
}

static void finalize(void)
{
  ns3_finalize();
  xbt_dynar_free_container(&IPV4addr);
  xbt_dict_free(&dict_socket);
}

static double ns3_share_resources(double min)
{
  XBT_DEBUG("ns3_share_resources");

  xbt_swag_t running_actions =
    surf_network_model->states.running_action_set;

  //get the first relevant value from the running_actions list
  if (!xbt_swag_size(running_actions) || min == 0.0)
    return -1.0;
  else
    do {
      ns3_simulator(min);
      time_to_next_flow_completion = ns3_time() - surf_get_clock();
    } while(double_equals(time_to_next_flow_completion,0));

  XBT_DEBUG("min       : %f",min);
  XBT_DEBUG("ns3  time : %f",ns3_time());
  XBT_DEBUG("surf time : %f",surf_get_clock());
  XBT_DEBUG("Next completion %f :",time_to_next_flow_completion);

  return time_to_next_flow_completion;
}

static void ns3_update_actions_state(double now, double delta)
{
  xbt_dict_cursor_t cursor = NULL;
  char *key;
  void *data;

  static xbt_dynar_t socket_to_destroy = NULL;
  if(!socket_to_destroy) socket_to_destroy = xbt_dynar_new(sizeof(char*),NULL);

  surf_action_network_ns3_t action = NULL;
  xbt_swag_t running_actions =
    surf_network_model->states.running_action_set;

  /* If there are no running flows, just return */
  if (!xbt_swag_size(running_actions)) {
    while(double_positive(now-ns3_time())) {
      ns3_simulator(now-ns3_time());
    }
    return;
  }

  xbt_dict_foreach(dict_socket,cursor,key,data){
    action = (surf_action_network_ns3_t)ns3_get_socket_action(data);
    XBT_DEBUG("Processing socket %p (action %p)",data,action);
    action->generic_action.remains = action->generic_action.cost - ns3_get_socket_sent(data);

#ifdef HAVE_TRACING
    if (TRACE_is_enabled() &&
        surf_action_state_get(&(action->generic_action)) == SURF_ACTION_RUNNING){
      double data_sent = ns3_get_socket_sent(data);
      double data_delta_sent = data_sent - action->last_sent;

      xbt_dynar_t route = NULL;

      routing_get_route_and_latency (action->src_elm, action->dst_elm, &route, NULL);
      unsigned int i;
      for (i = 0; i < xbt_dynar_length (route); i++){
        surf_ns3_link_t *link = ((surf_ns3_link_t*)xbt_dynar_get_ptr (route, i));
        TRACE_surf_link_set_utilization ((*link)->generic_resource.name,
                                         ((surf_action_t) action)->category,
                                         (data_delta_sent)/delta,
                                         now-delta,
                                         delta);
      }
      action->last_sent = data_sent;
    }
#endif

    if(ns3_get_socket_is_finished(data) == 1){
      xbt_dynar_push(socket_to_destroy,&key);
      XBT_DEBUG("Destroy socket %p of action %p", key, action);
      action->generic_action.finish = now;
      surf_action_state_set(&(action->generic_action), SURF_ACTION_DONE);
    }
  }

  while (!xbt_dynar_is_empty(socket_to_destroy)){
    xbt_dynar_pop(socket_to_destroy,&key);

    void *data = xbt_dict_get (dict_socket, key);
    surf_action_network_ns3_t action = (surf_action_network_ns3_t)ns3_get_socket_action(data);
    XBT_DEBUG ("Removing socket %p of action %p", key, action);
    xbt_dict_remove(dict_socket,key);
  }
  return;
}

/* Max durations are not supported */
static surf_action_t ns3_communicate(sg_routing_edge_t src_elm,
                                     sg_routing_edge_t dst_elm,
                                     double size, double rate)
{
  surf_action_network_ns3_t action = NULL;

  XBT_DEBUG("Communicate from %s to %s", src_elm->name, dst_elm->name);
  action = surf_action_new(sizeof(s_surf_action_network_ns3_t), size, surf_network_model, 0);

  ns3_create_flow(src_elm->name, dst_elm->name, surf_get_clock(), size, action);

#ifdef HAVE_TRACING
  action->last_sent = 0;
  action->src_elm = src_elm;
  action->dst_elm = dst_elm;
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

static int action_unref(surf_action_t action)
{
  action->refcount--;
  if (!action->refcount) {
    xbt_swag_remove(action, action->state_set);

#ifdef HAVE_TRACING
    xbt_free(action->category);
#endif
    XBT_DEBUG ("Removing action %p", action);
    surf_action_free(&action);
    return 1;
  }
  return 0;
}
