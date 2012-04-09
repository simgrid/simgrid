/* Copyright (c) 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <pcre.h>               /* regular expression library */

#include "simgrid/platf_interface.h"    // platform creation API internal interface

#include "surf_routing_private.h"
#include "surf/surf_routing.h"
#include "surf/surfxml_parse_values.h"

/**
 * @ingroup SURF_build_api
 * @brief A library containing all known workstations
 */
xbt_lib_t host_lib;

int ROUTING_HOST_LEVEL;         //Routing level
int SURF_CPU_LEVEL;             //Surf cpu level
int SURF_WKS_LEVEL;             //Surf workstation level
int SIMIX_HOST_LEVEL;           //Simix level
int MSG_HOST_LEVEL;             //Msg level
int SD_HOST_LEVEL;              //Simdag level
int COORD_HOST_LEVEL=0;         //Coordinates level
int NS3_HOST_LEVEL;             //host node for ns3

/**
 * @ingroup SURF_build_api
 * @brief A library containing all known links
 */
xbt_lib_t link_lib;
int SD_LINK_LEVEL;              //Simdag level
int SURF_LINK_LEVEL;            //Surf level
int NS3_LINK_LEVEL;             //link for ns3

xbt_lib_t as_router_lib;
int ROUTING_ASR_LEVEL;          //Routing level
int COORD_ASR_LEVEL;            //Coordinates level
int NS3_ASR_LEVEL;              //host node for ns3

static xbt_dict_t random_value = NULL;

/** @brief Retrieve a routing edge from its name
 *
 * Routing edges are either CPU/workstation and routers, whatever
 */
sg_routing_edge_t sg_routing_edge_by_name_or_null(const char *name) {
    sg_routing_edge_t net_elm = xbt_lib_get_or_null(host_lib, name, ROUTING_HOST_LEVEL);
    if(!net_elm) net_elm = xbt_lib_get_or_null(as_router_lib, name, ROUTING_ASR_LEVEL);
  return net_elm;
}

/* Global vars */
routing_global_t global_routing = NULL;
AS_t current_routing = NULL;

/* global parse functions */
xbt_dynar_t parsed_link_list = NULL;   /* temporary store of current list link of a route */
extern xbt_dynar_t mount_list;

static const char *src = NULL;  /* temporary store the source name of a route */
static const char *dst = NULL;  /* temporary store the destination name of a route */
static char *gw_src = NULL;     /* temporary store the gateway source name of a route */
static char *gw_dst = NULL;     /* temporary store the gateway destination name of a route */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route, surf, "Routing part of surf");

static void routing_parse_peer(sg_platf_peer_cbarg_t peer);     /* peer bypass */
static void routing_parse_Srandom(void);        /* random bypass */

static void routing_parse_postparse(void);

/* this lines are only for replace use like index in the model table */
typedef enum {
  SURF_MODEL_FULL = 0,
  SURF_MODEL_FLOYD,
  SURF_MODEL_DIJKSTRA,
  SURF_MODEL_DIJKSTRACACHE,
  SURF_MODEL_NONE,
  SURF_MODEL_RULEBASED,
  SURF_MODEL_VIVALDI,
  SURF_MODEL_CLUSTER
} e_routing_types;

struct s_model_type routing_models[] = {
  {"Full",
   "Full routing data (fast, large memory requirements, fully expressive)",
   model_full_create, model_full_end},
  {"Floyd",
   "Floyd routing data (slow initialization, fast lookup, lesser memory requirements, shortest path routing only)",
   model_floyd_create, model_floyd_end},
  {"Dijkstra",
   "Dijkstra routing data (fast initialization, slow lookup, small memory requirements, shortest path routing only)",
   model_dijkstra_create, model_dijkstra_both_end},
  {"DijkstraCache",
   "Dijkstra routing data (fast initialization, fast lookup, small memory requirements, shortest path routing only)",
   model_dijkstracache_create, model_dijkstra_both_end},
  {"none", "No routing (Unless you know what you are doing, avoid using this mode in combination with a non Constant network model).",
   model_none_create,  NULL},
  {"RuleBased", "Rule-Based routing data (...)",
   model_rulebased_create, NULL},
  {"Vivaldi", "Vivaldi routing",
   model_vivaldi_create, NULL},
  {"Cluster", "Cluster routing",
   model_cluster_create, NULL},
  {NULL, NULL, NULL, NULL}
};

/**
 * \brief Add a "host" to the network element list
 */
static void parse_S_host(sg_platf_host_cbarg_t host)
{
  sg_routing_edge_t info = NULL;
  if (current_routing->hierarchy == SURF_ROUTING_NULL)
    current_routing->hierarchy = SURF_ROUTING_BASE;
  xbt_assert(!xbt_lib_get_or_null(host_lib, host->id, ROUTING_HOST_LEVEL),
             "Reading a host, processing unit \"%s\" already exists", host->id);

  info = xbt_new0(s_network_element_t, 1);
  info->rc_component = current_routing;
  info->rc_type = SURF_NETWORK_ELEMENT_HOST;
  info->name = xbt_strdup(host->id);
  info->id = current_routing->parse_PU(current_routing, (void *) info);
  xbt_lib_set(host_lib, host->id, ROUTING_HOST_LEVEL, (void *) info);
  XBT_DEBUG("Having set name '%s' id '%d'",host->id,info->id);

  if(mount_list){
    xbt_lib_set(storage_lib, host->id, ROUTING_STORAGE_HOST_LEVEL, (void *) mount_list);
    mount_list = NULL;
  }

  if (host->coord && strcmp(host->coord, "")) {
    unsigned int cursor;
    char*str;

    if (!COORD_HOST_LEVEL)
      xbt_die ("To use host coordinates, please add --cfg=network/coordinates:yes to your command line");
    /* Pre-parse the host coordinates -- FIXME factorize with routers by overloading the routing->parse_PU function*/
    xbt_dynar_t ctn_str = xbt_str_split_str(host->coord, " ");
    xbt_dynar_t ctn = xbt_dynar_new(sizeof(double),NULL);
    xbt_dynar_foreach(ctn_str,cursor, str) {
      double val = atof(str);
      xbt_dynar_push(ctn,&val);
    }
    xbt_dynar_shrink(ctn, 0);
    xbt_dynar_free(&ctn_str);
    xbt_lib_set(host_lib, host->id, COORD_HOST_LEVEL, (void *) ctn);
    XBT_DEBUG("Having set host coordinates for '%s'",host->id);
  }
}

