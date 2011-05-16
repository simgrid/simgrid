/* Copyright (c) 2009, 2010. The SimGrid Team. 
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifdef HAVE_PCRE_LIB
#include <pcre.h>               /* regular expression library */
#endif
#include "surf_routing_private.h"
#include "surf/surf_routing.h"

xbt_lib_t host_lib;
int ROUTING_HOST_LEVEL; //Routing level
int	SURF_CPU_LEVEL;		//Surf cpu level
int SURF_WKS_LEVEL;		//Surf workstation level
int SIMIX_HOST_LEVEL;	//Simix level
int	MSG_HOST_LEVEL;		//Msg level
int	SD_HOST_LEVEL;		//Simdag level
int	COORD_HOST_LEVEL;	//Coordinates level
int NS3_HOST_LEVEL;		//host node for ns3

xbt_lib_t link_lib;
int SD_LINK_LEVEL;		//Simdag level
int SURF_LINK_LEVEL;	//Surf level
int NS3_LINK_LEVEL;		//link for ns3

xbt_lib_t as_router_lib;
int ROUTING_ASR_LEVEL;	//Routing level
int COORD_ASR_LEVEL;	//Coordinates level
int NS3_ASR_LEVEL;		//host node for ns3

static xbt_dict_t patterns = NULL;
static xbt_dict_t random_value = NULL;

/* Global vars */
routing_global_t global_routing = NULL;
routing_component_t current_routing = NULL;
model_type_t current_routing_model = NULL;

/* global parse functions */
xbt_dynar_t link_list = NULL;    /* temporary store of current list link of a route */
static const char *src = NULL;        /* temporary store the source name of a route */
static const char *dst = NULL;        /* temporary store the destination name of a route */
static char *gw_src = NULL;     /* temporary store the gateway source name of a route */
static char *gw_dst = NULL;     /* temporary store the gateway destination name of a route */
static double_f_cpvoid_t get_link_latency = NULL;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route, surf, "Routing part of surf");

static void routing_parse_Speer(void);          /* peer bypass */
static void routing_parse_Srandom(void);        /* random bypass */
static void routing_parse_Erandom(void);        /* random bypass */

static void routing_parse_Sconfig(void);        /* config Tag */
static void routing_parse_Econfig(void);        /* config Tag */

static char* replace_random_parameter(char * chaine);
static void clean_dict_random(void);

/* this lines are only for replace use like index in the model table */
typedef enum {
  SURF_MODEL_FULL = 0,
  SURF_MODEL_FLOYD,
  SURF_MODEL_DIJKSTRA,
  SURF_MODEL_DIJKSTRACACHE,
  SURF_MODEL_NONE,
#ifdef HAVE_PCRE_LIB
  SURF_MODEL_RULEBASED
#endif
} e_routing_types;

struct s_model_type routing_models[] = { {"Full",
                                          "Full routing data (fast, large memory requirements, fully expressive)",
                                          model_full_create,
                                          model_full_load,
                                          model_full_unload,
                                          model_full_end},
{"Floyd",
 "Floyd routing data (slow initialization, fast lookup, lesser memory requirements, shortest path routing only)",
 model_floyd_create, model_floyd_load, model_floyd_unload,
 model_floyd_end},
{"Dijkstra",
 "Dijkstra routing data (fast initialization, slow lookup, small memory requirements, shortest path routing only)",
 model_dijkstra_create, model_dijkstra_both_load,
 model_dijkstra_both_unload, model_dijkstra_both_end},
{"DijkstraCache",
 "Dijkstra routing data (fast initialization, fast lookup, small memory requirements, shortest path routing only)",
 model_dijkstracache_create, model_dijkstra_both_load,
 model_dijkstra_both_unload, model_dijkstra_both_end},
{"none", "No routing (usable with Constant network only)",
 model_none_create, model_none_load, model_none_unload, model_none_end},
#ifdef HAVE_PCRE_LIB
{"RuleBased", "Rule-Based routing data (...)", model_rulebased_create,
 model_rulebased_load, model_rulebased_unload, model_rulebased_end},
{"Vivaldi", "Vivaldi routing", model_rulebased_create,
  model_rulebased_load, model_rulebased_unload, model_rulebased_end},
#endif
{NULL, NULL, NULL, NULL, NULL, NULL}
};

static double euclidean_dist_comp(int index, xbt_dynar_t src, xbt_dynar_t dst)
{
	double src_coord, dst_coord;

	src_coord = atof(xbt_dynar_get_as(src, index, char *));
	dst_coord = atof(xbt_dynar_get_as(dst, index, char *));

	return (src_coord-dst_coord)*(src_coord-dst_coord);

}

static double base_vivaldi_get_latency (const char *src, const char *dst)
{
  double euclidean_dist;
  xbt_dynar_t src_ctn, dst_ctn;
  src_ctn = xbt_lib_get_or_null(host_lib, src, COORD_HOST_LEVEL);
  if(!src_ctn) src_ctn = xbt_lib_get_or_null(as_router_lib, src, COORD_ASR_LEVEL);
  dst_ctn = xbt_lib_get_or_null(host_lib, dst, COORD_HOST_LEVEL);
  if(!dst_ctn) dst_ctn = xbt_lib_get_or_null(as_router_lib, dst, COORD_ASR_LEVEL);

  if(dst_ctn == NULL || src_ctn == NULL)
  xbt_die("Coord src '%s' :%p   dst '%s' :%p",src,src_ctn,dst,dst_ctn);

  euclidean_dist = sqrt (euclidean_dist_comp(0,src_ctn,dst_ctn)+euclidean_dist_comp(1,src_ctn,dst_ctn))
  				 + fabs(atof(xbt_dynar_get_as(src_ctn, 2, char *)))+fabs(atof(xbt_dynar_get_as(dst_ctn, 2, char *)));

  xbt_assert(euclidean_dist>=0, "Euclidean Dist is less than 0\"%s\" and \"%.2f\"", src, euclidean_dist);

  return euclidean_dist;
}

static double vivaldi_get_link_latency (routing_component_t rc,const char *src, const char *dst, route_extended_t e_route)
{
  if(get_network_element_type(src) == SURF_NETWORK_ELEMENT_AS) {
	  int need_to_clean = e_route?0:1;
	  double latency;
	  e_route = e_route?e_route:(*(rc->get_route)) (rc, src, dst);
	  latency = base_vivaldi_get_latency(e_route->src_gateway,e_route->dst_gateway);
	  if(need_to_clean) generic_free_extended_route(e_route);
	  return latency;
  } else {
	  return base_vivaldi_get_latency(src,dst);
  }
}

/**
 * \brief Add a "host" to the network element list
 */
static void parse_S_host(const char *host_id, const char* coord)
{
  network_element_info_t info = NULL;
  if (current_routing->hierarchy == SURF_ROUTING_NULL)
    current_routing->hierarchy = SURF_ROUTING_BASE;
  xbt_assert(!xbt_lib_get_or_null(host_lib, host_id,ROUTING_HOST_LEVEL),
              "Reading a host, processing unit \"%s\" already exists",
              host_id);
  xbt_assert(current_routing->set_processing_unit,
              "no defined method \"set_processing_unit\" in \"%s\"",
              current_routing->name);
  (*(current_routing->set_processing_unit)) (current_routing, host_id);
  info = xbt_new0(s_network_element_info_t, 1);
  info->rc_component = current_routing;
  info->rc_type = SURF_NETWORK_ELEMENT_HOST;
  xbt_lib_set(host_lib,host_id,ROUTING_HOST_LEVEL,(void *) info);
  if (strcmp(coord,"")) {
	if(!COORD_HOST_LEVEL) xbt_die("To use coordinates, you must set configuration 'coordinates' to 'yes'");
    xbt_dynar_t ctn = xbt_str_split_str(coord, " ");
    xbt_dynar_shrink(ctn, 0);
    xbt_lib_set(host_lib,host_id,COORD_HOST_LEVEL,(void *) ctn);
  }
}

static void parse_E_host(void)
{
	 xbt_dict_cursor_t cursor = NULL;
	  char *key;
	  char *elem;

	  xbt_dict_foreach(current_property_set, cursor, key, elem) {
		  XBT_DEBUG("property : %s = %s",key,elem);
		}
}

/*
 * \brief Add a host to the network element list from XML
 */
static void parse_S_host_XML(void)
{
	parse_S_host(A_surfxml_host_id, A_surfxml_host_coordinates);
}
static void parse_E_host_XML(void)
{
	parse_E_host();
}

/*
 * \brief Add a host to the network element list from lua script
 */
static void parse_S_host_lua(const char *host_id, const char *coord)
{
  parse_S_host(host_id, coord);
}


/**
 * \brief Add a "router" to the network element list
 */
static void parse_S_router(const char *router_id)
{
  network_element_info_t info = NULL;

  if (current_routing->hierarchy == SURF_ROUTING_NULL)
    current_routing->hierarchy = SURF_ROUTING_BASE;
  xbt_assert(!xbt_lib_get_or_null(as_router_lib,A_surfxml_router_id, ROUTING_ASR_LEVEL),
              "Reading a router, processing unit \"%s\" already exists",
              router_id);
  xbt_assert(current_routing->set_processing_unit,
              "no defined method \"set_processing_unit\" in \"%s\"",
              current_routing->name);
  (*(current_routing->set_processing_unit)) (current_routing,
                                             router_id);
  info = xbt_new0(s_network_element_info_t, 1);
  info->rc_component = current_routing;
  info->rc_type = SURF_NETWORK_ELEMENT_ROUTER;

  xbt_lib_set(as_router_lib,router_id,ROUTING_ASR_LEVEL,(void *) info);
  if (strcmp(A_surfxml_router_coordinates,"")) {
	if(!COORD_ASR_LEVEL) xbt_die("To use coordinates, you must set configuration 'coordinates' to 'yes'");
    xbt_dynar_t ctn = xbt_str_split_str(A_surfxml_router_coordinates, " ");
    xbt_dynar_shrink(ctn, 0);
    xbt_lib_set(as_router_lib,router_id,COORD_ASR_LEVEL,(void *) ctn);
  }
}

/**
 * brief Add a "router" to the network element list from XML description
 */
static void parse_S_router_XML(void)
{
	return parse_S_router(A_surfxml_router_id);
}

/**
 * brief Add a "router" to the network element list from XML description
 */
static void parse_S_router_lua(const char* router_id)
{
	return parse_S_router(router_id);
}

/**
 * \brief Set the endponints for a route
 */
static void parse_S_route_new_and_endpoints(const char *src_id, const char *dst_id)
{
  if (src != NULL && dst != NULL && link_list != NULL)
    THROWF(arg_error, 0, "Route between %s to %s can not be defined",
           src_id, dst_id);
  src = src_id;
  dst = dst_id;
  xbt_assert(strlen(src) > 0 || strlen(dst) > 0,
              "Some limits are null in the route between \"%s\" and \"%s\"",
              src, dst);
  link_list = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
}

