/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/network_ns3.hpp"
#include "src/surf/surf_private.h"
#include "src/surf/host_interface.hpp"
#include "simgrid/sg_config.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(ns3);

xbt_dynar_t IPV4addr;
static double time_to_next_flow_completion = -1;

extern xbt_dict_t dict_socket;

/*************
 * Callbacks *
 *************/

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

static void simgrid_ns3_add_host(simgrid::surf::Host* host)
{
  const char* id = host->getName();
  XBT_DEBUG("NS3_ADD_HOST '%s'", id);
  host->getHost()->set_facet(NS3_HOST_LEVEL, ns3_add_host(id));
}

static void parse_ns3_add_link(sg_platf_link_cbarg_t link)
{
  XBT_DEBUG("NS3_ADD_LINK '%s'",link->id);

  if(!IPV4addr) IPV4addr = xbt_dynar_new(sizeof(char*),free);

  surf_network_model->createLink(link->id,
                                     link->bandwidth,
                                     link->bandwidth_trace,
                                     link->latency,
                                     link->latency_trace,
                                     link->state,
                                     link->state_trace,
                                     link->policy,
                                     link->properties);
}

static void simgrid_ns3_add_router(simgrid::surf::RoutingEdge* router)
{
  const char* router_id = router->getName();
  XBT_DEBUG("NS3_ADD_ROUTER '%s'",router_id);
  xbt_lib_set(as_router_lib,
              router_id,
              NS3_ASR_LEVEL,
              ns3_add_router(router_id)
    );
}