/**
 * \brief Add a "router" to the network element list
 */
static void parse_S_router(sg_platf_router_cbarg_t router)
{
  sg_routing_edge_t info = NULL;
  if (current_routing->hierarchy == SURF_ROUTING_NULL)
    current_routing->hierarchy = SURF_ROUTING_BASE;
  xbt_assert(!xbt_lib_get_or_null(as_router_lib, router->id, ROUTING_ASR_LEVEL),
             "Reading a router, processing unit \"%s\" already exists",
             router->id);

  info = xbt_new0(s_network_element_t, 1);
  info->rc_component = current_routing;
  info->rc_type = SURF_NETWORK_ELEMENT_ROUTER;
  info->name = xbt_strdup(router->id);
  info->id = current_routing->parse_PU(current_routing, (void *) info);
  xbt_lib_set(as_router_lib, router->id, ROUTING_ASR_LEVEL, (void *) info);
  XBT_DEBUG("Having set name '%s' id '%d'",router->id,info->id);

  if (strcmp(router->coord, "")) {
    unsigned int cursor;
    char*str;

    if (!COORD_ASR_LEVEL)
      xbt_die ("To use host coordinates, please add --cfg=network/coordinates:yes to your command line");
    /* Pre-parse the host coordinates */
    xbt_dynar_t ctn_str = xbt_str_split_str(router->coord, " ");
    xbt_dynar_t ctn = xbt_dynar_new(sizeof(double),NULL);
    xbt_dynar_foreach(ctn_str,cursor, str) {
      double val = atof(str);
      xbt_dynar_push(ctn,&val);
    }
    xbt_dynar_shrink(ctn, 0);
    xbt_dynar_free(&ctn_str);
    xbt_lib_set(as_router_lib, router->id, COORD_ASR_LEVEL, (void *) ctn);
    XBT_DEBUG("Having set router coordinates for '%s'",router->id);
  }
}


/**
 * \brief Set the end points for a route
 */
static void routing_parse_S_route(void)
{
  if (src != NULL && dst != NULL && parsed_link_list != NULL)
    THROWF(arg_error, 0, "Route between %s to %s can not be defined",
           A_surfxml_route_src, A_surfxml_route_dst);
  src = A_surfxml_route_src;
  dst = A_surfxml_route_dst;
  xbt_assert(strlen(src) > 0 || strlen(dst) > 0,
             "Some limits are null in the route between \"%s\" and \"%s\"",
             src, dst);
  parsed_link_list = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
}

/**
 * \brief Set the end points and gateways for a ASroute
 */
static void routing_parse_S_ASroute(void)
{
  if (src != NULL && dst != NULL && parsed_link_list != NULL)
    THROWF(arg_error, 0, "Route between %s to %s can not be defined",
           A_surfxml_ASroute_src, A_surfxml_ASroute_dst);
  src = A_surfxml_ASroute_src;
  dst = A_surfxml_ASroute_dst;
  gw_src = A_surfxml_ASroute_gw_src;
  gw_dst = A_surfxml_ASroute_gw_dst;
  xbt_assert(strlen(src) > 0 || strlen(dst) > 0 || strlen(gw_src) > 0
             || strlen(gw_dst) > 0,
             "Some limits are null in the route between \"%s\" and \"%s\"",
             src, dst);
  parsed_link_list = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
}

/**
 * \brief Set the end points for a bypassRoute
 */
static void routing_parse_S_bypassRoute(void)
{
  if (src != NULL && dst != NULL && parsed_link_list != NULL)
    THROWF(arg_error, 0,
           "Bypass Route between %s to %s can not be defined",
           A_surfxml_bypassRoute_src, A_surfxml_bypassRoute_dst);
  src = A_surfxml_bypassRoute_src;
  dst = A_surfxml_bypassRoute_dst;
  gw_src = NULL;
  gw_dst = NULL;
  xbt_assert(strlen(src) > 0 || strlen(dst) > 0 || strlen(gw_src) > 0
             || strlen(gw_dst) > 0,
             "Some limits are null in the route between \"%s\" and \"%s\"",
             src, dst);
  parsed_link_list = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
}

/**
 * \brief Set the end points for a bypassASroute
 */
static void routing_parse_S_bypassASroute(void)
{
  if (src != NULL && dst != NULL && parsed_link_list != NULL)
    THROWF(arg_error, 0,
           "Bypass AS Route between %s to %s can not be defined",
           A_surfxml_bypassASroute_src, A_surfxml_bypassASroute_dst);
  src = A_surfxml_bypassASroute_src;
  dst = A_surfxml_bypassASroute_dst;
  gw_src = A_surfxml_bypassASroute_gw_src;
  gw_dst = A_surfxml_bypassASroute_gw_dst;
  xbt_assert(strlen(src) > 0 || strlen(dst) > 0 || strlen(gw_src) > 0
             || strlen(gw_dst) > 0,
             "Some limits are null in the route between \"%s\" and \"%s\"",
             src, dst);
  parsed_link_list = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
}
/**
 * \brief Set a new link on the actual list of link for a route or ASroute from XML
 */

static void routing_parse_link_ctn(void)
{
  char *link_id;
  switch (A_surfxml_link_ctn_direction) {
  case AU_surfxml_link_ctn_direction:
  case A_surfxml_link_ctn_direction_NONE:
    link_id = xbt_strdup(A_surfxml_link_ctn_id);
    break;
  case A_surfxml_link_ctn_direction_UP:
    link_id = bprintf("%s_UP", A_surfxml_link_ctn_id);
    break;
  case A_surfxml_link_ctn_direction_DOWN:
    link_id = bprintf("%s_DOWN", A_surfxml_link_ctn_id);
    break;
  }
  xbt_dynar_push(parsed_link_list, &link_id);
}

/**
 * \brief Store the route by calling the set_route function of the current routing component
 */