/**
 * \brief Set the endpoints for a route from XML
 */
static void parse_S_route_new_and_endpoints_XML(void)
{
  parse_S_route_new_and_endpoints(A_surfxml_route_src,
                                  A_surfxml_route_dst);
}

/**
 * \brief Set the endpoints for a route from lua
 */
static void parse_S_route_new_and_endpoints_lua(const char *id_src, const char *id_dst)
{
  parse_S_route_new_and_endpoints(id_src, id_dst);
}

/**
 * \brief Set the endponints and gateways for a ASroute
 */
static void parse_S_ASroute_new_and_endpoints(void)
{
  if (src != NULL && dst != NULL && link_list != NULL)
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
  link_list = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
}

/**
 * \brief Set the endponints for a bypassRoute
 */
static void parse_S_bypassRoute_new_and_endpoints(void)
{
  if (src != NULL && dst != NULL && link_list != NULL)
    THROWF(arg_error, 0,
           "Bypass Route between %s to %s can not be defined",
           A_surfxml_bypassRoute_src, A_surfxml_bypassRoute_dst);
  src = A_surfxml_bypassRoute_src;
  dst = A_surfxml_bypassRoute_dst;
  gw_src = A_surfxml_bypassRoute_gw_src;
  gw_dst = A_surfxml_bypassRoute_gw_dst;
  xbt_assert(strlen(src) > 0 || strlen(dst) > 0 || strlen(gw_src) > 0
              || strlen(gw_dst) > 0,
              "Some limits are null in the route between \"%s\" and \"%s\"",
              src, dst);
  link_list = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
}

/**
 * \brief Set a new link on the actual list of link for a route or ASroute
 */
static void parse_E_link_ctn_new_elem(const char *link_id)
{
  char *val;
  val = xbt_strdup(link_id);
  xbt_dynar_push(link_list, &val);
}

/**
 * \brief Set a new link on the actual list of link for a route or ASroute from XML
 */

static void parse_E_link_ctn_new_elem_XML(void)
{
  if (A_surfxml_link_ctn_direction == A_surfxml_link_ctn_direction_NONE)
    parse_E_link_ctn_new_elem(A_surfxml_link_ctn_id);
  if (A_surfxml_link_ctn_direction == A_surfxml_link_ctn_direction_UP) {
    char *link_id = bprintf("%s_UP", A_surfxml_link_ctn_id);
    parse_E_link_ctn_new_elem(link_id);
    free(link_id);
  }
  if (A_surfxml_link_ctn_direction == A_surfxml_link_ctn_direction_DOWN) {
    char *link_id = bprintf("%s_DOWN", A_surfxml_link_ctn_id);
    parse_E_link_ctn_new_elem(link_id);
    free(link_id);
  }
}

/**
 * \brief Set a new link on the actual list of link for a route or ASroute from lua
 */
static void parse_E_link_c_ctn_new_elem_lua(const char *link_id)
{
  parse_E_link_ctn_new_elem(link_id);
}

/**
 * \brief Store the route by calling the set_route function of the current routing component
 */
static void parse_E_route_store_route(void)
{
  name_route_extended_t route = xbt_new0(s_name_route_extended_t, 1);
  route->generic_route.link_list = link_list;
  xbt_assert(current_routing->set_route,
              "no defined method \"set_route\" in \"%s\"",
              current_routing->name);
  (*(current_routing->set_route)) (current_routing, src, dst, route);
  link_list = NULL;
  src = NULL;
  dst = NULL;
}

/**
 * \brief Store the ASroute by calling the set_ASroute function of the current routing component
 */
static void parse_E_ASroute_store_route(void)
{
  name_route_extended_t e_route = xbt_new0(s_name_route_extended_t, 1);
  e_route->generic_route.link_list = link_list;
  e_route->src_gateway = xbt_strdup(gw_src);
  e_route->dst_gateway = xbt_strdup(gw_dst);
  xbt_assert(current_routing->set_ASroute,
              "no defined method \"set_ASroute\" in \"%s\"",
              current_routing->name);
  (*(current_routing->set_ASroute)) (current_routing, src, dst, e_route);
  link_list = NULL;
  src = NULL;
  dst = NULL;
  gw_src = NULL;
  gw_dst = NULL;
}

/**
 * \brief Store the bypass route by calling the set_bypassroute function of the current routing component
 */
static void parse_E_bypassRoute_store_route(void)
{
  route_extended_t e_route = xbt_new0(s_route_extended_t, 1);
  e_route->generic_route.link_list = link_list;
  e_route->src_gateway = xbt_strdup(gw_src);
  e_route->dst_gateway = xbt_strdup(gw_dst);
  xbt_assert(current_routing->set_bypassroute,
              "no defined method \"set_bypassroute\" in \"%s\"",
              current_routing->name);
  (*(current_routing->set_bypassroute)) (current_routing, src, dst,
                                         e_route);
  link_list = NULL;
  src = NULL;
  dst = NULL;
  gw_src = NULL;
  gw_dst = NULL;
}

/**
 * \brief Make a new routing component
 *
 * make the new structure and
 * set the parsing functions to allows parsing the part of the routing tree
 */
static void parse_S_AS(char *AS_id, char *AS_routing)
{
  routing_component_t new_routing;
  model_type_t model = NULL;
  char *wanted = AS_routing;
  int cpt;
  /* seach the routing model */
  for (cpt = 0; routing_models[cpt].name; cpt++)
    if (!strcmp(wanted, routing_models[cpt].name))
      model = &routing_models[cpt];
  /* if its not exist, error */
  if (model == NULL) {
    fprintf(stderr, "Routing model %s not found. Existing models:\n",
            wanted);
    for (cpt = 0; routing_models[cpt].name; cpt++)
      if (!strcmp(wanted, routing_models[cpt].name))
        fprintf(stderr, "   %s: %s\n", routing_models[cpt].name,
                routing_models[cpt].desc);
    xbt_die(NULL);
  }

  /* make a new routing component */
  new_routing = (routing_component_t) (*(model->create)) ();
  new_routing->routing = model;
  new_routing->hierarchy = SURF_ROUTING_NULL;
  new_routing->name = xbt_strdup(AS_id);
  new_routing->routing_sons = xbt_dict_new();

  /* Hack for Vivaldi */
  if(!strcmp(model->name,"Vivaldi"))
  	new_routing->get_latency = vivaldi_get_link_latency;

  if (current_routing == NULL && global_routing->root == NULL) {

    /* it is the first one */
    new_routing->routing_father = NULL;
    global_routing->root = new_routing;

  } else if (current_routing != NULL && global_routing->root != NULL) {

    xbt_assert(!xbt_dict_get_or_null
                (current_routing->routing_sons, AS_id),
                "The AS \"%s\" already exists", AS_id);
    /* it is a part of the tree */
    new_routing->routing_father = current_routing;
    /* set the father behavior */
    if (current_routing->hierarchy == SURF_ROUTING_NULL)
      current_routing->hierarchy = SURF_ROUTING_RECURSIVE;
    /* add to the sons dictionary */
    xbt_dict_set(current_routing->routing_sons, AS_id,
                 (void *) new_routing, NULL);
    /* add to the father element list */
    (*(current_routing->set_autonomous_system)) (current_routing, AS_id);
    /* unload the prev parse rules */
    (*(current_routing->routing->unload)) ();

  } else {
    THROWF(arg_error, 0, "All defined components must be belong to a AS");
  }
  /* set the new parse rules */
  (*(new_routing->routing->load)) ();
  /* set the new current component of the tree */
  current_routing = new_routing;
}

/*
 * Detect the routing model type of the routing component from XML platforms
 */
static void parse_S_AS_XML(void)
{
  parse_S_AS(A_surfxml_AS_id, A_surfxml_AS_routing);
}

/*
 * define the routing model type of routing component from lua script
 */
static void parse_S_AS_lua(char *id, char *mode)
{
  parse_S_AS(id, mode);
}


/**
 * \brief Finish the creation of a new routing component
 *
 * When you finish to read the routing component, other structures must be created. 
 * the "end" method allow to do that for any routing model type
 */
static void parse_E_AS(const char *AS_id)
{

  if (current_routing == NULL) {
    THROWF(arg_error, 0, "Close AS(%s), that never open", AS_id);
  } else {
    network_element_info_t info = NULL;
    xbt_assert(!xbt_lib_get_or_null(as_router_lib,current_routing->name, ROUTING_ASR_LEVEL),
    		"The AS \"%s\" already exists",current_routing->name);
    info = xbt_new0(s_network_element_info_t, 1);
    info->rc_component = current_routing->routing_father;
    info->rc_type = SURF_NETWORK_ELEMENT_AS;
    xbt_lib_set(as_router_lib,current_routing->name,ROUTING_ASR_LEVEL,(void *) info);

    (*(current_routing->routing->unload)) ();
    (*(current_routing->routing->end)) ();
    current_routing = current_routing->routing_father;
    if (current_routing != NULL)
      (*(current_routing->routing->load)) ();
  }
}

/*
 * \brief Finish the creation of a new routing component from XML
 */
static void parse_E_AS_XML(void)
{
  parse_E_AS(A_surfxml_AS_id);
}

/*
 * \brief Finish the creation of a new routing component from lua
 */
static void parse_E_AS_lua(const char *id)
{
  parse_E_AS(id);
}

/* Aux Business methods */

/**
 * \brief Get the AS name of the element
 *
 * \param name the host name
 *
 */
static char* elements_As_name(const char *name)
{
  routing_component_t as_comp;

  /* (1) find the as where the host is located */
  as_comp = ((network_element_info_t)
            xbt_lib_get_or_null(host_lib,name, ROUTING_HOST_LEVEL))->rc_component;
  return as_comp->name;
}


/**
 * \brief Get the AS father and the first elements of the chain
 *
 * \param src the source host name 
 * \param dst the destination host name
 * 
 * Get the common father of the to processing units, and the first different 
 * father in the chain
 */