static void parse_ns3_add_AS(simgrid::surf::As* as)
{
  const char* as_id = as->p_name;
  XBT_DEBUG("NS3_ADD_AS '%s'", as_id);
  xbt_lib_set(as_router_lib, as_id, NS3_ASR_LEVEL, ns3_add_AS(as_id) );
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
      simgrid::Host::by_name_or_create(router_id)
        ->set_facet(NS3_HOST_LEVEL, ns3_add_host_cluster(router_id));
      XBT_DEBUG("NS3_ADD_ROUTER '%s'",router_id);
      free(router_id);
      break;

    case 2:
      start = surf_parse_get_int(xbt_dynar_get_as(radical_ends, 0, char *));
      end = surf_parse_get_int(xbt_dynar_get_as(radical_ends, 1, char *));
      for (i = start; i <= end; i++){
        xbt_dynar_push_as(tab_elements_num, int, i);
        router_id = bprintf("ns3_%s%d%s", cluster_prefix, i, cluster_suffix);
        simgrid::Host::by_name_or_create(router_id)
          ->set_facet(NS3_HOST_LEVEL, ns3_add_host_cluster(router_id));
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

    ns3_nodes_t host_src = ns3_find_host(host_id);
    ns3_nodes_t host_dst = ns3_find_host(router_id);

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

/* Create the ns3 topology based on routing strategy */
static void create_ns3_topology(void)
{
  XBT_DEBUG("Starting topology generation");

  xbt_dynar_shrink(IPV4addr,0);

  //get the onelinks from the parsed platform
  xbt_dynar_t onelink_routes = routing_platf->getOneLinkRoutes();
  if (!onelink_routes)
    xbt_die("There is no routes!");
  XBT_DEBUG("Have get_onelink_routes, found %ld routes",onelink_routes->used);
  //save them in trace file
  simgrid::surf::Onelink *onelink;
  unsigned int iter;
  xbt_dynar_foreach(onelink_routes, iter, onelink) {
    char *src = onelink->p_src->getName();
    char *dst = onelink->p_dst->getName();
    simgrid::surf::NetworkNS3Link *link =
      static_cast<simgrid::surf::NetworkNS3Link *>(onelink->p_link);

    if (strcmp(src,dst) && link->m_created){
      XBT_DEBUG("Route from '%s' to '%s' with link '%s'", src, dst, link->getName());
      char * link_bdw = xbt_strdup(link->p_bdw);
      char * link_lat = xbt_strdup(link->p_lat);
      replace_lat_ns3(&link_lat);
      replace_bdw_ns3(&link_bdw);
      link->m_created = 0;

      //   XBT_DEBUG("src (%s), dst (%s), src_id = %d, dst_id = %d",src,dst, src_id, dst_id);
      XBT_DEBUG("\tLink (%s) bdw:%s lat:%s", link->getName(), link_bdw, link_lat);

      //create link ns3
      ns3_nodes_t host_src = ns3_find_host(src);
      if(!host_src) host_src = static_cast<ns3_nodes_t>(xbt_lib_get_or_null(as_router_lib,src,NS3_ASR_LEVEL));
      ns3_nodes_t host_dst = ns3_find_host(dst);
      if(!host_dst) host_dst = static_cast<ns3_nodes_t>(xbt_lib_get_or_null(as_router_lib,dst,NS3_ASR_LEVEL));

      if(host_src && host_dst){}
      else xbt_die("\tns3_add_link from %d to %d",host_src->node_num,host_dst->node_num);

      ns3_add_link(host_src->node_num,host_src->type,host_dst->node_num,host_dst->type,link_bdw,link_lat);

      xbt_free(link_bdw);
      xbt_free(link_lat);
    }
  }
}

static void parse_ns3_end_platform(void)
{
  ns3_end_platform();
}

static void define_callbacks_ns3(void)
{
  simgrid::surf::hostCreatedCallbacks.connect(simgrid_ns3_add_host);
  simgrid::surf::routingEdgeCreatedCallbacks.connect(simgrid_ns3_add_router);
  sg_platf_link_add_cb (&parse_ns3_add_link);
  sg_platf_cluster_add_cb (&parse_ns3_add_cluster);
  simgrid::surf::asCreatedCallbacks.connect(parse_ns3_add_AS);
  sg_platf_postparse_add_cb(&create_ns3_topology); //get_one_link_routes
  sg_platf_postparse_add_cb(&parse_ns3_end_platform); //InitializeRoutes
}

/*********
 * Model *
 *********/
static void free_ns3_link(void * elmts)
{
  delete static_cast<simgrid::surf::NetworkNS3Link*>(elmts);
}

static void free_ns3_host(void * elmts)
{
  ns3_nodes_t host = static_cast<ns3_nodes_t>(elmts);
  free(host);
}

void surf_network_model_init_NS3()
{
  if (surf_network_model)
    return;

  surf_network_model = new simgrid::surf::NetworkNS3Model();

  xbt_dynar_push(all_existing_models, &surf_network_model);
}

namespace simgrid {
namespace surf {

NetworkNS3Model::NetworkNS3Model() : NetworkModel() {
  if (ns3_initialize(xbt_cfg_get_string(_sg_cfg_set, "ns3/TcpModel"))) {
    xbt_die("Impossible to initialize NS3 interface");
  }
  routing_model_create(NULL);
  define_callbacks_ns3();

  NS3_HOST_LEVEL = simgrid::Host::add_level(free_ns3_host);
  NS3_ASR_LEVEL  = xbt_lib_add_level(as_router_lib, free_ns3_host);
}

NetworkNS3Model::~NetworkNS3Model() {
  ns3_finalize();
  xbt_dynar_free_container(&IPV4addr);
  xbt_dict_free(&dict_socket);
}

Link* NetworkNS3Model::createLink(const char *name,
	                                 double bw_initial,
	                                 tmgr_trace_t bw_trace,
	                                 double lat_initial,
	                                 tmgr_trace_t lat_trace,
	                                 e_surf_resource_state_t state_initial,
	                                 tmgr_trace_t state_trace,
	                                 e_surf_link_sharing_policy_t policy,
	                                 xbt_dict_t properties){
  if (bw_trace)
    XBT_INFO("The NS3 network model doesn't support bandwidth state traces");
  if (lat_trace)
    XBT_INFO("The NS3 network model doesn't support latency state traces");
  if (state_trace)
    XBT_INFO("The NS3 network model doesn't support link state traces");
  Link* link = new NetworkNS3Link(this, name, properties, bw_initial, lat_initial);
  surf_callback_emit(networkLinkCreatedCallbacks, link);
  return link;
}

xbt_dynar_t NetworkNS3Model::getRoute(RoutingEdge *src, RoutingEdge *dst)
{
  xbt_dynar_t route = NULL;
  routing_get_route_and_latency(src, dst, &route, NULL);
  //routing_platf->getRouteAndLatency(src, dst, &route, NULL);
  return route;
}

Action *NetworkNS3Model::communicate(RoutingEdge *src, RoutingEdge *dst,
		                               double size, double rate)
{
  XBT_DEBUG("Communicate from %s to %s", src->getName(), dst->getName());
  NetworkNS3Action *action = new NetworkNS3Action(this, size, 0);

  ns3_create_flow(src->getName(), dst->getName(), surf_get_clock(), size, action);

  action->m_lastSent = 0;
  action->p_srcElm = src;
  action->p_dstElm = dst;
  surf_callback_emit(networkCommunicateCallbacks, action, src, dst, size, rate);

  return (surf_action_t) action;
}

double NetworkNS3Model::shareResources(double now)
{
  XBT_DEBUG("ns3_share_resources");

  //get the first relevant value from the running_actions list
  if (!getRunningActionSet()->size() || now == 0.0)
    return -1.0;
  else
    do {
      ns3_simulator(now);
      time_to_next_flow_completion = ns3_time() - surf_get_clock();//FIXME: use now instead ?
    } while(double_equals(time_to_next_flow_completion, 0, sg_surf_precision));

  XBT_DEBUG("min       : %f", now);
  XBT_DEBUG("ns3  time : %f", ns3_time());
  XBT_DEBUG("surf time : %f", surf_get_clock());
  XBT_DEBUG("Next completion %f :", time_to_next_flow_completion);

  return time_to_next_flow_completion;
}

void NetworkNS3Model::updateActionsState(double now, double delta)
{
  xbt_dict_cursor_t cursor = NULL;
  char *key;
  void *data;

  static xbt_dynar_t socket_to_destroy = NULL;
  if(!socket_to_destroy) socket_to_destroy = xbt_dynar_new(sizeof(char*),NULL);

  /* If there are no running flows, just return */
  if (!getRunningActionSet()->size()) {
    while(double_positive(now-ns3_time(), sg_surf_precision)) {
      ns3_simulator(now-ns3_time());
    }
    return;
  }

  NetworkNS3Action *action;
  xbt_dict_foreach(dict_socket,cursor,key,data){
    action = static_cast<NetworkNS3Action*>(ns3_get_socket_action(data));
    XBT_DEBUG("Processing socket %p (action %p)",data,action);
    action->setRemains(action->getCost() - ns3_get_socket_sent(data));

    if (TRACE_is_enabled() &&
    		action->getState() == SURF_ACTION_RUNNING){
    	double data_sent = ns3_get_socket_sent(data);
    	double data_delta_sent = data_sent - action->m_lastSent;

    	xbt_dynar_t route = NULL;

    	routing_get_route_and_latency (action->p_srcElm, action->p_dstElm, &route, NULL);
    	unsigned int i;
    	for (i = 0; i < xbt_dynar_length (route); i++){
    		NetworkNS3Link* link = ((NetworkNS3Link*)xbt_dynar_get_ptr(route, i));
    		TRACE_surf_link_set_utilization (link->getName(),
    				action->getCategory(),
					(data_delta_sent)/delta,
					now-delta,
					delta);
    	}
    	action->m_lastSent = data_sent;
    }

    if(ns3_get_socket_is_finished(data) == 1){
      xbt_dynar_push(socket_to_destroy,&key);
      XBT_DEBUG("Destroy socket %p of action %p", key, action);
      action->finish();
      action->setState(SURF_ACTION_DONE);
    }
  }

  while (!xbt_dynar_is_empty(socket_to_destroy)){
    xbt_dynar_pop(socket_to_destroy,&key);

    void *data = xbt_dict_get (dict_socket, key);
    action = static_cast<NetworkNS3Action*>(ns3_get_socket_action(data));
    XBT_DEBUG ("Removing socket %p of action %p", key, action);
    xbt_dict_remove(dict_socket, key);
  }
  return;
}

/************
 * Resource *
 ************/

NetworkNS3Link::NetworkNS3Link(NetworkNS3Model *model, const char *name, xbt_dict_t props,
		                       double bw_initial, double lat_initial)
 : Link(model, name, props)
 , p_lat(bprintf("%f", lat_initial))
 , p_bdw(bprintf("%f", bw_initial))
 , m_created(1)
{
}

NetworkNS3Link::~NetworkNS3Link()
{
}

void NetworkNS3Link::updateState(tmgr_trace_event_t event_type, double value, double date)
{

}

/**********
 * Action *
 **********/

NetworkNS3Action::NetworkNS3Action(Model *model, double cost, bool failed)
: NetworkAction(model, cost, failed)
{}

#ifdef HAVE_LATENCY_BOUND_TRACKING
  int NetworkNS3Action::getLatencyLimited() {
    return m_latencyLimited;
  }
#endif

 void NetworkNS3Action::suspend()
{
  THROW_UNIMPLEMENTED;
}

void NetworkNS3Action::resume()
{
  THROW_UNIMPLEMENTED;
}

  /* Test whether a flow is suspended */
bool NetworkNS3Action::isSuspended()
{
  return 0;
}

int NetworkNS3Action::unref()
{
  m_refcount--;
  if (!m_refcount) {
	if (action_hook.is_linked())
	  p_stateSet->erase(p_stateSet->iterator_to(*this));
    XBT_DEBUG ("Removing action %p", this);
	delete this;
    return 1;
  }
  return 0;
}

}
}