static void routing_parse_E_route(void)
{
  route_t route = xbt_new0(s_route_t, 1);
  route->link_list = parsed_link_list;
  xbt_assert(current_routing->parse_route,
             "no defined method \"set_route\" in \"%s\"",
             current_routing->name);
  current_routing->parse_route(current_routing, src, dst, route);
  generic_free_route(route);
  parsed_link_list = NULL;
  src = NULL;
  dst = NULL;
}

/**
 * \brief Store the ASroute by calling the set_ASroute function of the current routing component
 */
static void routing_parse_E_ASroute(void)
{
  route_t e_route = xbt_new0(s_route_t, 1);
  e_route->link_list = parsed_link_list;

  if (!strcmp(current_routing->model_desc->name,"RuleBased")) {
    // DIRTY PERL HACK AHEAD: with the rulebased routing, the {src,dst}_gateway fields
    // store the provided name instead of the entity directly (model_rulebased_parse_ASroute knows)
    //
    // This is because the user will provide something like "^AS_(.*)$" instead of the proper name of a given entity
    e_route->src_gateway = (sg_routing_edge_t) gw_src;
    e_route->dst_gateway = (sg_routing_edge_t) gw_dst;
  } else {
    e_route->src_gateway = sg_routing_edge_by_name_or_null(gw_src);
    e_route->dst_gateway = sg_routing_edge_by_name_or_null(gw_dst);
  }
  xbt_assert(current_routing->parse_ASroute,
             "no defined method \"set_ASroute\" in \"%s\"",
             current_routing->name);
  current_routing->parse_ASroute(current_routing, src, dst, e_route);
  generic_free_route(e_route);
  parsed_link_list = NULL;
  src = NULL;
  dst = NULL;
  gw_src = NULL;
  gw_dst = NULL;
}

/**
 * \brief Store the bypass route by calling the set_bypassroute function of the current routing component
 */
static void routing_parse_E_bypassRoute(void)
{
  route_t e_route = xbt_new0(s_route_t, 1);
  e_route->link_list = parsed_link_list;

  xbt_assert(current_routing->parse_bypassroute,
             "Bypassing mechanism not implemented by routing '%s'",
             current_routing->name);

  current_routing->parse_bypassroute(current_routing, src, dst, e_route);
  parsed_link_list = NULL;
  src = NULL;
  dst = NULL;
  gw_src = NULL;
  gw_dst = NULL;
}
/**
 * \brief Store the bypass route by calling the set_bypassroute function of the current routing component
 */
static void routing_parse_E_bypassASroute(void)
{
  route_t e_route = xbt_new0(s_route_t, 1);
  e_route->link_list = parsed_link_list;
  e_route->src_gateway = sg_routing_edge_by_name_or_null(gw_src);
  e_route->dst_gateway = sg_routing_edge_by_name_or_null(gw_dst);
  xbt_assert(current_routing->parse_bypassroute,
             "Bypassing mechanism not implemented by routing '%s'",
             current_routing->name);
  current_routing->parse_bypassroute(current_routing, src, dst, e_route);
  parsed_link_list = NULL;
  src = NULL;
  dst = NULL;
  gw_src = NULL;
  gw_dst = NULL;
}

/**
 * \brief Make a new routing component to the platform
 *
 * Add a new autonomous system to the platform. Any elements (such as host,
 * router or sub-AS) added after this call and before the corresponding call
 * to sg_platf_new_AS_close() will be added to this AS.
 *
 * Once this function was called, the configuration concerning the used
 * models cannot be changed anymore.
 *
 * @param AS_id name of this autonomous system. Must be unique in the platform
 * @param wanted_routing_type one of Full, Floyd, Dijkstra or similar. Full list in the variable routing_models, in src/surf/surf_routing.c
 */
void routing_AS_begin(const char *AS_id, const char *wanted_routing_type)
{
  AS_t new_as;
  routing_model_description_t model = NULL;
  int cpt;

  xbt_assert(!xbt_lib_get_or_null
             (as_router_lib, AS_id, ROUTING_ASR_LEVEL),
             "The AS \"%s\" already exists", AS_id);

  /* search the routing model */
  for (cpt = 0; routing_models[cpt].name; cpt++)
    if (!strcmp(wanted_routing_type, routing_models[cpt].name))
      model = &routing_models[cpt];
  /* if its not exist, error */
  if (model == NULL) {
    fprintf(stderr, "Routing model %s not found. Existing models:\n",
            wanted_routing_type);
    for (cpt = 0; routing_models[cpt].name; cpt++)
      fprintf(stderr, "   %s: %s\n", routing_models[cpt].name,
              routing_models[cpt].desc);
    xbt_die("dying");
  }

  /* make a new routing component */
  new_as = (AS_t) model->create();
  new_as->model_desc = model;
  new_as->hierarchy = SURF_ROUTING_NULL;
  new_as->name = xbt_strdup(AS_id);

  sg_routing_edge_t info = NULL;
  info = xbt_new0(s_network_element_t, 1);

  if (current_routing == NULL && global_routing->root == NULL) {

    /* it is the first one */
    new_as->routing_father = NULL;
    global_routing->root = new_as;
    info->id = -1;
  } else if (current_routing != NULL && global_routing->root != NULL) {

    xbt_assert(!xbt_dict_get_or_null
               (current_routing->routing_sons, AS_id),
               "The AS \"%s\" already exists", AS_id);
    /* it is a part of the tree */
    new_as->routing_father = current_routing;
    /* set the father behavior */
    if (current_routing->hierarchy == SURF_ROUTING_NULL)
      current_routing->hierarchy = SURF_ROUTING_RECURSIVE;
    /* add to the sons dictionary */
    xbt_dict_set(current_routing->routing_sons, AS_id,
                 (void *) new_as, NULL);
    /* add to the father element list */
    info->id = current_routing->parse_AS(current_routing, (void *) info);
  } else {
    THROWF(arg_error, 0, "All defined components must be belong to a AS");
  }

  info->rc_component = new_as->routing_father;
  info->rc_type = SURF_NETWORK_ELEMENT_AS;
  info->name = new_as->name;

  xbt_lib_set(as_router_lib, new_as->name, ROUTING_ASR_LEVEL,
              (void *) info);
  XBT_DEBUG("Having set name '%s' id '%d'",new_as->name,info->id);

  /* set the new current component of the tree */
  current_routing = new_as;
  current_routing->net_elem = info;

}