static void elements_father(const char *src, const char *dst,
                            routing_component_t *res_father,
                            routing_component_t *res_src,
                            routing_component_t *res_dst)
{
  xbt_assert(src && dst, "bad parameters for \"elements_father\" method");
#define ELEMENTS_FATHER_MAXDEPTH 16 /* increase if it is not enough */
  routing_component_t src_as, dst_as;
  routing_component_t path_src[ELEMENTS_FATHER_MAXDEPTH];
  routing_component_t path_dst[ELEMENTS_FATHER_MAXDEPTH];
  int index_src = 0;
  int index_dst = 0;
  routing_component_t current;
  routing_component_t current_src;
  routing_component_t current_dst;
  routing_component_t father;

  /* (1) find the as where the src and dst are located */
  network_element_info_t src_data = xbt_lib_get_or_null(host_lib, src,
                                                        ROUTING_HOST_LEVEL);
  network_element_info_t dst_data = xbt_lib_get_or_null(host_lib, dst,
                                                        ROUTING_HOST_LEVEL);
  if (!src_data)
    src_data = xbt_lib_get_or_null(as_router_lib, src, ROUTING_ASR_LEVEL);
  if (!dst_data)
    dst_data = xbt_lib_get_or_null(as_router_lib, dst, ROUTING_ASR_LEVEL);
  src_as = src_data->rc_component;
  dst_as = dst_data->rc_component;

  xbt_assert(src_as && dst_as,
             "Ask for route \"from\"(%s) or \"to\"(%s) no found", src, dst);

  /* (2) find the path to the root routing component */
  for (current = src_as ; current != NULL ; current = current->routing_father) {
    path_src[index_src++] = current;
    xbt_assert(index_src <= ELEMENTS_FATHER_MAXDEPTH,
               "ELEMENTS_FATHER_MAXDEPTH should be increased for path_src");
  }
  for (current = dst_as ; current != NULL ; current = current->routing_father) {
    path_dst[index_dst++] = current;
    xbt_assert(index_dst <= ELEMENTS_FATHER_MAXDEPTH,
               "ELEMENTS_FATHER_MAXDEPTH should be increased for path_dst");
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
 * \param *e_route the route where the links are stored
 * \param *latency the latency, if needed
 * 
 * This function is called by "get_route" and "get_latency". It allows to walk
 * recursively through the routing components tree.
 */
static void _get_route_latency(const char *src, const char *dst,
                               xbt_dynar_t *route, double *latency)
{
  XBT_DEBUG("Solve route/latency  \"%s\" to \"%s\"", src, dst);
  xbt_assert(src && dst, "bad parameters for \"_get_route_latency\" method");

  routing_component_t common_father;
  routing_component_t src_father;
  routing_component_t dst_father;
  elements_father(src, dst, &common_father, &src_father, &dst_father);

  if (src_father == dst_father) { /* SURF_ROUTING_BASE */

    route_extended_t e_route = NULL;
    if (route) {
      e_route = common_father->get_route(common_father, src, dst);
      xbt_assert(e_route, "no route between \"%s\" and \"%s\"", src, dst);
      *route = e_route->generic_route.link_list;
    }
    if (latency) {
      *latency = common_father->get_latency(common_father, src, dst, e_route);
      xbt_assert(*latency >= 0.0,
                 "latency error on route between \"%s\" and \"%s\"", src, dst);
    }
    if (e_route) {
      xbt_free(e_route->src_gateway);
      xbt_free(e_route->dst_gateway);
      xbt_free(e_route);
    }

  } else {                      /* SURF_ROUTING_RECURSIVE */

    route_extended_t e_route_bypass = NULL;
    if (common_father->get_bypass_route)
      e_route_bypass = common_father->get_bypass_route(common_father, src, dst);

    xbt_assert(!latency || !e_route_bypass,
               "Bypass cannot work yet with get_latency");

    route_extended_t e_route_cnt = e_route_bypass
      ? e_route_bypass
      : common_father->get_route(common_father,
                               src_father->name, dst_father->name);

    xbt_assert(e_route_cnt, "no route between \"%s\" and \"%s\"",
               src_father->name, dst_father->name);

    xbt_assert((e_route_cnt->src_gateway == NULL) ==
               (e_route_cnt->dst_gateway == NULL),
               "bad gateway for route between \"%s\" and \"%s\"", src, dst);

    if (route) {
      *route = xbt_dynar_new(global_routing->size_of_link, NULL);
    }
    if (latency) {
      *latency = common_father->get_latency(common_father,
                                            src_father->name, dst_father->name,
                                            e_route_cnt);
      xbt_assert(*latency >= 0.0,
                 "latency error on route between \"%s\" and \"%s\"",
                 src_father->name, dst_father->name);
    }

    void *link;
    unsigned int cpt;

    if (strcmp(src, e_route_cnt->src_gateway)) {
      double latency_src;
      xbt_dynar_t route_src;

      _get_route_latency(src, e_route_cnt->src_gateway,
                         (route ? &route_src : NULL),
                         (latency ? &latency_src : NULL));
      if (route) {
        xbt_assert(route_src, "no route between \"%s\" and \"%s\"",
                   src, e_route_cnt->src_gateway);
        xbt_dynar_foreach(route_src, cpt, link) {
          xbt_dynar_push(*route, &link);
        }
        xbt_dynar_free(&route_src);
      }
      if (latency) {
        xbt_assert(latency_src >= 0.0,
                   "latency error on route between \"%s\" and \"%s\"",
                   src, e_route_cnt->src_gateway);
        *latency += latency_src;
      }
    }

    if (route) {
      xbt_dynar_foreach(e_route_cnt->generic_route.link_list, cpt, link) {
        xbt_dynar_push(*route, &link);
      }
    }

    if (strcmp(e_route_cnt->dst_gateway, dst)) {
      double latency_dst;
      xbt_dynar_t route_dst;

      _get_route_latency(e_route_cnt->dst_gateway, dst,
                         (route ? &route_dst : NULL),
                         (latency ? &latency_dst : NULL));
      if (route) {
        xbt_assert(route_dst, "no route between \"%s\" and \"%s\"",
                   e_route_cnt->dst_gateway, dst);
        xbt_dynar_foreach(route_dst, cpt, link) {
          xbt_dynar_push(*route, &link);
        }
        xbt_dynar_free(&route_dst);
      }
      if (latency) {
        xbt_assert(latency_dst >= 0.0,
                   "latency error on route between \"%s\" and \"%s\"",
                   e_route_cnt->dst_gateway, dst);
        *latency += latency_dst;
      }
    }

    generic_free_extended_route(e_route_cnt);
  }
}

/**
 * \brief Generic function for get_route, get_route_no_cleanup, and get_latency
 */
static void get_route_latency(const char *src, const char *dst,
                              xbt_dynar_t *route, double *latency, int cleanup)
{
  _get_route_latency(src, dst, route, latency);
  xbt_assert(!route || *route, "no route between \"%s\" and \"%s\"", src, dst);
  xbt_assert(!latency || *latency >= 0.0,
             "latency error on route between \"%s\" and \"%s\"", src, dst);
  if (route) {
    xbt_dynar_free(&global_routing->last_route);
    global_routing->last_route = cleanup ? *route : NULL;
  }
}

/**
 * \brief Generic method: find a route between hosts
 *
 * \param src the source host name 
 * \param dst the destination host name
 * 
 * walk through the routing components tree and find a route between hosts
 * by calling the differents "get_route" functions in each routing component.
 * No need to free the returned dynar. It will be freed at the next call.
 */
static xbt_dynar_t get_route(const char *src, const char *dst)
{
  xbt_dynar_t route = NULL;
  get_route_latency(src, dst, &route, NULL, 1);
  return route;
}

/**
 * \brief Generic method: find a route between hosts
 *
 * \param src the source host name
 * \param dst the destination host name
 *
 * walk through the routing components tree and find a route between hosts
 * by calling the differents "get_route" functions in each routing component.
 * Leaves the caller the responsability to clean the returned dynar.
 */
static xbt_dynar_t get_route_no_cleanup(const char *src, const char *dst)
{
  xbt_dynar_t route = NULL;
  get_route_latency(src, dst, &route, NULL, 0);
  return route;
}

/*Get Latency*/
static double get_latency(const char *src, const char *dst)
{
  double latency = -1.0;
  get_route_latency(src, dst, NULL, &latency, 0);
  return latency;
}

/**
 * \brief Recursive function for finalize
 *
 * \param rc the source host name 
 * 
 * This fuction is call by "finalize". It allow to finalize the 
 * AS or routing components. It delete all the structures.
 */
static void _finalize(routing_component_t rc)
{
  if (rc) {
    xbt_dict_cursor_t cursor = NULL;
    char *key;
    routing_component_t elem;
    xbt_dict_foreach(rc->routing_sons, cursor, key, elem) {
      _finalize(elem);
    }
    xbt_dict_t tmp_sons = rc->routing_sons;
    char *tmp_name = rc->name;
    xbt_dict_free(&tmp_sons);
    xbt_free(tmp_name);
    xbt_assert(rc->finalize, "no defined method \"finalize\" in \"%s\"",
                current_routing->name);
    (*(rc->finalize)) (rc);
  }
}

/**
 * \brief Generic method: delete all the routing structures
 * 
 * walk through the routing components tree and delete the structures
 * by calling the differents "finalize" functions in each routing component
 */
static void finalize(void)
{
  /* delete recursibly all the tree */
  _finalize(global_routing->root);
  /* delete last_route */
  xbt_dynar_free(&(global_routing->last_route));
  /* delete global routing structure */
  xbt_free(global_routing);
}

static xbt_dynar_t recursive_get_onelink_routes(routing_component_t rc)
{
  xbt_dynar_t ret = xbt_dynar_new(sizeof(onelink_t), xbt_free);

  //adding my one link routes
  unsigned int cpt;
  void *link;
  xbt_dynar_t onelink_mine = rc->get_onelink_routes(rc);
  if (onelink_mine) {
    xbt_dynar_foreach(onelink_mine, cpt, link) {
      xbt_dynar_push(ret, &link);
    }
  }
  //recursing
  char *key;
  xbt_dict_cursor_t cursor = NULL;
  routing_component_t rc_child;
  xbt_dict_foreach(rc->routing_sons, cursor, key, rc_child) {
    xbt_dynar_t onelink_child = recursive_get_onelink_routes(rc_child);
    if (onelink_child) {
      xbt_dynar_foreach(onelink_child, cpt, link) {
        xbt_dynar_push(ret, &link);
      }
    }
  }
  return ret;
}

static xbt_dynar_t get_onelink_routes(void)
{
  return recursive_get_onelink_routes(global_routing->root);
}

e_surf_network_element_type_t get_network_element_type(const char
                                                              *name)
{
  network_element_info_t rc = NULL;

  rc = xbt_lib_get_or_null(host_lib, name, ROUTING_HOST_LEVEL);
  if(rc) return rc->rc_type;

  rc = xbt_lib_get_or_null(as_router_lib, name, ROUTING_ASR_LEVEL);
  if(rc) return rc->rc_type;

  return SURF_NETWORK_ELEMENT_NULL;
}

/**
 * \brief Generic method: create the global routing schema
 * 
 * Make a global routing structure and set all the parsing functions.
 */
void routing_model_create(size_t size_of_links, void *loopback, double_f_cpvoid_t get_link_latency_fun)
{
  /* config the uniq global routing */
  global_routing = xbt_new0(s_routing_global_t, 1);
  global_routing->root = NULL;
  global_routing->get_route = get_route;
  global_routing->get_latency = get_latency;
  global_routing->get_route_no_cleanup = get_route_no_cleanup;
  global_routing->get_onelink_routes = get_onelink_routes;
  global_routing->get_network_element_type = get_network_element_type;
  global_routing->finalize = finalize;
  global_routing->loopback = loopback;
  global_routing->size_of_link = size_of_links;
  global_routing->last_route = NULL;
  get_link_latency = get_link_latency_fun;
  /* no current routing at moment */
  current_routing = NULL;

  /* parse generic elements */
  surfxml_add_callback(STag_surfxml_host_cb_list, &parse_S_host_XML);
  surfxml_add_callback(ETag_surfxml_host_cb_list, &parse_E_host_XML);
  surfxml_add_callback(STag_surfxml_router_cb_list, &parse_S_router_XML);

  surfxml_add_callback(STag_surfxml_route_cb_list,
                       &parse_S_route_new_and_endpoints_XML);
  surfxml_add_callback(STag_surfxml_ASroute_cb_list,
                       &parse_S_ASroute_new_and_endpoints);
  surfxml_add_callback(STag_surfxml_bypassRoute_cb_list,
                       &parse_S_bypassRoute_new_and_endpoints);

  surfxml_add_callback(ETag_surfxml_link_ctn_cb_list,
                       &parse_E_link_ctn_new_elem_XML);

  surfxml_add_callback(ETag_surfxml_route_cb_list,
                       &parse_E_route_store_route);
  surfxml_add_callback(ETag_surfxml_ASroute_cb_list,
                       &parse_E_ASroute_store_route);
  surfxml_add_callback(ETag_surfxml_bypassRoute_cb_list,
                       &parse_E_bypassRoute_store_route);

  surfxml_add_callback(STag_surfxml_AS_cb_list, &parse_S_AS_XML);
  surfxml_add_callback(ETag_surfxml_AS_cb_list, &parse_E_AS_XML);

  surfxml_add_callback(STag_surfxml_cluster_cb_list,
                       &routing_parse_Scluster);

  surfxml_add_callback(STag_surfxml_peer_cb_list,
                         &routing_parse_Speer);

  surfxml_add_callback(ETag_surfxml_platform_cb_list,
						  &clean_dict_random);

#ifdef HAVE_TRACING
  instr_routing_define_callbacks();
#endif
}

void surf_parse_add_callback_config(void)
{
	surfxml_add_callback(STag_surfxml_config_cb_list, &routing_parse_Sconfig);
	surfxml_add_callback(ETag_surfxml_config_cb_list, &routing_parse_Econfig);
	surfxml_add_callback(STag_surfxml_prop_cb_list, &parse_properties_XML);
	surfxml_add_callback(STag_surfxml_AS_cb_list, &surf_parse_models_setup);
	surfxml_add_callback(STag_surfxml_random_cb_list, &routing_parse_Srandom);
}

void surf_parse_models_setup()
{
	routing_parse_Erandom();
	surfxml_del_callback(STag_surfxml_AS_cb_list, surf_parse_models_setup);
	surf_config_models_setup(platform_filename);
	free(platform_filename);
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

void generic_set_processing_unit(routing_component_t rc,
                                        const char *name)
{
  XBT_DEBUG("Load process unit \"%s\"", name);
  int *id = xbt_new0(int, 1);
  xbt_dict_t _to_index;
  _to_index = current_routing->to_index;
  *id = xbt_dict_length(_to_index);
  xbt_dict_set(_to_index, name, id, xbt_free);
}

void generic_set_autonomous_system(routing_component_t rc,
                                          const char *name)
{
  XBT_DEBUG("Load Autonomous system \"%s\"", name);
  int *id = xbt_new0(int, 1);
  xbt_dict_t _to_index;
  _to_index = current_routing->to_index;
  *id = xbt_dict_length(_to_index);
  xbt_dict_set(_to_index, name, id, xbt_free);
}

int surf_pointer_resource_cmp(const void *a, const void *b)
{
  return a != b;
}

int surf_link_resource_cmp(const void *a, const void *b)
{
  return !!memcmp(a,b,global_routing->size_of_link);
}

void generic_set_bypassroute(routing_component_t rc,
                                    const char *src, const char *dst,
                                    route_extended_t e_route)
{
  XBT_DEBUG("Load bypassRoute from \"%s\" to \"%s\"", src, dst);
  xbt_dict_t dict_bypassRoutes = rc->bypassRoutes;
  char *route_name;

  route_name = bprintf("%s#%s", src, dst);
  xbt_assert(xbt_dynar_length(e_route->generic_route.link_list) > 0,
              "Invalid count of links, must be greater than zero (%s,%s)",
              src, dst);
  xbt_assert(!xbt_dict_get_or_null(dict_bypassRoutes, route_name),
              "The bypass route between \"%s\"(\"%s\") and \"%s\"(\"%s\") already exists",
              src, e_route->src_gateway, dst, e_route->dst_gateway);

  route_extended_t new_e_route =
      generic_new_extended_route(SURF_ROUTING_RECURSIVE, e_route, 0);
  xbt_dynar_free(&(e_route->generic_route.link_list));
  xbt_free(e_route);

  xbt_dict_set(dict_bypassRoutes, route_name, new_e_route,
               (void (*)(void *)) generic_free_extended_route);
  xbt_free(route_name);
}

/* ************************************************************************** */
/* *********************** GENERIC BUSINESS METHODS ************************* */

double generic_get_link_latency(routing_component_t rc,
                                       const char *src, const char *dst,
                                       route_extended_t route)
{
	int need_to_clean = route?0:1;
	void * link;
	unsigned int i;
	double latency = 0.0;

	route = route?route:rc->get_route(rc,src,dst);

	xbt_dynar_foreach(route->generic_route.link_list,i,link) {
		latency += get_link_latency(link);
	}
	if(need_to_clean) generic_free_extended_route(route);
  return latency;
}

xbt_dynar_t generic_get_onelink_routes(routing_component_t rc)
{
  xbt_die("\"generic_get_onelink_routes\" not implemented yet");
}

route_extended_t generic_get_bypassroute(routing_component_t rc,
                                                const char *src,
                                                const char *dst)
{
  xbt_dict_t dict_bypassRoutes = rc->bypassRoutes;
  routing_component_t src_as, dst_as;
  int index_src, index_dst;
  xbt_dynar_t path_src = NULL;
  xbt_dynar_t path_dst = NULL;
  routing_component_t current = NULL;
  routing_component_t *current_src = NULL;
  routing_component_t *current_dst = NULL;

  /* (1) find the as where the src and dst are located */
  void * src_data = xbt_lib_get_or_null(host_lib,src, ROUTING_HOST_LEVEL);
  void * dst_data = xbt_lib_get_or_null(host_lib,dst, ROUTING_HOST_LEVEL);
  if(!src_data) src_data = xbt_lib_get_or_null(as_router_lib,src, ROUTING_ASR_LEVEL);
  if(!dst_data) dst_data = xbt_lib_get_or_null(as_router_lib,dst, ROUTING_ASR_LEVEL);

  if(src_data == NULL || dst_data == NULL)
	  xbt_die("Ask for route \"from\"(%s) or \"to\"(%s) no found at AS \"%s\"",
	             src, dst, rc->name);

  src_as = ((network_element_info_t)src_data)->rc_component;
  dst_as = ((network_element_info_t)dst_data)->rc_component;

  /* (2) find the path to the root routing component */
  path_src = xbt_dynar_new(sizeof(routing_component_t), NULL);
  current = src_as;
  while (current != NULL) {
    xbt_dynar_push(path_src, &current);
    current = current->routing_father;
  }
  path_dst = xbt_dynar_new(sizeof(routing_component_t), NULL);
  current = dst_as;
  while (current != NULL) {
    xbt_dynar_push(path_dst, &current);
    current = current->routing_father;
  }

  /* (3) find the common father */
  index_src = path_src->used - 1;
  index_dst = path_dst->used - 1;
  current_src = xbt_dynar_get_ptr(path_src, index_src);
  current_dst = xbt_dynar_get_ptr(path_dst, index_dst);
  while (index_src >= 0 && index_dst >= 0 && *current_src == *current_dst) {
    routing_component_t *tmp_src, *tmp_dst;
    tmp_src = xbt_dynar_pop_ptr(path_src);
    tmp_dst = xbt_dynar_pop_ptr(path_dst);
    index_src--;
    index_dst--;
    current_src = xbt_dynar_get_ptr(path_src, index_src);
    current_dst = xbt_dynar_get_ptr(path_dst, index_dst);
  }

  int max_index_src = path_src->used - 1;
  int max_index_dst = path_dst->used - 1;

  int max_index = max(max_index_src, max_index_dst);
  int i, max;

  route_extended_t e_route_bypass = NULL;

  for (max = 0; max <= max_index; max++) {
    for (i = 0; i < max; i++) {
      if (i <= max_index_src && max <= max_index_dst) {
        char *route_name = bprintf("%s#%s",
                                   (*(routing_component_t *)
                                    (xbt_dynar_get_ptr
                                     (path_src, i)))->name,
                                   (*(routing_component_t *)
                                    (xbt_dynar_get_ptr
                                     (path_dst, max)))->name);
        e_route_bypass =
            xbt_dict_get_or_null(dict_bypassRoutes, route_name);
        xbt_free(route_name);
      }
      if (e_route_bypass)
        break;
      if (max <= max_index_src && i <= max_index_dst) {
        char *route_name = bprintf("%s#%s",
                                   (*(routing_component_t *)
                                    (xbt_dynar_get_ptr
                                     (path_src, max)))->name,
                                   (*(routing_component_t *)
                                    (xbt_dynar_get_ptr
                                     (path_dst, i)))->name);
        e_route_bypass =
            xbt_dict_get_or_null(dict_bypassRoutes, route_name);
        xbt_free(route_name);
      }
      if (e_route_bypass)
        break;
    }

    if (e_route_bypass)
      break;

    if (max <= max_index_src && max <= max_index_dst) {
      char *route_name = bprintf("%s#%s",
                                 (*(routing_component_t *)
                                  (xbt_dynar_get_ptr
                                   (path_src, max)))->name,
                                 (*(routing_component_t *)
                                  (xbt_dynar_get_ptr
                                   (path_dst, max)))->name);
      e_route_bypass = xbt_dict_get_or_null(dict_bypassRoutes, route_name);
      xbt_free(route_name);
    }
    if (e_route_bypass)
      break;
  }

  xbt_dynar_free(&path_src);
  xbt_dynar_free(&path_dst);

  route_extended_t new_e_route = NULL;

  if (e_route_bypass) {
    void *link;
    unsigned int cpt = 0;
    new_e_route = xbt_new0(s_route_extended_t, 1);
    new_e_route->src_gateway = xbt_strdup(e_route_bypass->src_gateway);
    new_e_route->dst_gateway = xbt_strdup(e_route_bypass->dst_gateway);
    new_e_route->generic_route.link_list =
        xbt_dynar_new(global_routing->size_of_link, NULL);
    xbt_dynar_foreach(e_route_bypass->generic_route.link_list, cpt, link) {
      xbt_dynar_push(new_e_route->generic_route.link_list, &link);
    }
  }

  return new_e_route;
}

/* ************************************************************************** */
/* ************************* GENERIC AUX FUNCTIONS ************************** */

route_t
generic_new_route(e_surf_routing_hierarchy_t hierarchy,
                           void *data, int order)
{

  char *link_name;
  route_t new_route;
  unsigned int cpt;
  xbt_dynar_t links = NULL, links_id = NULL;

  new_route = xbt_new0(s_route_t, 1);
  new_route->link_list =
      xbt_dynar_new(global_routing->size_of_link, NULL);

  xbt_assert(hierarchy == SURF_ROUTING_BASE,
              "the hierarchy type is not SURF_ROUTING_BASE");

  links = ((route_t) data)->link_list;


  links_id = new_route->link_list;

  xbt_dynar_foreach(links, cpt, link_name) {

    void *link =
    		xbt_lib_get_or_null(link_lib, link_name, SURF_LINK_LEVEL);
    if (link) {
      if (order)
        xbt_dynar_push(links_id, &link);
      else
        xbt_dynar_unshift(links_id, &link);
    } else
      THROWF(mismatch_error, 0, "Link %s not found", link_name);
  }

  return new_route;
}

route_extended_t
generic_new_extended_route(e_surf_routing_hierarchy_t hierarchy,
                           void *data, int order)
{

  char *link_name;
  route_extended_t e_route, new_e_route;
  route_t route;
  unsigned int cpt;
  xbt_dynar_t links = NULL, links_id = NULL;

  new_e_route = xbt_new0(s_route_extended_t, 1);
  new_e_route->generic_route.link_list =
      xbt_dynar_new(global_routing->size_of_link, NULL);
  new_e_route->src_gateway = NULL;
  new_e_route->dst_gateway = NULL;

  xbt_assert(hierarchy == SURF_ROUTING_BASE
              || hierarchy == SURF_ROUTING_RECURSIVE,
              "the hierarchy type is not defined");

  if (hierarchy == SURF_ROUTING_BASE) {

    route = (route_t) data;
    links = route->link_list;

  } else if (hierarchy == SURF_ROUTING_RECURSIVE) {

    e_route = (route_extended_t) data;
    xbt_assert(e_route->src_gateway
                && e_route->dst_gateway, "bad gateway, is null");
    links = e_route->generic_route.link_list;

    /* remeber not erase the gateway names */
    new_e_route->src_gateway = strdup(e_route->src_gateway);
    new_e_route->dst_gateway = strdup(e_route->dst_gateway);
  }

  links_id = new_e_route->generic_route.link_list;

  xbt_dynar_foreach(links, cpt, link_name) {

    void *link =
    		xbt_lib_get_or_null(link_lib, link_name, SURF_LINK_LEVEL);
    if (link) {
      if (order)
        xbt_dynar_push(links_id, &link);
      else
        xbt_dynar_unshift(links_id, &link);
    } else
      THROWF(mismatch_error, 0, "Link %s not found", link_name);
  }

  return new_e_route;
}

void generic_free_route(route_t route)
{
  if (route) {
    xbt_dynar_free(&(route->link_list));
    xbt_free(route);
  }
}

void generic_free_extended_route(route_extended_t e_route)
{
  if (e_route) {
    xbt_dynar_free(&(e_route->generic_route.link_list));
    if (e_route->src_gateway)
      xbt_free(e_route->src_gateway);
    if (e_route->dst_gateway)
      xbt_free(e_route->dst_gateway);
    xbt_free(e_route);
  }
}

static routing_component_t generic_as_exist(routing_component_t find_from,
                                            routing_component_t to_find)
{
  //return to_find; // FIXME: BYPASSERROR OF FOREACH WITH BREAK
  xbt_dict_cursor_t cursor = NULL;
  char *key;
  int found = 0;
  routing_component_t elem;
  xbt_dict_foreach(find_from->routing_sons, cursor, key, elem) {
    if (to_find == elem || generic_as_exist(elem, to_find)) {
      found = 1;
      break;
    }
  }
  if (found)
    return to_find;
  return NULL;
}

routing_component_t
generic_autonomous_system_exist(routing_component_t rc, char *element)
{
  //return rc; // FIXME: BYPASSERROR OF FOREACH WITH BREAK
  routing_component_t element_as, result, elem;
  xbt_dict_cursor_t cursor = NULL;
  char *key;
  element_as = ((network_element_info_t)
                xbt_lib_get_or_null(as_router_lib, element, ROUTING_ASR_LEVEL))->rc_component;
  result = ((routing_component_t) - 1);
  if (element_as != rc)
    result = generic_as_exist(rc, element_as);

  int found = 0;
  if (result) {
    xbt_dict_foreach(element_as->routing_sons, cursor, key, elem) {
      found = !strcmp(elem->name, element);
      if (found)
        break;
    }
    if (found)
      return element_as;
  }
  return NULL;
}

routing_component_t
generic_processing_units_exist(routing_component_t rc, char *element)
{
  routing_component_t element_as;
  element_as = ((network_element_info_t)
                xbt_lib_get_or_null(host_lib,
                 element, ROUTING_HOST_LEVEL))->rc_component;
  if (element_as == rc)
    return element_as;
  return generic_as_exist(rc, element_as);
}

void generic_src_dst_check(routing_component_t rc, const char *src,
                                  const char *dst)
{

  void * src_data = xbt_lib_get_or_null(host_lib,src, ROUTING_HOST_LEVEL);
  void * dst_data = xbt_lib_get_or_null(host_lib,dst, ROUTING_HOST_LEVEL);
  if(!src_data) src_data = xbt_lib_get_or_null(as_router_lib,src, ROUTING_ASR_LEVEL);
  if(!dst_data) dst_data = xbt_lib_get_or_null(as_router_lib,dst, ROUTING_ASR_LEVEL);

  if(src_data == NULL || dst_data == NULL)
	  xbt_die("Ask for route \"from\"(%s) or \"to\"(%s) no found at AS \"%s\"",
	             src, dst, rc->name);

  routing_component_t src_as = ((network_element_info_t)src_data)->rc_component;
  routing_component_t dst_as = ((network_element_info_t)dst_data)->rc_component;

  if(src_as != dst_as)
	  xbt_die("The src(%s in %s) and dst(%s in %s) are in differents AS",
              src, src_as->name, dst, dst_as->name);
  if(rc != dst_as)
	 xbt_die("The routing component of src'%s' and dst'%s' is not the same as the network elements belong (%s?=%s?=%s)",
     src,dst,src_as->name, dst_as->name,rc->name);
}

static void routing_parse_Sconfig(void)
{
  XBT_DEBUG("START configuration name = %s",A_surfxml_config_id);
}

static void routing_parse_Econfig(void)
{
  xbt_dict_cursor_t cursor = NULL;
  char *key;
  char *elem;
  char *cfg;
  xbt_dict_foreach(current_property_set, cursor, key, elem) {
	  cfg = bprintf("%s:%s",key,elem);
	  if(xbt_cfg_is_default_value(_surf_cfg_set, key))
		  xbt_cfg_set_parse(_surf_cfg_set, cfg);
	  else
		  XBT_INFO("The custom configuration '%s' is already define by user!",key);
	  free(cfg);
  }
  XBT_DEBUG("End configuration name = %s",A_surfxml_config_id);
}

void routing_parse_Scluster(void)
{
  static int AX_ptr = 0;

  char *cluster_id = A_surfxml_cluster_id;
  char *cluster_prefix = A_surfxml_cluster_prefix;
  char *cluster_suffix = A_surfxml_cluster_suffix;
  char *cluster_radical = A_surfxml_cluster_radical;
  char *cluster_power = A_surfxml_cluster_power;
  char *cluster_core = A_surfxml_cluster_core;
  char *cluster_bw = A_surfxml_cluster_bw;
  char *cluster_lat = A_surfxml_cluster_lat;
  char *temp_cluster_bw = NULL;
  char *temp_cluster_lat = NULL;
  char *temp_cluster_power = NULL;
  char *cluster_bb_bw = A_surfxml_cluster_bb_bw;
  char *cluster_bb_lat = A_surfxml_cluster_bb_lat;
  char *cluster_availability_file = A_surfxml_cluster_availability_file;
  char *cluster_state_file = A_surfxml_cluster_state_file;
  char *host_id, *groups, *link_id = NULL;
  char *router_id, *link_backbone;
  char *availability_file = xbt_strdup(cluster_availability_file);
  char *state_file = xbt_strdup(cluster_state_file);

  if(xbt_dict_size(patterns)==0)
	  patterns = xbt_dict_new();

  xbt_dict_set(patterns,"id",cluster_id,NULL);
  xbt_dict_set(patterns,"prefix",cluster_prefix,NULL);
  xbt_dict_set(patterns,"suffix",cluster_suffix,NULL);

#ifdef HAVE_PCRE_LIB
  char *route_src_dst;
#endif
  unsigned int iter;
  int start, end, i;
  xbt_dynar_t radical_elements;
  xbt_dynar_t radical_ends;
  int cluster_sharing_policy = AX_surfxml_cluster_sharing_policy;
  int cluster_bb_sharing_policy = AX_surfxml_cluster_bb_sharing_policy;

#ifndef HAVE_PCRE_LIB
  xbt_dynar_t tab_elements_num = xbt_dynar_new(sizeof(int), NULL);
  char *route_src, *route_dst;
  int j;
#endif

  static unsigned int surfxml_buffer_stack_stack_ptr = 1;
  static unsigned int surfxml_buffer_stack_stack[1024];

  surfxml_buffer_stack_stack[0] = 0;

  surfxml_bufferstack_push(1);

  SURFXML_BUFFER_SET(AS_id, cluster_id);
#ifdef HAVE_PCRE_LIB
  SURFXML_BUFFER_SET(AS_routing, "RuleBased");
  XBT_DEBUG("<AS id=\"%s\"\trouting=\"RuleBased\">", cluster_id);
#else
  SURFXML_BUFFER_SET(AS_routing, "Full");
  XBT_DEBUG("<AS id=\"%s\"\trouting=\"Full\">", cluster_id);
#endif
  SURFXML_START_TAG(AS);

  radical_elements = xbt_str_split(cluster_radical, ",");
  xbt_dynar_foreach(radical_elements, iter, groups) {
    radical_ends = xbt_str_split(groups, "-");
    switch (xbt_dynar_length(radical_ends)) {
    case 1:
      surf_parse_get_int(&start,
                         xbt_dynar_get_as(radical_ends, 0, char *));
      host_id = bprintf("%s%d%s", cluster_prefix, start, cluster_suffix);
#ifndef HAVE_PCRE_LIB
      xbt_dynar_push_as(tab_elements_num, int, start);
#endif
      link_id = bprintf("%s_link_%d", cluster_id, start);

      xbt_dict_set(patterns, "radical", bprintf("%d", start), xbt_free);
      temp_cluster_power = xbt_strdup(cluster_power);
      temp_cluster_power = replace_random_parameter(temp_cluster_power);
      XBT_DEBUG("<host\tid=\"%s\"\tpower=\"%s\">", host_id, temp_cluster_power);
      A_surfxml_host_state = A_surfxml_host_state_ON;
      SURFXML_BUFFER_SET(host_id, host_id);
      SURFXML_BUFFER_SET(host_power, temp_cluster_power);
      SURFXML_BUFFER_SET(host_core, cluster_core);
      SURFXML_BUFFER_SET(host_availability, "1.0");
      SURFXML_BUFFER_SET(host_coordinates, "");
      xbt_free(availability_file);
      availability_file = xbt_strdup(cluster_availability_file);
      xbt_free(state_file);
      state_file = xbt_strdup(cluster_state_file);
      XBT_DEBUG("\tavailability_file=\"%s\"",xbt_str_varsubst(availability_file,patterns));
      XBT_DEBUG("\tstate_file=\"%s\"",xbt_str_varsubst(state_file,patterns));
      SURFXML_BUFFER_SET(host_availability_file, xbt_str_varsubst(availability_file,patterns));
      SURFXML_BUFFER_SET(host_state_file, xbt_str_varsubst(state_file,patterns));
      XBT_DEBUG("</host>");
      SURFXML_START_TAG(host);
      SURFXML_END_TAG(host);


      temp_cluster_bw = xbt_strdup(cluster_bw);
      temp_cluster_bw = replace_random_parameter(temp_cluster_bw);
      temp_cluster_lat = xbt_strdup(cluster_lat);
      temp_cluster_lat = replace_random_parameter(temp_cluster_lat);
      XBT_DEBUG("<link\tid=\"%s\"\tbw=\"%s\"\tlat=\"%s\"/>", link_id,temp_cluster_bw, cluster_lat);
      A_surfxml_link_state = A_surfxml_link_state_ON;
      A_surfxml_link_sharing_policy = A_surfxml_link_sharing_policy_SHARED;
      if(cluster_sharing_policy == A_surfxml_cluster_sharing_policy_FULLDUPLEX)
	  {A_surfxml_link_sharing_policy =  A_surfxml_link_sharing_policy_FULLDUPLEX;}
      if(cluster_sharing_policy == A_surfxml_cluster_sharing_policy_FATPIPE)
	  {A_surfxml_link_sharing_policy =  A_surfxml_link_sharing_policy_FATPIPE;}
      SURFXML_BUFFER_SET(link_id, link_id);
      SURFXML_BUFFER_SET(link_bandwidth, temp_cluster_bw);
      SURFXML_BUFFER_SET(link_latency, temp_cluster_lat);
      SURFXML_BUFFER_SET(link_bandwidth_file, "");
      SURFXML_BUFFER_SET(link_latency_file, "");
      SURFXML_BUFFER_SET(link_state_file, "");
      SURFXML_START_TAG(link);
      SURFXML_END_TAG(link);

      xbt_free(temp_cluster_bw);
      xbt_free(temp_cluster_lat);
      xbt_free(temp_cluster_power);
      free(link_id);
      free(host_id);
      break;

    case 2:

      surf_parse_get_int(&start,
                         xbt_dynar_get_as(radical_ends, 0, char *));
      surf_parse_get_int(&end, xbt_dynar_get_as(radical_ends, 1, char *));
      for (i = start; i <= end; i++) {
        host_id = bprintf("%s%d%s", cluster_prefix, i, cluster_suffix);
#ifndef HAVE_PCRE_LIB
        xbt_dynar_push_as(tab_elements_num, int, i);
#endif
        link_id = bprintf("%s_link_%d", cluster_id, i);

        xbt_dict_set(patterns, "radical", bprintf("%d", i), xbt_free);
        temp_cluster_power = xbt_strdup(cluster_power);
        temp_cluster_power = replace_random_parameter(temp_cluster_power);
        XBT_DEBUG("<host\tid=\"%s\"\tpower=\"%s\">", host_id, temp_cluster_power);
        A_surfxml_host_state = A_surfxml_host_state_ON;
        SURFXML_BUFFER_SET(host_id, host_id);
        SURFXML_BUFFER_SET(host_power, temp_cluster_power);
        SURFXML_BUFFER_SET(host_core, cluster_core);
        SURFXML_BUFFER_SET(host_availability, "1.0");
        SURFXML_BUFFER_SET(host_coordinates, "");
        xbt_free(availability_file);
        availability_file = xbt_strdup(cluster_availability_file);
        xbt_free(state_file);
        state_file = xbt_strdup(cluster_state_file);
        XBT_DEBUG("\tavailability_file=\"%s\"",xbt_str_varsubst(availability_file,patterns));
        XBT_DEBUG("\tstate_file=\"%s\"",xbt_str_varsubst(state_file,patterns));
        SURFXML_BUFFER_SET(host_availability_file, xbt_str_varsubst(availability_file,patterns));
        SURFXML_BUFFER_SET(host_state_file, xbt_str_varsubst(state_file,patterns));
        XBT_DEBUG("</host>");
        SURFXML_START_TAG(host);
        SURFXML_END_TAG(host);

        xbt_free(temp_cluster_power);

        temp_cluster_bw = xbt_strdup(cluster_bw);
        temp_cluster_bw = replace_random_parameter(temp_cluster_bw);
        temp_cluster_lat = xbt_strdup(cluster_lat);
        temp_cluster_lat = replace_random_parameter(temp_cluster_lat);
        XBT_DEBUG("<link\tid=\"%s\"\tbw=\"%s\"\tlat=\"%s\"/>", link_id,temp_cluster_bw, cluster_lat);
        A_surfxml_link_state = A_surfxml_link_state_ON;
        A_surfxml_link_sharing_policy = A_surfxml_link_sharing_policy_SHARED;
        if(cluster_sharing_policy == A_surfxml_cluster_sharing_policy_FULLDUPLEX)
  	    {A_surfxml_link_sharing_policy =  A_surfxml_link_sharing_policy_FULLDUPLEX;}
        if(cluster_sharing_policy == A_surfxml_cluster_sharing_policy_FATPIPE)
  	    {A_surfxml_link_sharing_policy =  A_surfxml_link_sharing_policy_FATPIPE;}
        SURFXML_BUFFER_SET(link_id, link_id);
        SURFXML_BUFFER_SET(link_bandwidth, temp_cluster_bw);
        SURFXML_BUFFER_SET(link_latency, temp_cluster_lat);
        SURFXML_BUFFER_SET(link_bandwidth_file, "");
        SURFXML_BUFFER_SET(link_latency_file, "");
        SURFXML_BUFFER_SET(link_state_file, "");
        SURFXML_START_TAG(link);
        SURFXML_END_TAG(link);

        xbt_free(temp_cluster_bw);
        xbt_free(temp_cluster_lat);
        free(link_id);
        free(host_id);
      }
      break;

    default:
      XBT_DEBUG("Malformed radical");
    }

    xbt_dynar_free(&radical_ends);
  }
  xbt_dynar_free(&radical_elements);

  XBT_DEBUG(" ");
  router_id =
      bprintf("%s%s_router%s", cluster_prefix, cluster_id,
              cluster_suffix);
  //link_router = bprintf("%s_link_%s_router", cluster_id, cluster_id);
  link_backbone = bprintf("%s_backbone", cluster_id);

  XBT_DEBUG("<router id=\"%s\"/>", router_id);
  SURFXML_BUFFER_SET(router_id, router_id);
  SURFXML_BUFFER_SET(router_coordinates, "");
  SURFXML_START_TAG(router);
  SURFXML_END_TAG(router);

  //TODO
//  xbt_dict_set(patterns, "radical", xbt_strdup("_router"), xbt_free);
//  temp_cluster_bw = xbt_strdup(cluster_bw);
//  temp_cluster_bw = replace_random_parameter(temp_cluster_bw);
//  temp_cluster_lat = xbt_strdup(cluster_lat);
//  temp_cluster_lat = replace_random_parameter(temp_cluster_lat);
//  XBT_DEBUG("<link\tid=\"%s\" bw=\"%s\" lat=\"%s\"/>", link_router,temp_cluster_bw, temp_cluster_lat);
//  A_surfxml_link_state = A_surfxml_link_state_ON;
//  A_surfxml_link_sharing_policy = A_surfxml_link_sharing_policy_SHARED;
//  if(cluster_sharing_policy == A_surfxml_cluster_sharing_policy_FULLDUPLEX)
//  {A_surfxml_link_sharing_policy =  A_surfxml_link_sharing_policy_FULLDUPLEX;}
//  if(cluster_sharing_policy == A_surfxml_cluster_sharing_policy_FATPIPE)
//  {A_surfxml_link_sharing_policy =  A_surfxml_link_sharing_policy_FATPIPE;}
//  SURFXML_BUFFER_SET(link_id, link_router);
//  SURFXML_BUFFER_SET(link_bandwidth, temp_cluster_bw);
//  SURFXML_BUFFER_SET(link_latency, temp_cluster_lat);
//  SURFXML_BUFFER_SET(link_bandwidth_file, "");
//  SURFXML_BUFFER_SET(link_latency_file, "");
//  SURFXML_BUFFER_SET(link_state_file, "");
//  SURFXML_START_TAG(link);
//  SURFXML_END_TAG(link);

//  xbt_free(temp_cluster_bw);
//  xbt_free(temp_cluster_lat);

  XBT_DEBUG("<link\tid=\"%s\" bw=\"%s\" lat=\"%s\"/>", link_backbone,cluster_bb_bw, cluster_bb_lat);
  A_surfxml_link_state = A_surfxml_link_state_ON;
  A_surfxml_link_sharing_policy = A_surfxml_link_sharing_policy_SHARED;
  if(cluster_bb_sharing_policy == A_surfxml_cluster_bb_sharing_policy_FATPIPE)
  {A_surfxml_link_sharing_policy =  A_surfxml_link_sharing_policy_FATPIPE;}
  SURFXML_BUFFER_SET(link_id, link_backbone);
  SURFXML_BUFFER_SET(link_bandwidth, cluster_bb_bw);
  SURFXML_BUFFER_SET(link_latency, cluster_bb_lat);
  SURFXML_BUFFER_SET(link_bandwidth_file, "");
  SURFXML_BUFFER_SET(link_latency_file, "");
  SURFXML_BUFFER_SET(link_state_file, "");
  SURFXML_START_TAG(link);
  SURFXML_END_TAG(link);

  XBT_DEBUG(" ");

#ifdef HAVE_PCRE_LIB
  char *new_suffix = xbt_strdup("");

  radical_elements = xbt_str_split(cluster_suffix, ".");
  xbt_dynar_foreach(radical_elements, iter, groups) {
    if (strcmp(groups, "")) {
      char *old_suffix = new_suffix;
      new_suffix = bprintf("%s\\.%s", old_suffix, groups);
      free(old_suffix);
    }
  }
  route_src_dst = bprintf("%s(.*)%s", cluster_prefix, new_suffix);
  xbt_dynar_free(&radical_elements);
  free(new_suffix);

  char *pcre_link_src = bprintf("%s_link_$1src", cluster_id);
  char *pcre_link_backbone = bprintf("%s_backbone", cluster_id);
  char *pcre_link_dst = bprintf("%s_link_$1dst", cluster_id);

  //from router to router
  XBT_DEBUG("<route\tsrc=\"%s\"\tdst=\"%s\"", router_id, router_id);
  XBT_DEBUG("symmetrical=\"NO\">");
  SURFXML_BUFFER_SET(route_src, router_id);
  SURFXML_BUFFER_SET(route_dst, router_id);
  A_surfxml_route_symmetrical = A_surfxml_route_symmetrical_NO;
  SURFXML_START_TAG(route);

  XBT_DEBUG("<link_ctn\tid=\"%s\"/>", pcre_link_backbone);
  SURFXML_BUFFER_SET(link_ctn_id, pcre_link_backbone);
  A_surfxml_link_ctn_direction = A_surfxml_link_ctn_direction_NONE;
  SURFXML_START_TAG(link_ctn);
  SURFXML_END_TAG(link_ctn);

  XBT_DEBUG("</route>");
  SURFXML_END_TAG(route);

  //from host to router
  XBT_DEBUG("<route\tsrc=\"%s\"\tdst=\"%s\"", route_src_dst, router_id);
  XBT_DEBUG("symmetrical=\"NO\">");
  SURFXML_BUFFER_SET(route_src, route_src_dst);
  SURFXML_BUFFER_SET(route_dst, router_id);
  A_surfxml_route_symmetrical = A_surfxml_route_symmetrical_NO;
  SURFXML_START_TAG(route);

  XBT_DEBUG("<link_ctn\tid=\"%s\"/>", pcre_link_src);
  SURFXML_BUFFER_SET(link_ctn_id, pcre_link_src);
  A_surfxml_link_ctn_direction = A_surfxml_link_ctn_direction_NONE;
  if(cluster_sharing_policy == A_surfxml_cluster_sharing_policy_FULLDUPLEX)
  {A_surfxml_link_ctn_direction = A_surfxml_link_ctn_direction_UP;}
  SURFXML_START_TAG(link_ctn);
  SURFXML_END_TAG(link_ctn);

  XBT_DEBUG("<link_ctn\tid=\"%s\"/>", pcre_link_backbone);
  SURFXML_BUFFER_SET(link_ctn_id, pcre_link_backbone);
  A_surfxml_link_ctn_direction = A_surfxml_link_ctn_direction_NONE;
  SURFXML_START_TAG(link_ctn);
  SURFXML_END_TAG(link_ctn);

  XBT_DEBUG("</route>");
  SURFXML_END_TAG(route);

  //from router to host
  XBT_DEBUG("<route\tsrc=\"%s\"\tdst=\"%s\"", router_id, route_src_dst);
  XBT_DEBUG("symmetrical=\"NO\">");
  SURFXML_BUFFER_SET(route_src, router_id);
  SURFXML_BUFFER_SET(route_dst, route_src_dst);
  A_surfxml_route_symmetrical = A_surfxml_route_symmetrical_NO;
  SURFXML_START_TAG(route);

  XBT_DEBUG("<link_ctn\tid=\"%s\"/>", pcre_link_backbone);
  SURFXML_BUFFER_SET(link_ctn_id, pcre_link_backbone);
  A_surfxml_link_ctn_direction = A_surfxml_link_ctn_direction_NONE;
  SURFXML_START_TAG(link_ctn);
  SURFXML_END_TAG(link_ctn);

  XBT_DEBUG("<link_ctn\tid=\"%s\"/>", pcre_link_dst);
  SURFXML_BUFFER_SET(link_ctn_id, pcre_link_dst);
  A_surfxml_link_ctn_direction = A_surfxml_link_ctn_direction_NONE;
  if(cluster_sharing_policy == A_surfxml_cluster_sharing_policy_FULLDUPLEX)
  {A_surfxml_link_ctn_direction = A_surfxml_link_ctn_direction_UP;}
  SURFXML_START_TAG(link_ctn);
  SURFXML_END_TAG(link_ctn);

  XBT_DEBUG("</route>");
  SURFXML_END_TAG(route);

  //from host to host
  XBT_DEBUG("<route\tsrc=\"%s\"\tdst=\"%s\"", route_src_dst, route_src_dst);
  XBT_DEBUG("symmetrical=\"NO\">");
  SURFXML_BUFFER_SET(route_src, route_src_dst);
  SURFXML_BUFFER_SET(route_dst, route_src_dst);
  A_surfxml_route_symmetrical = A_surfxml_route_symmetrical_NO;
  SURFXML_START_TAG(route);

  XBT_DEBUG("<link_ctn\tid=\"%s\"/>", pcre_link_src);
  SURFXML_BUFFER_SET(link_ctn_id, pcre_link_src);
  A_surfxml_link_ctn_direction = A_surfxml_link_ctn_direction_NONE;
  if(cluster_sharing_policy == A_surfxml_cluster_sharing_policy_FULLDUPLEX)
  {A_surfxml_link_ctn_direction = A_surfxml_link_ctn_direction_UP;}
  SURFXML_START_TAG(link_ctn);
  SURFXML_END_TAG(link_ctn);

  XBT_DEBUG("<link_ctn\tid=\"%s\"/>", pcre_link_backbone);
  SURFXML_BUFFER_SET(link_ctn_id, pcre_link_backbone);
  A_surfxml_link_ctn_direction = A_surfxml_link_ctn_direction_NONE;
  SURFXML_START_TAG(link_ctn);
  SURFXML_END_TAG(link_ctn);

  XBT_DEBUG("<link_ctn\tid=\"%s\"/>", pcre_link_dst);
  SURFXML_BUFFER_SET(link_ctn_id, pcre_link_dst);
  A_surfxml_link_ctn_direction = A_surfxml_link_ctn_direction_NONE;
  if(cluster_sharing_policy == A_surfxml_cluster_sharing_policy_FULLDUPLEX)
  {A_surfxml_link_ctn_direction = A_surfxml_link_ctn_direction_DOWN;}
  SURFXML_START_TAG(link_ctn);
  SURFXML_END_TAG(link_ctn);

  XBT_DEBUG("</route>");
  SURFXML_END_TAG(route);

  free(pcre_link_dst);
  free(pcre_link_backbone);
  free(pcre_link_src);
  free(route_src_dst);
#else
  for (i = 0; i <= xbt_dynar_length(tab_elements_num); i++) {
    for (j = 0; j <= xbt_dynar_length(tab_elements_num); j++) {
      if (i == xbt_dynar_length(tab_elements_num)) {
        route_src = router_id;
      } else {
        route_src =
            bprintf("%s%d%s", cluster_prefix,
                    xbt_dynar_get_as(tab_elements_num, i, int),
                    cluster_suffix);
      }

      if (j == xbt_dynar_length(tab_elements_num)) {
        route_dst = router_id;
      } else {
        route_dst =
            bprintf("%s%d%s", cluster_prefix,
                    xbt_dynar_get_as(tab_elements_num, j, int),
                    cluster_suffix);
      }

      XBT_DEBUG("<route\tsrc=\"%s\"\tdst=\"%s\"", route_src, route_dst);
      XBT_DEBUG("symmetrical=\"NO\">");
      SURFXML_BUFFER_SET(route_src, route_src);
      SURFXML_BUFFER_SET(route_dst, route_dst);
      A_surfxml_route_symmetrical = A_surfxml_route_symmetrical_NO;
      SURFXML_START_TAG(route);

      if (i != xbt_dynar_length(tab_elements_num)){
          route_src =
                bprintf("%s_link_%d", cluster_id,
                        xbt_dynar_get_as(tab_elements_num, i, int));
		  XBT_DEBUG("<link_ctn\tid=\"%s\"/>", route_src);
		  SURFXML_BUFFER_SET(link_ctn_id, route_src);
		  A_surfxml_link_ctn_direction = A_surfxml_link_ctn_direction_NONE;
		  if(cluster_sharing_policy == A_surfxml_cluster_sharing_policy_FULLDUPLEX)
		  {A_surfxml_link_ctn_direction = A_surfxml_link_ctn_direction_UP;}
		  SURFXML_START_TAG(link_ctn);
		  SURFXML_END_TAG(link_ctn);
		  free(route_src);
      }

      XBT_DEBUG("<link_ctn\tid=\"%s_backbone\"/>", cluster_id);
      SURFXML_BUFFER_SET(link_ctn_id, bprintf("%s_backbone", cluster_id));
      A_surfxml_link_ctn_direction = A_surfxml_link_ctn_direction_NONE;
      SURFXML_START_TAG(link_ctn);
      SURFXML_END_TAG(link_ctn);

      if (j != xbt_dynar_length(tab_elements_num)) {
          route_dst =
                bprintf("%s_link_%d", cluster_id,
                        xbt_dynar_get_as(tab_elements_num, j, int));
		  XBT_DEBUG("<link_ctn\tid=\"%s\"/>", route_dst);
		  SURFXML_BUFFER_SET(link_ctn_id, route_dst);
		  A_surfxml_link_ctn_direction = A_surfxml_link_ctn_direction_NONE;
		  if(cluster_sharing_policy == A_surfxml_cluster_sharing_policy_FULLDUPLEX)
		  {A_surfxml_link_ctn_direction = A_surfxml_link_ctn_direction_DOWN;}
		  SURFXML_START_TAG(link_ctn);
		  SURFXML_END_TAG(link_ctn);
		  free(route_dst);
      }

      XBT_DEBUG("</route>");
      SURFXML_END_TAG(route);
    }
  }
  xbt_dynar_free(&tab_elements_num);

#endif

  free(router_id);
  free(link_backbone);
  //free(link_router);
  xbt_dict_free(&patterns);
  free(availability_file);
  free(state_file);

  XBT_DEBUG("</AS>");
  SURFXML_END_TAG(AS);
  XBT_DEBUG(" ");

  surfxml_bufferstack_pop(1);
}
/*
 * This function take a string and replace parameters from patterns dict.
 * It returns the new value.
 */
static char* replace_random_parameter(char * string)
{
  char *test_string = NULL;

  if(xbt_dict_size(random_value)==0)
    return string;

  string = xbt_str_varsubst(string, patterns); // for patterns of cluster
  test_string = bprintf("${%s}", string);
  test_string = xbt_str_varsubst(test_string,random_value); //Add ${xxxxx} for random Generator

  if (strcmp(test_string,"")) { //if not empty, keep this value.
    xbt_free(string);
    string = test_string;
  } //In other case take old value (without ${})
  else
	free(test_string);
  return string;
}

static void clean_dict_random(void)
{
	xbt_dict_free(&random_value);
	xbt_dict_free(&patterns);
}

static void routing_parse_Speer(void)
{
  static int AX_ptr = 0;

  char *peer_id = A_surfxml_peer_id;
  char *peer_power = A_surfxml_peer_power;
  char *peer_bw_in = A_surfxml_peer_bw_in;
  char *peer_bw_out = A_surfxml_peer_bw_out;
  char *peer_lat = A_surfxml_peer_lat;
  char *peer_coord = A_surfxml_peer_coordinates;
  char *peer_state_file = A_surfxml_peer_state_file;
  char *peer_availability_file = A_surfxml_peer_availability_file;

  char *host_id = NULL;
  char *router_id, *link_router, *link_backbone, *link_id_up, *link_id_down;

  static unsigned int surfxml_buffer_stack_stack_ptr = 1;
  static unsigned int surfxml_buffer_stack_stack[1024];

  surfxml_buffer_stack_stack[0] = 0;

  surfxml_bufferstack_push(1);

  SURFXML_BUFFER_SET(AS_id, peer_id);

  SURFXML_BUFFER_SET(AS_routing, "Full");
  XBT_DEBUG("<AS id=\"%s\"\trouting=\"Full\">", peer_id);

  SURFXML_START_TAG(AS);

  XBT_DEBUG(" ");
  host_id = bprintf("peer_%s", peer_id);
  router_id = bprintf("router_%s", peer_id);
  link_id_up = bprintf("link_%s_up", peer_id);
  link_id_down = bprintf("link_%s_down", peer_id);

  link_router = bprintf("%s_link_router", peer_id);
  link_backbone = bprintf("%s_backbone", peer_id);

  XBT_DEBUG("<host\tid=\"%s\"\tpower=\"%s\"/>", host_id, peer_power);
  A_surfxml_host_state = A_surfxml_host_state_ON;
  SURFXML_BUFFER_SET(host_id, host_id);
  SURFXML_BUFFER_SET(host_power, peer_power);
  SURFXML_BUFFER_SET(host_availability, "1.0");
  SURFXML_BUFFER_SET(host_availability_file, peer_availability_file);
  SURFXML_BUFFER_SET(host_state_file, peer_state_file);
  SURFXML_BUFFER_SET(host_coordinates, "");
  SURFXML_START_TAG(host);
  SURFXML_END_TAG(host);

  XBT_DEBUG("<router id=\"%s\"\tcoordinates=\"%s\"/>", router_id, peer_coord);
  SURFXML_BUFFER_SET(router_id, router_id);
  SURFXML_BUFFER_SET(router_coordinates, peer_coord);
  SURFXML_START_TAG(router);
  SURFXML_END_TAG(router);

  XBT_DEBUG("<link\tid=\"%s\"\tbw=\"%s\"\tlat=\"%s\"/>", link_id_up, peer_bw_in, peer_lat);
  A_surfxml_link_state = A_surfxml_link_state_ON;
  A_surfxml_link_sharing_policy = A_surfxml_link_sharing_policy_SHARED;
  SURFXML_BUFFER_SET(link_id, link_id_up);
  SURFXML_BUFFER_SET(link_bandwidth, peer_bw_in);
  SURFXML_BUFFER_SET(link_latency, peer_lat);
  SURFXML_BUFFER_SET(link_bandwidth_file, "");
  SURFXML_BUFFER_SET(link_latency_file, "");
  SURFXML_BUFFER_SET(link_state_file, "");
  SURFXML_START_TAG(link);
  SURFXML_END_TAG(link);

  XBT_DEBUG("<link\tid=\"%s\"\tbw=\"%s\"\tlat=\"%s\"/>", link_id_down, peer_bw_out, peer_lat);
  A_surfxml_link_state = A_surfxml_link_state_ON;
  A_surfxml_link_sharing_policy = A_surfxml_link_sharing_policy_SHARED;
  SURFXML_BUFFER_SET(link_id, link_id_down);
  SURFXML_BUFFER_SET(link_bandwidth, peer_bw_out);
  SURFXML_BUFFER_SET(link_latency, peer_lat);
  SURFXML_BUFFER_SET(link_bandwidth_file, "");
  SURFXML_BUFFER_SET(link_latency_file, "");
  SURFXML_BUFFER_SET(link_state_file, "");
  SURFXML_START_TAG(link);
  SURFXML_END_TAG(link);

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
  SURFXML_END_TAG(AS);
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
	  surf_parse_get_double(&mean,A_surfxml_random_mean);
	  surf_parse_get_double(&std,A_surfxml_random_std_deviation);
	  surf_parse_get_double(&min,A_surfxml_random_min);
	  surf_parse_get_double(&max,A_surfxml_random_max);
	  surf_parse_get_double(&seed,A_surfxml_random_seed);

	  double res = 0;
	  int i = 0;
	  random_data_t random = xbt_new0(s_random_data_t, 1);
          char *tmpbuf;

	  xbt_dynar_t radical_elements;
	  unsigned int iter;
	  char *groups;
	  int start, end;
	  xbt_dynar_t radical_ends;

	  random->generator = A_surfxml_random_generator;
	  random->seed = seed;
	  random->min = min;
	  random->max = max;

	  /* Check user stupidities */
	  if (max < min)
	    THROWF(arg_error, 0, "random->max < random->min (%f < %f)", max, min);
	  if (mean < min)
	    THROWF(arg_error, 0, "random->mean < random->min (%f < %f)", mean,
		   min);
	  if (mean > max)
	    THROWF(arg_error, 0, "random->mean > random->max (%f > %f)", mean,
		   max);

	  /* normalize the mean and standard deviation before storing */
	  random->mean = (mean - min) / (max - min);
	  random->std = std / (max - min);

	  if (random->mean * (1 - random->mean) < random->std * random->std)
	    THROWF(arg_error, 0, "Invalid mean and standard deviation (%f and %f)",
		   random->mean, random->std);

	  XBT_DEBUG("id = '%s' min = '%f' max = '%f' mean = '%f' std_deviatinon = '%f' generator = '%d' seed = '%ld' radical = '%s'",
	  random_id,
	  random->min,
	  random->max,
	  random->mean,
	  random->std,
	  random->generator,
	  random->seed,
	  random_radical);

	  if(xbt_dict_size(random_value)==0)
		  random_value = xbt_dict_new();

	  if(!strcmp(random_radical,""))
	  {
		  res = random_generate(random);
		  rd_value = bprintf("%f",res);
		  xbt_dict_set(random_value, random_id, rd_value, free);
	  }
	  else
	  {
		  radical_elements = xbt_str_split(random_radical, ",");
		  xbt_dynar_foreach(radical_elements, iter, groups) {
			radical_ends = xbt_str_split(groups, "-");
			switch (xbt_dynar_length(radical_ends)) {
			case 1:
					  xbt_assert(!xbt_dict_get_or_null(random_value,random_id),"Custom Random '%s' already exists !",random_id);
					  res = random_generate(random);
                                          tmpbuf = bprintf("%s%d",random_id,atoi(xbt_dynar_getfirst_as(radical_ends,char *)));
                                          xbt_dict_set(random_value, tmpbuf, bprintf("%f",res), free);
                                          xbt_free(tmpbuf);
					  break;

			case 2:	  surf_parse_get_int(&start,
										 xbt_dynar_get_as(radical_ends, 0, char *));
					  surf_parse_get_int(&end, xbt_dynar_get_as(radical_ends, 1, char *));
					  for (i = start; i <= end; i++) {
						  xbt_assert(!xbt_dict_get_or_null(random_value,random_id),"Custom Random '%s' already exists !",bprintf("%s%d",random_id,i));
						  res = random_generate(random);
                          tmpbuf = bprintf("%s%d",random_id,i);
						  xbt_dict_set(random_value, tmpbuf, bprintf("%f",res), free);
                          xbt_free(tmpbuf);
					  }
					  break;
			default:
				XBT_INFO("Malformed radical");
			}
			res = random_generate(random);
			rd_name  = bprintf("%s_router",random_id);
			rd_value = bprintf("%f",res);
			xbt_dict_set(random_value, rd_name, rd_value, free);

			xbt_dynar_free(&radical_ends);
		  }
		  free(rd_name);
		  xbt_dynar_free(&radical_elements);
	  }
}

static void routing_parse_Erandom(void)
{
	xbt_dict_cursor_t cursor = NULL;
	char *key;
	char *elem;

	xbt_dict_foreach(random_value, cursor, key, elem) {
	  XBT_DEBUG("%s = %s",key,elem);
	}

}


/*
 * New methods to init the routing model component from the lua script
 */

/*
 * calling parse_S_AS_lua with lua values
 */
void routing_AS_init(const char *AS_id, const char *AS_routing)
{
  parse_S_AS_lua((char *) AS_id, (char *) AS_routing);
}

/*
 * calling parse_E_AS_lua to fisnish the creation of routing component
 */
void routing_AS_end(const char *AS_id)
{
  parse_E_AS_lua((char *) AS_id);
}

/*
 * add a host to the network element list
 */

void routing_add_host(const char *host_id)
{
  parse_S_host_lua((char *) host_id, (char*)""); // FIXME propagate coordinate system to lua
}

/*
 * Set a new link on the actual list of link for a route or ASroute
 */
void routing_add_link(const char *link_id)
{
  parse_E_link_c_ctn_new_elem_lua((char *) link_id);
}

/*
 *Set the endpoints for a route
 */
void routing_set_route(const char *src_id, const char *dst_id)
{
  parse_S_route_new_and_endpoints_lua(src_id, dst_id);
}

/*
 * Store the route by calling parse_E_route_store_route
 */
void routing_store_route(void)
{
  parse_E_route_store_route();
}