/**
 * \brief Specify that the current description of AS is finished
 *
 * Once you've declared all the content of your AS, you have to close
 * it with this call. Your AS is not usable until you call this function.
 *
 * @fixme: this call is not as robust as wanted: bad things WILL happen
 * if you call it twice for the same AS, or if you forget calling it, or
 * even if you add stuff to a closed AS
 *
 */
void routing_AS_end()
{

  if (current_routing == NULL) {
    THROWF(arg_error, 0, "Close an AS, but none was under construction");
  } else {
    if (current_routing->model_desc->end)
      current_routing->model_desc->end(current_routing);
    current_routing = current_routing->routing_father;
  }
}

/* Aux Business methods */

/**
 * \brief Get the AS father and the first elements of the chain
 *
 * \param src the source host name 
 * \param dst the destination host name
 * 
 * Get the common father of the to processing units, and the first different 
 * father in the chain
 */
static void elements_father(sg_routing_edge_t src, sg_routing_edge_t dst,
                            AS_t * res_father,
                            AS_t * res_src,
                            AS_t * res_dst)
{
  xbt_assert(src && dst, "bad parameters for \"elements_father\" method");
#define ELEMENTS_FATHER_MAXDEPTH 16     /* increase if it is not enough */
  AS_t src_as, dst_as;
  AS_t path_src[ELEMENTS_FATHER_MAXDEPTH];
  AS_t path_dst[ELEMENTS_FATHER_MAXDEPTH];
  int index_src = 0;
  int index_dst = 0;
  AS_t current;
  AS_t current_src;
  AS_t current_dst;
  AS_t father;

  /* (1) find the as where the src and dst are located */
  sg_routing_edge_t src_data = src;
  sg_routing_edge_t dst_data = dst;
  src_as = src_data->rc_component;
  dst_as = dst_data->rc_component;
#ifndef NDEBUG
  char* src_name = src_data->name;
  char* dst_name = dst_data->name;
#endif

  xbt_assert(src_as && dst_as,
             "Ask for route \"from\"(%s) or \"to\"(%s) no found", src_name, dst_name);

  /* (2) find the path to the root routing component */
  for (current = src_as; current != NULL; current = current->routing_father) {
    if (index_src >= ELEMENTS_FATHER_MAXDEPTH)
      xbt_die("ELEMENTS_FATHER_MAXDEPTH should be increased for path_src");
    path_src[index_src++] = current;
  }
  for (current = dst_as; current != NULL; current = current->routing_father) {
    if (index_dst >= ELEMENTS_FATHER_MAXDEPTH)
      xbt_die("ELEMENTS_FATHER_MAXDEPTH should be increased for path_dst");
    path_dst[index_dst++] = current;
  }

  /* (3) find the common father */
  do {
    current_src = path_src[--index_src];
    current_dst = path_dst[--index_dst];
  } while (index_src > 0 && index_dst > 0 && current_src == current_dst);

  /* (4) they are not in the same routing component, make the path */
  if (current_src == current_dst)
    father = current_src;
  else
    father = path_src[index_src + 1];

  /* (5) result generation */
  *res_father = father;         /* first the common father of src and dst */
  *res_src = current_src;       /* second the first different father of src */
  *res_dst = current_dst;       /* three  the first different father of dst */

#undef ELEMENTS_FATHER_MAXDEPTH
}

/* Global Business methods */

/**
 * \brief Recursive function for get_route_latency
 *
 * \param src the source host name 
 * \param dst the destination host name
 * \param *route the route where the links are stored. It is either NULL or a ready to use dynar
 * \param *latency the latency, if needed
 * 
 * This function is called by "get_route" and "get_latency". It allows to walk
 * recursively through the ASes tree.
 */
static void _get_route_and_latency(sg_routing_edge_t src, sg_routing_edge_t dst,
                                   xbt_dynar_t * links, double *latency)
{
  s_route_t route;
  memset(&route,0,sizeof(route));

  XBT_DEBUG("Solve route/latency  \"%s\" to \"%s\"", src->name, dst->name);
  xbt_assert(src && dst, "bad parameters for \"_get_route_latency\" method");

  /* Find how src and dst are interconnected */
  AS_t common_father, src_father, dst_father;
  elements_father(src, dst, &common_father, &src_father, &dst_father);
  XBT_DEBUG("elements_father: common father '%s' src_father '%s' dst_father '%s'",
      common_father->name,src_father->name,dst_father->name);

  /* Check whether a direct bypass is defined */
  route_t e_route_bypass = NULL;
  if (common_father->get_bypass_route)
    e_route_bypass = common_father->get_bypass_route(common_father, src, dst, latency);

  /* Common ancestor is kind enough to declare a bypass route from src to dst -- use it and bail out */
  if (e_route_bypass) {
    xbt_dynar_merge(links,&(e_route_bypass->link_list));
    generic_free_route(e_route_bypass);
    return;
  }

  /* If src and dst are in the same AS, life is good */
  if (src_father == dst_father) {       /* SURF_ROUTING_BASE */

    route.link_list = *links;

    common_father->get_route_and_latency(common_father, src, dst, &route,latency);
    return;
  }

  /* Not in the same AS, no bypass. We'll have to find our path between the ASes recursively*/

  route.link_list = xbt_dynar_new(sizeof(sg_routing_link_t), NULL);
  // Find the net_card corresponding to father
  sg_routing_edge_t src_father_net_elm = src_father->net_elem;
  sg_routing_edge_t dst_father_net_elm = dst_father->net_elem;

  common_father->get_route_and_latency(common_father, src_father_net_elm, dst_father_net_elm, &route,latency);

  xbt_assert((route.src_gateway != NULL) && (route.dst_gateway != NULL),
      "bad gateways for route from \"%s\" to \"%s\"", src->name, dst->name);

  sg_routing_edge_t src_gateway_net_elm = route.src_gateway;
  sg_routing_edge_t dst_gateway_net_elm = route.dst_gateway;

  /* If source gateway is not our source, we have to recursively find our way up to this point */
  if (src != src_gateway_net_elm)
    _get_route_and_latency(src, src_gateway_net_elm, links, latency);

  xbt_dynar_merge(links,&(route.link_list));

  /* If dest gateway is not our destination, we have to recursively find our way from this point */
  // FIXME why can't I factorize it the same way than [src;src_gw] without breaking the examples??
  if (dst_gateway_net_elm != dst) {
    _get_route_and_latency(dst_gateway_net_elm, dst, links, latency);
  }
  xbt_dynar_free(&route.link_list);
}

/**
 * \brief Find a route between hosts
 *
 * \param src the network_element_t for src host
 * \param dst the network_element_t for dst host
 * \param route where to store the list of links.
 *              If *route=NULL, create a short lived dynar. Else, fill the provided dynar
 * \param latency where to store the latency experienced on the path (or NULL if not interested)
 *                It is the caller responsability to initialize latency to 0 (we add to provided route)
 * \pre route!=NULL
 *
 * walk through the routing components tree and find a route between hosts
 * by calling the differents "get_route" functions in each routing component.
 */
void routing_get_route_and_latency(sg_routing_edge_t src,
                                   sg_routing_edge_t dst,
                                   xbt_dynar_t * route, double *latency)
{
  XBT_DEBUG("routing_get_route_and_latency from %s to %s",src->name,dst->name);
  if (!*route) {
    xbt_dynar_reset(global_routing->last_route);
    *route = global_routing->last_route;
  }

  _get_route_and_latency(src, dst, route, latency);

  xbt_assert(!latency || *latency >= 0.0,
             "negative latency on route between \"%s\" and \"%s\"", src->name, dst->name);
}

static xbt_dynar_t recursive_get_onelink_routes(AS_t rc)
{
  xbt_dynar_t ret = xbt_dynar_new(sizeof(onelink_t), xbt_free);

  //adding my one link routes
  xbt_dynar_t onelink_mine = rc->get_onelink_routes(rc);
  if (onelink_mine)
    xbt_dynar_merge(&ret,&onelink_mine);

  //recursing
  char *key;
  xbt_dict_cursor_t cursor = NULL;
  AS_t rc_child;
  xbt_dict_foreach(rc->routing_sons, cursor, key, rc_child) {
    xbt_dynar_t onelink_child = recursive_get_onelink_routes(rc_child);
    if (onelink_child)
      xbt_dynar_merge(&ret,&onelink_child);
  }
  return ret;
}

static xbt_dynar_t get_onelink_routes(void)
{
  return recursive_get_onelink_routes(global_routing->root);
}

e_surf_network_element_type_t routing_get_network_element_type(const char *name)
{
  sg_routing_edge_t rc = sg_routing_edge_by_name_or_null(name);
  if (rc)
    return rc->rc_type;

  return SURF_NETWORK_ELEMENT_NULL;
}

/**
 * \brief Generic method: create the global routing schema
 * 
 * Make a global routing structure and set all the parsing functions.
 */
void routing_model_create( void *loopback)
{
  /* config the uniq global routing */
  global_routing = xbt_new0(s_routing_global_t, 1);
  global_routing->root = NULL;
  global_routing->get_onelink_routes = get_onelink_routes;
  global_routing->loopback = loopback;
  global_routing->last_route = xbt_dynar_new(sizeof(sg_routing_link_t),NULL);
  /* no current routing at moment */
  current_routing = NULL;
}


/* ************************************************** */
/* ********** PATERN FOR NEW ROUTING **************** */

/* The minimal configuration of a new routing model need the next functions,
 * also you need to set at the start of the file, the new model in the model
 * list. Remember keep the null ending of the list.
 */
/*** Routing model structure ***/
// typedef struct {
//   s_routing_component_t generic_routing;
//   /* things that your routing model need */
// } s_routing_component_NEW_t,*routing_component_NEW_t;

/*** Parse routing model functions ***/
// static void model_NEW_set_processing_unit(routing_component_t rc, const char* name) {}
// static void model_NEW_set_autonomous_system(routing_component_t rc, const char* name) {}
// static void model_NEW_set_route(routing_component_t rc, const char* src, const char* dst, route_t route) {}
// static void model_NEW_set_ASroute(routing_component_t rc, const char* src, const char* dst, route_extended_t route) {}
// static void model_NEW_set_bypassroute(routing_component_t rc, const char* src, const char* dst, route_extended_t e_route) {}

/*** Business methods ***/
// static route_extended_t NEW_get_route(routing_component_t rc, const char* src,const char* dst) {return NULL;}
// static route_extended_t NEW_get_bypass_route(routing_component_t rc, const char* src,const char* dst) {return NULL;}
// static void NEW_finalize(routing_component_t rc) { xbt_free(rc);}

/*** Creation routing model functions ***/
// static void* model_NEW_create(void) {
//   routing_component_NEW_t new_component =  xbt_new0(s_routing_component_NEW_t,1);
//   new_component->generic_routing.set_processing_unit = model_NEW_set_processing_unit;
//   new_component->generic_routing.set_autonomous_system = model_NEW_set_autonomous_system;
//   new_component->generic_routing.set_route = model_NEW_set_route;
//   new_component->generic_routing.set_ASroute = model_NEW_set_ASroute;
//   new_component->generic_routing.set_bypassroute = model_NEW_set_bypassroute;
//   new_component->generic_routing.get_route = NEW_get_route;
//   new_component->generic_routing.get_bypass_route = NEW_get_bypass_route;
//   new_component->generic_routing.finalize = NEW_finalize;
//   /* initialization of internal structures */
//   return new_component;
// } /* mandatory */
// static void  model_NEW_load(void) {}   /* mandatory */
// static void  model_NEW_unload(void) {} /* mandatory */
// static void  model_NEW_end(void) {}    /* mandatory */

/* ************************************************************************** */
/* ************************* GENERIC PARSE FUNCTIONS ************************ */

static void routing_parse_cluster(sg_platf_cluster_cbarg_t cluster)
{
  char *host_id, *groups, *link_id = NULL;
  xbt_dict_t patterns = NULL;

  s_sg_platf_host_cbarg_t host;
  s_sg_platf_link_cbarg_t link;

  unsigned int iter;
  int start, end, i;
  xbt_dynar_t radical_elements;
  xbt_dynar_t radical_ends;

  if (strcmp(cluster->availability_trace, "")
      || strcmp(cluster->state_trace, "")) {
    patterns = xbt_dict_new_homogeneous(xbt_free_f);
    xbt_dict_set(patterns, "id", xbt_strdup(cluster->id), NULL);
    xbt_dict_set(patterns, "prefix", xbt_strdup(cluster->prefix), NULL);
    xbt_dict_set(patterns, "suffix", xbt_strdup(cluster->suffix), NULL);
  }

  XBT_DEBUG("<AS id=\"%s\"\trouting=\"Cluster\">", cluster->id);
  sg_platf_new_AS_begin(cluster->id, "Cluster");

  current_routing->link_up_down_list
            = xbt_dynar_new(sizeof(s_surf_parsing_link_up_down_t),NULL);

  //Make all hosts
  radical_elements = xbt_str_split(cluster->radical, ",");
  xbt_dynar_foreach(radical_elements, iter, groups) {

    radical_ends = xbt_str_split(groups, "-");
    start = surf_parse_get_int(xbt_dynar_get_as(radical_ends, 0, char *));

    switch (xbt_dynar_length(radical_ends)) {
    case 1:
      end = start;
      break;
    case 2:
      end = surf_parse_get_int(xbt_dynar_get_as(radical_ends, 1, char *));
      break;
    default:
      surf_parse_error("Malformed radical");
      break;
    }
    for (i = start; i <= end; i++) {
      host_id =
          bprintf("%s%d%s", cluster->prefix, i, cluster->suffix);
      link_id = bprintf("%s_link_%d", cluster->id, i);

      XBT_DEBUG("<host\tid=\"%s\"\tpower=\"%f\">", host_id, cluster->power);

      memset(&host, 0, sizeof(host));
      host.id = host_id;
      if (strcmp(cluster->availability_trace, "")) {
        xbt_dict_set(patterns, "radical", bprintf("%d", i), NULL);
        char *avail_file = xbt_str_varsubst(cluster->availability_trace, patterns);
        XBT_DEBUG("\tavailability_file=\"%s\"", avail_file);
        host.power_trace = tmgr_trace_new(avail_file);
        xbt_free(avail_file);
      } else {
        XBT_DEBUG("\tavailability_file=\"\"");
      }

      if (strcmp(cluster->state_trace, "")) {
        char *avail_file = xbt_str_varsubst(cluster->state_trace, patterns);
        XBT_DEBUG("\tstate_file=\"%s\"", avail_file);
        host.state_trace = tmgr_trace_new(avail_file);
        xbt_free(avail_file);
      } else {
        XBT_DEBUG("\tstate_file=\"\"");
      }

      host.power_peak = cluster->power;
      host.power_scale = 1.0;
      host.core_amount = cluster->core_amount;
      host.initial_state = SURF_RESOURCE_ON;
      host.coord = "";
      sg_platf_new_host(&host);
      XBT_DEBUG("</host>");

      XBT_DEBUG("<link\tid=\"%s\"\tbw=\"%f\"\tlat=\"%f\"/>", link_id,
                cluster->bw, cluster->lat);

      memset(&link, 0, sizeof(link));
      link.id = link_id;
      link.bandwidth = cluster->bw;
      link.latency = cluster->lat;
      link.state = SURF_RESOURCE_ON;
      link.policy = cluster->sharing_policy;
      sg_platf_new_link(&link);

      s_surf_parsing_link_up_down_t info;

      if (link.policy == SURF_LINK_FULLDUPLEX) {
        char *tmp_link = bprintf("%s_UP", link_id);
        info.link_up =
            xbt_lib_get_or_null(link_lib, tmp_link, SURF_LINK_LEVEL);
        free(tmp_link);
        tmp_link = bprintf("%s_DOWN", link_id);
        info.link_down =
            xbt_lib_get_or_null(link_lib, tmp_link, SURF_LINK_LEVEL);
        free(tmp_link);
      } else {
        info.link_up = xbt_lib_get_or_null(link_lib, link_id, SURF_LINK_LEVEL);
        info.link_down = info.link_up;
      }
      xbt_dynar_push(current_routing->link_up_down_list,&info);
      xbt_free(link_id);
      xbt_free(host_id);
    }

    xbt_dynar_free(&radical_ends);
  }
  xbt_dynar_free(&radical_elements);

  // Add a router. It is magically used thanks to the way in which surf_routing_cluster is written,
  // and it's very useful to connect clusters together
  XBT_DEBUG(" ");
  XBT_DEBUG("<router id=\"%s\"/>", cluster->router_id);
  char *newid = NULL;
  s_sg_platf_router_cbarg_t router;
  memset(&router, 0, sizeof(router));
  router.id = cluster->router_id;
  router.coord = "";
  if (!router.id || !strcmp(router.id, ""))
    router.id = newid =
        bprintf("%s%s_router%s", cluster->prefix, cluster->id,
                cluster->suffix);
  sg_platf_new_router(&router);
  free(newid);

  //Make the backbone
  if ((cluster->bb_bw != 0) && (cluster->bb_lat != 0)) {
    char *link_backbone = bprintf("%s_backbone", cluster->id);
    XBT_DEBUG("<link\tid=\"%s\" bw=\"%f\" lat=\"%f\"/>", link_backbone,
              cluster->bb_bw, cluster->bb_lat);

    memset(&link, 0, sizeof(link));
    link.id = link_backbone;
    link.bandwidth = cluster->bb_bw;
    link.latency = cluster->bb_lat;
    link.state = SURF_RESOURCE_ON;
    link.policy = cluster->bb_sharing_policy;

    sg_platf_new_link(&link);

    surf_routing_cluster_add_backbone(current_routing, xbt_lib_get_or_null(link_lib, link_backbone, SURF_LINK_LEVEL));

    free(link_backbone);
  }

  XBT_DEBUG("</AS>");
  sg_platf_new_AS_end();
  XBT_DEBUG(" ");
  xbt_dict_free(&patterns); // no op if it were never set
}

static void routing_parse_postparse(void) {
  xbt_dict_free(&random_value);
}

static void routing_parse_peer(sg_platf_peer_cbarg_t peer)
{
  static int AX_ptr = 0;
  char *host_id = NULL;
  char *router_id, *link_router, *link_backbone, *link_id_up, *link_id_down;

  static unsigned int surfxml_buffer_stack_stack_ptr = 1;
  static unsigned int surfxml_buffer_stack_stack[1024];

  surfxml_buffer_stack_stack[0] = 0;

  surfxml_bufferstack_push(1);

  XBT_DEBUG("<AS id=\"%s\"\trouting=\"Full\">", peer->id);
  sg_platf_new_AS_begin(peer->id, "Full");

  XBT_DEBUG(" ");
  host_id = HOST_PEER(peer->id);
  router_id = ROUTER_PEER(peer->id);
  link_id_up = LINK_UP_PEER(peer->id);
  link_id_down = LINK_DOWN_PEER(peer->id);

  link_router = bprintf("%s_link_router", peer->id);
  link_backbone = bprintf("%s_backbone", peer->id);

  XBT_DEBUG("<host\tid=\"%s\"\tpower=\"%f\"/>", host_id, peer->power);
  s_sg_platf_host_cbarg_t host;
  memset(&host, 0, sizeof(host));
  host.initial_state = SURF_RESOURCE_ON;
  host.id = host_id;
  host.power_peak = peer->power;
  host.power_scale = 1.0;
  host.power_trace = peer->availability_trace;
  host.state_trace = peer->state_trace;
  host.core_amount = 1;
  host.coord = peer->coord;
  sg_platf_new_host(&host);


  XBT_DEBUG("<router id=\"%s\"\tcoordinates=\"%s\"/>", router_id, peer->coord);
  s_sg_platf_router_cbarg_t router;
  memset(&router, 0, sizeof(router));
  router.id = router_id;
  router.coord = peer->coord;
  sg_platf_new_router(&router);

  XBT_DEBUG("<link\tid=\"%s\"\tbw=\"%f\"\tlat=\"%f\"/>", link_id_up,
            peer->bw_in, peer->lat);
  s_sg_platf_link_cbarg_t link;
  memset(&link, 0, sizeof(link));
  link.state = SURF_RESOURCE_ON;
  link.policy = SURF_LINK_SHARED;
  link.id = link_id_up;
  link.bandwidth = peer->bw_in;
  link.latency = peer->lat;
  sg_platf_new_link(&link);

  // FIXME: dealing with full duplex is not the role of this piece of code, I'd say [Mt]
  // Instead, it should be created fullduplex, and the models will do what's needed in this case
  XBT_DEBUG("<link\tid=\"%s\"\tbw=\"%f\"\tlat=\"%f\"/>", link_id_down,
            peer->bw_out, peer->lat);
  link.id = link_id_down;
  sg_platf_new_link(&link);

  XBT_DEBUG(" ");

  // begin here
  XBT_DEBUG("<route\tsrc=\"%s\"\tdst=\"%s\"", host_id, router_id);
  XBT_DEBUG("symmetrical=\"NO\">");
  SURFXML_BUFFER_SET(route_src, host_id);
  SURFXML_BUFFER_SET(route_dst, router_id);
  A_surfxml_route_symmetrical = A_surfxml_route_symmetrical_NO;
  SURFXML_START_TAG(route);

  XBT_DEBUG("<link_ctn\tid=\"%s\"/>", link_id_up);
  SURFXML_BUFFER_SET(link_ctn_id, link_id_up);
  A_surfxml_link_ctn_direction = A_surfxml_link_ctn_direction_NONE;
  SURFXML_START_TAG(link_ctn);
  SURFXML_END_TAG(link_ctn);

  XBT_DEBUG("</route>");
  SURFXML_END_TAG(route);

  //Opposite Route
  XBT_DEBUG("<route\tsrc=\"%s\"\tdst=\"%s\"", router_id, host_id);
  XBT_DEBUG("symmetrical=\"NO\">");
  SURFXML_BUFFER_SET(route_src, router_id);
  SURFXML_BUFFER_SET(route_dst, host_id);
  A_surfxml_route_symmetrical = A_surfxml_route_symmetrical_NO;
  SURFXML_START_TAG(route);

  XBT_DEBUG("<link_ctn\tid=\"%s\"/>", link_id_down);
  SURFXML_BUFFER_SET(link_ctn_id, link_id_down);
  A_surfxml_link_ctn_direction = A_surfxml_link_ctn_direction_NONE;
  SURFXML_START_TAG(link_ctn);
  SURFXML_END_TAG(link_ctn);

  XBT_DEBUG("</route>");
  SURFXML_END_TAG(route);

  XBT_DEBUG("</AS>");
  sg_platf_new_AS_end();
  XBT_DEBUG(" ");

  //xbt_dynar_free(&tab_elements_num);
  free(host_id);
  free(router_id);
  free(link_router);
  free(link_backbone);
  free(link_id_up);
  free(link_id_down);
  surfxml_bufferstack_pop(1);
}

static void routing_parse_Srandom(void)
{
  double mean, std, min, max, seed;
  char *random_id = A_surfxml_random_id;
  char *random_radical = A_surfxml_random_radical;
  char *rd_name = NULL;
  char *rd_value;
  mean = surf_parse_get_double(A_surfxml_random_mean);
  std = surf_parse_get_double(A_surfxml_random_std_deviation);
  min = surf_parse_get_double(A_surfxml_random_min);
  max = surf_parse_get_double(A_surfxml_random_max);
  seed = surf_parse_get_double(A_surfxml_random_seed);

  double res = 0;
  int i = 0;
  random_data_t random = xbt_new0(s_random_data_t, 1);
  char *tmpbuf;

  xbt_dynar_t radical_elements;
  unsigned int iter;
  char *groups;
  int start, end;
  xbt_dynar_t radical_ends;

  switch (A_surfxml_random_generator) {
  case AU_surfxml_random_generator:
  case A_surfxml_random_generator_NONE:
    random->generator = NONE;
    break;
  case A_surfxml_random_generator_DRAND48:
    random->generator = DRAND48;
    break;
  case A_surfxml_random_generator_RAND:
    random->generator = RAND;
    break;
  case A_surfxml_random_generator_RNGSTREAM:
    random->generator = RNGSTREAM;
    break;
  default:
    surf_parse_error("Invalid random generator");
    break;
  }
  random->seed = seed;
  random->min = min;
  random->max = max;

  /* Check user stupidities */
  if (max < min)
    THROWF(arg_error, 0, "random->max < random->min (%f < %f)", max, min);
  if (mean < min)
    THROWF(arg_error, 0, "random->mean < random->min (%f < %f)", mean, min);
  if (mean > max)
    THROWF(arg_error, 0, "random->mean > random->max (%f > %f)", mean, max);

  /* normalize the mean and standard deviation before storing */
  random->mean = (mean - min) / (max - min);
  random->std = std / (max - min);

  if (random->mean * (1 - random->mean) < random->std * random->std)
    THROWF(arg_error, 0, "Invalid mean and standard deviation (%f and %f)",
           random->mean, random->std);

  XBT_DEBUG
      ("id = '%s' min = '%f' max = '%f' mean = '%f' std_deviatinon = '%f' generator = '%d' seed = '%ld' radical = '%s'",
       random_id, random->min, random->max, random->mean, random->std,
       (int)random->generator, random->seed, random_radical);

  if (!random_value)
    random_value = xbt_dict_new_homogeneous(free);

  if (!strcmp(random_radical, "")) {
    res = random_generate(random);
    rd_value = bprintf("%f", res);
    xbt_dict_set(random_value, random_id, rd_value, NULL);
  } else {
    radical_elements = xbt_str_split(random_radical, ",");
    xbt_dynar_foreach(radical_elements, iter, groups) {
      radical_ends = xbt_str_split(groups, "-");
      switch (xbt_dynar_length(radical_ends)) {
      case 1:
        xbt_assert(!xbt_dict_get_or_null(random_value, random_id),
                   "Custom Random '%s' already exists !", random_id);
        res = random_generate(random);
        tmpbuf =
            bprintf("%s%d", random_id,
                    atoi(xbt_dynar_getfirst_as(radical_ends, char *)));
        xbt_dict_set(random_value, tmpbuf, bprintf("%f", res), NULL);
        xbt_free(tmpbuf);
        break;

      case 2:
        start = surf_parse_get_int(xbt_dynar_get_as(radical_ends, 0, char *));
        end = surf_parse_get_int(xbt_dynar_get_as(radical_ends, 1, char *));
        for (i = start; i <= end; i++) {
          xbt_assert(!xbt_dict_get_or_null(random_value, random_id),
                     "Custom Random '%s' already exists !", bprintf("%s%d",
                                                                    random_id,
                                                                    i));
          res = random_generate(random);
          tmpbuf = bprintf("%s%d", random_id, i);
          xbt_dict_set(random_value, tmpbuf, bprintf("%f", res), NULL);
          xbt_free(tmpbuf);
        }
        break;
      default:
        XBT_CRITICAL("Malformed radical");
        break;
      }
      res = random_generate(random);
      rd_name = bprintf("%s_router", random_id);
      rd_value = bprintf("%f", res);
      xbt_dict_set(random_value, rd_name, rd_value, NULL);

      xbt_dynar_free(&radical_ends);
    }
    free(rd_name);
    xbt_dynar_free(&radical_elements);
  }
}

void routing_register_callbacks()
{
  sg_platf_host_add_cb(parse_S_host);
  sg_platf_router_add_cb(parse_S_router);

  surfxml_add_callback(STag_surfxml_random_cb_list, &routing_parse_Srandom);

  surfxml_add_callback(STag_surfxml_route_cb_list, &routing_parse_S_route);
  surfxml_add_callback(STag_surfxml_ASroute_cb_list, &routing_parse_S_ASroute);
  surfxml_add_callback(STag_surfxml_bypassRoute_cb_list,
                       &routing_parse_S_bypassRoute);
  surfxml_add_callback(STag_surfxml_bypassASroute_cb_list,
                       &routing_parse_S_bypassASroute);

  surfxml_add_callback(ETag_surfxml_link_ctn_cb_list, &routing_parse_link_ctn);

  surfxml_add_callback(ETag_surfxml_route_cb_list, &routing_parse_E_route);
  surfxml_add_callback(ETag_surfxml_ASroute_cb_list, &routing_parse_E_ASroute);
  surfxml_add_callback(ETag_surfxml_bypassRoute_cb_list,
                       &routing_parse_E_bypassRoute);
  surfxml_add_callback(ETag_surfxml_bypassASroute_cb_list,
                       &routing_parse_E_bypassASroute);

  sg_platf_cluster_add_cb(routing_parse_cluster);

  sg_platf_peer_add_cb(routing_parse_peer);
  sg_platf_postparse_add_cb(routing_parse_postparse);

  /* we care about the ASes while parsing the platf. Incredible, isnt it? */
  sg_platf_AS_end_add_cb(routing_AS_end);
  sg_platf_AS_begin_add_cb(routing_AS_begin);

#ifdef HAVE_TRACING
  instr_routing_define_callbacks();
#endif
}

/**
 * \brief Recursive function for finalize
 *
 * \param rc the source host name
 *
 * This fuction is call by "finalize". It allow to finalize the
 * AS or routing components. It delete all the structures.
 */
static void finalize_rec(AS_t as) {
  xbt_dict_cursor_t cursor = NULL;
  char *key;
  AS_t elem;

  xbt_dict_foreach(as->routing_sons, cursor, key, elem) {
    finalize_rec(elem);
  }

  as->finalize(as);
}

/** \brief Frees all memory allocated by the routing module */
void routing_exit(void) {
  if (!global_routing)
    return;
  xbt_dynar_free(&global_routing->last_route);
  finalize_rec(global_routing->root);
  xbt_free(global_routing);
}
