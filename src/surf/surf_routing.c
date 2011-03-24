/* Copyright (c) 2009, 2010. The SimGrid Team. 
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */



#include <float.h>
#include "gras_config.h"

#ifdef HAVE_PCRE_LIB
#include <pcre.h>               /* regular expresion library */
#endif
#include "surf_private.h"
#include "xbt/dynar.h"
#include "xbt/str.h"
#include "xbt/config.h"
#include "xbt/graph.h"
#include "xbt/set.h"
#include "surf/surfxml_parse.h"

xbt_dict_t patterns = NULL;
xbt_dict_t random_value = NULL;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route, surf, "Routing part of surf");

/* Global vars */
routing_global_t global_routing = NULL;
routing_component_t current_routing = NULL;
model_type_t current_routing_model = NULL;
static double_f_cpvoid_t get_link_latency = NULL;

/* Prototypes of each model */
static void *model_full_create(void);   /* create structures for full routing model */
static void model_full_load(void);      /* load parse functions for full routing model */
static void model_full_unload(void);    /* unload parse functions for full routing model */
static void model_full_end(void);       /* finalize the creation of full routing model */
static void model_full_set_route(		/* Set the route and ASroute between src and dst */
		routing_component_t rc, const char *src, const char *dst, name_route_extended_t route);

static void *model_floyd_create(void);  /* create structures for floyd routing model */
static void model_floyd_load(void);     /* load parse functions for floyd routing model */
static void model_floyd_unload(void);   /* unload parse functions for floyd routing model */
static void model_floyd_end(void);      /* finalize the creation of floyd routing model */
static void model_floyd_set_route(routing_component_t rc, const char *src,
        const char *dst, name_route_extended_t route);

static void *model_dijkstra_both_create(int cached);    /* create by calling dijkstra or dijkstracache */
static void *model_dijkstra_create(void);       /* create structures for dijkstra routing model */
static void *model_dijkstracache_create(void);  /* create structures for dijkstracache routing model */
static void model_dijkstra_both_load(void);     /* load parse functions for dijkstra routing model */
static void model_dijkstra_both_unload(void);   /* unload parse functions for dijkstra routing model */
static void model_dijkstra_both_end(void);      /* finalize the creation of dijkstra routing model */
static void model_dijkstra_both_set_route (routing_component_t rc, const char *src,
                     const char *dst, name_route_extended_t route);

static void *model_rulebased_create(void);      /* create structures for rulebased routing model */
static void model_rulebased_load(void); /* load parse functions for rulebased routing model */
static void model_rulebased_unload(void);       /* unload parse functions for rulebased routing model */
static void model_rulebased_end(void);  /* finalize the creation of rulebased routing model */

static void *model_none_create(void);   /* none routing model */
static void model_none_load(void);      /* none routing model */
static void model_none_unload(void);    /* none routing model */
static void model_none_end(void);       /* none routing model */

static void routing_parse_Scluster(void);  /*cluster bypass */
static void routing_parse_Speer(void);  	/*peer bypass */
static void routing_parse_Srandom(void);  	/*random bypass */
static void routing_parse_Erandom(void);  	/*random bypass */

static void routing_parse_Sconfig(void);        /*config Tag */
static void routing_parse_Econfig(void);        /*config Tag */

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


/* must be finish with null and carefull if change de order */
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

/* ************************************************************************** */
/* ***************** GENERIC PARSE FUNCTIONS (declarations) ***************** */

static void generic_set_processing_unit(routing_component_t rc,
                                        const char *name);
static void generic_set_autonomous_system(routing_component_t rc,
                                          const char *name);
static void generic_set_bypassroute(routing_component_t rc,
                                    const char *src, const char *dst,
                                    route_extended_t e_route);

static int surf_link_resource_cmp(const void *a, const void *b);
static int surf_pointer_resource_cmp(const void *a, const void *b);

/* ************************************************************************** */
/* *************** GENERIC BUSINESS METHODS (declarations) ****************** */

static double generic_get_link_latency(routing_component_t rc, const char *src, const char *dst);
static xbt_dynar_t generic_get_onelink_routes(routing_component_t rc);
static route_extended_t generic_get_bypassroute(routing_component_t rc,
                                                const char *src,
                                                const char *dst);

/* ************************************************************************** */
/* ****************** GENERIC AUX FUNCTIONS (declarations) ****************** */

static route_extended_t
generic_new_extended_route(e_surf_routing_hierarchy_t hierarchy,
                           void *data, int order);
static route_t
generic_new_route(e_surf_routing_hierarchy_t hierarchy,
                           void *data, int order);
static void generic_free_route(route_t route);
static void generic_free_extended_route(route_extended_t e_route);
static routing_component_t
generic_autonomous_system_exist(routing_component_t rc, char *element);
static routing_component_t
generic_processing_units_exist(routing_component_t rc, char *element);
static void generic_src_dst_check(routing_component_t rc, const char *src,
                                  const char *dst);

/* ************************************************************************** */
/* **************************** GLOBAL FUNCTIONS **************************** */

/* global parse functions */
static const char *src = NULL;        /* temporary store the source name of a route */
static const char *dst = NULL;        /* temporary store the destination name of a route */
static char *gw_src = NULL;     /* temporary store the gateway source name of a route */
static char *gw_dst = NULL;     /* temporary store the gateway destination name of a route */
static xbt_dynar_t link_list = NULL;    /* temporary store of current list link of a route */

static xbt_dict_t coordinates = NULL;


static double eculidean_dist_comp(int index, xbt_dynar_t src, xbt_dynar_t dst)
{
	double src_coord, dst_coord;

	src_coord = atof(xbt_dynar_get_as(src, index, char *));
	dst_coord = atof(xbt_dynar_get_as(dst, index, char *));

	return (src_coord-dst_coord)*(src_coord-dst_coord);

}

static double vivaldi_get_link_latency (routing_component_t rc,const char *src, const char *dst)
{
  double euclidean_dist;
  xbt_dynar_t src_ctn, dst_ctn;
  src_ctn = xbt_dict_get(coordinates, src);
  dst_ctn = xbt_dict_get(coordinates, dst);

  euclidean_dist = sqrt (eculidean_dist_comp(0,src_ctn,dst_ctn)+eculidean_dist_comp(1,src_ctn,dst_ctn))
  								+fabs(atof(xbt_dynar_get_as(src_ctn, 2, char *)))+fabs(atof(xbt_dynar_get_as(dst_ctn, 2, char *)));

  xbt_assert2(euclidean_dist>=0, "Euclidean Dist is less than 0\"%s\" and \"%.2f\"", src, euclidean_dist);

  return euclidean_dist;

  /*
  x = atof(xbt_dynar_get_as(src_ctn, 0, char *))-atof(xbt_dynar_get_as(dst_ctn, 0, char *));
  y = atof(xbt_dynar_get_as(src_ctn, 1, char *));
  h = atof(xbt_dynar_get_as(ctn, 2, char *));
  sqrt((c1->x - c2->x) * (c1->x - c2->x) + (c1->y - c2->y) * (c1->y - c2->y)) + fabs(c1->h) + fabs(c2->h);

	  if (strcmp(coord,"")) {
  	xbt_dynar_t ctn = xbt_str_split_str(coord, " ");
  	xbt_dynar_shrink(ctn,0);
   	xbt_dict_set (coordinates,host_id,ctn,NULL);
  }
	*/
}

/**
 * \brief Add a "host" to the network element list
 */
static void parse_S_host(const char *host_id, const char* coord)
{
  network_element_info_t info = NULL;
  if (current_routing->hierarchy == SURF_ROUTING_NULL)
    current_routing->hierarchy = SURF_ROUTING_BASE;
  xbt_assert1(!xbt_lib_get_or_null(host_lib, host_id,ROUTING_HOST_LEVEL),
              "Reading a host, processing unit \"%s\" already exists",
              host_id);
  xbt_assert1(current_routing->set_processing_unit,
              "no defined method \"set_processing_unit\" in \"%s\"",
              current_routing->name);
  (*(current_routing->set_processing_unit)) (current_routing, host_id);
  info = xbt_new0(s_network_element_info_t, 1);
  info->rc_component = current_routing;
  info->rc_type = SURF_NETWORK_ELEMENT_HOST;
  xbt_lib_set(host_lib,host_id,ROUTING_HOST_LEVEL,(void *) info);
  if (strcmp(coord,"")) {
    xbt_dynar_t ctn = xbt_str_split_str(coord, " ");
    xbt_dynar_shrink(ctn, 0);
    xbt_dict_set(coordinates, host_id, ctn, xbt_dynar_free_voidp);
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
  xbt_assert1(!xbt_lib_get_or_null(as_router_lib,A_surfxml_router_id, ROUTING_ASR_LEVEL),
              "Reading a router, processing unit \"%s\" already exists",
              router_id);
  xbt_assert1(current_routing->set_processing_unit,
              "no defined method \"set_processing_unit\" in \"%s\"",
              current_routing->name);
  (*(current_routing->set_processing_unit)) (current_routing,
                                             router_id);
  info = xbt_new0(s_network_element_info_t, 1);
  info->rc_component = current_routing;
  info->rc_type = SURF_NETWORK_ELEMENT_ROUTER;

  xbt_lib_set(as_router_lib,router_id,ROUTING_ASR_LEVEL,(void *) info);
  if (strcmp(A_surfxml_router_coordinates,"")) {
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
    THROW2(arg_error, 0, "Route between %s to %s can not be defined",
           src_id, dst_id);
  src = src_id;
  dst = dst_id;
  xbt_assert2(strlen(src) > 0 || strlen(dst) > 0,
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
    THROW2(arg_error, 0, "Route between %s to %s can not be defined",
           A_surfxml_ASroute_src, A_surfxml_ASroute_dst);
  src = A_surfxml_ASroute_src;
  dst = A_surfxml_ASroute_dst;
  gw_src = A_surfxml_ASroute_gw_src;
  gw_dst = A_surfxml_ASroute_gw_dst;
  xbt_assert2(strlen(src) > 0 || strlen(dst) > 0 || strlen(gw_src) > 0
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
    THROW2(arg_error, 0,
           "Bypass Route between %s to %s can not be defined",
           A_surfxml_bypassRoute_src, A_surfxml_bypassRoute_dst);
  src = A_surfxml_bypassRoute_src;
  dst = A_surfxml_bypassRoute_dst;
  gw_src = A_surfxml_bypassRoute_gw_src;
  gw_dst = A_surfxml_bypassRoute_gw_dst;
  xbt_assert2(strlen(src) > 0 || strlen(dst) > 0 || strlen(gw_src) > 0
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
  xbt_assert1(current_routing->set_route,
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
  xbt_assert1(current_routing->set_ASroute,
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
  xbt_assert1(current_routing->set_bypassroute,
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

    xbt_assert1(!xbt_dict_get_or_null
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
    THROW0(arg_error, 0, "All defined components must be belong to a AS");
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

  if (strcmp(A_surfxml_AS_coordinates,"")) {
    XBT_DEBUG("%s coordinates : %s", A_surfxml_AS_id, A_surfxml_AS_coordinates);
    xbt_dynar_t ctn = xbt_str_split_str(A_surfxml_AS_coordinates, " ");
    xbt_dynar_shrink(ctn, 0);
    xbt_dict_set(coordinates, A_surfxml_AS_id, ctn, xbt_dynar_free_voidp);
  }
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
    THROW1(arg_error, 0, "Close AS(%s), that never open", AS_id);
  } else {
    network_element_info_t info = NULL;
    xbt_assert1(!xbt_lib_get_or_null(as_router_lib,current_routing->name, ROUTING_ASR_LEVEL),
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
static xbt_dynar_t elements_father(const char *src, const char *dst)
{

  xbt_assert0(src && dst, "bad parameters for \"elements_father\" method");

  xbt_dynar_t result = xbt_dynar_new(sizeof(char *), NULL);

  routing_component_t src_as, dst_as;
  int index_src, index_dst, index_father_src, index_father_dst;
  xbt_dynar_t path_src = NULL;
  xbt_dynar_t path_dst = NULL;
  routing_component_t current = NULL;
  routing_component_t *current_src = NULL;
  routing_component_t *current_dst = NULL;
  routing_component_t *father = NULL;

  /* (1) find the as where the src and dst are located */
  void * src_data = xbt_lib_get_or_null(host_lib,src, ROUTING_HOST_LEVEL);
  void * dst_data = xbt_lib_get_or_null(host_lib,dst, ROUTING_HOST_LEVEL);
  if(!src_data) src_data = xbt_lib_get_or_null(as_router_lib,src, ROUTING_ASR_LEVEL);
  if(!dst_data) dst_data = xbt_lib_get_or_null(as_router_lib,dst, ROUTING_ASR_LEVEL);
  src_as = ((network_element_info_t)src_data)->rc_component;
  dst_as = ((network_element_info_t)dst_data)->rc_component;

  xbt_assert2(src_as
              && dst_as,
              "Ask for route \"from\"(%s) or \"to\"(%s) no found", src,
              dst);

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
    current_src = xbt_dynar_get_ptr(path_src, index_src);
    current_dst = xbt_dynar_get_ptr(path_dst, index_dst);
    index_src--;
    index_dst--;
  }
  index_src++;
  index_dst++;
  current_src = xbt_dynar_get_ptr(path_src, index_src);
  current_dst = xbt_dynar_get_ptr(path_dst, index_dst);

  /* (4) they are not in the same routing component, make the path */
  index_father_src = index_src + 1;
  index_father_dst = index_dst + 1;

  if (*current_src == *current_dst)
    father = current_src;
  else
    father = xbt_dynar_get_ptr(path_src, index_father_src);

  /* (5) result generation */
  xbt_dynar_push(result, father);       /* first same the father of src and dst */
  xbt_dynar_push(result, current_src);  /* second the first different father of src */
  xbt_dynar_push(result, current_dst);  /* three  the first different father of dst */

  xbt_dynar_free(&path_src);
  xbt_dynar_free(&path_dst);

  return result;
}

/* Global Business methods */

/**
 * \brief Recursive function for get_route
 *
 * \param src the source host name 
 * \param dst the destination host name
 * 
 * This fuction is call by "get_route". It allow to walk through the 
 * routing components tree.
 */
static route_extended_t _get_route(const char *src, const char *dst)
{

  void *link;
  unsigned int cpt = 0;

  XBT_DEBUG("Solve route  \"%s\" to \"%s\"", src, dst);

  xbt_assert0(src && dst, "bad parameters for \"_get_route\" method");

  route_extended_t e_route, e_route_cnt, e_route_src = NULL, e_route_dst =
      NULL;

  xbt_dynar_t elem_father_list = elements_father(src, dst);

  routing_component_t common_father =
      xbt_dynar_get_as(elem_father_list, 0, routing_component_t);
  routing_component_t src_father =
      xbt_dynar_get_as(elem_father_list, 1, routing_component_t);
  routing_component_t dst_father =
      xbt_dynar_get_as(elem_father_list, 2, routing_component_t);

  e_route = xbt_new0(s_route_extended_t, 1);
  e_route->src_gateway = NULL;
  e_route->dst_gateway = NULL;
  e_route->generic_route.link_list =
      xbt_dynar_new(global_routing->size_of_link, NULL);

  if (src_father == dst_father) {       /* SURF_ROUTING_BASE */

    if (strcmp(src, dst)) {
      e_route_cnt =
          (*(common_father->get_route)) (common_father, src, dst);
      xbt_assert2(e_route_cnt, "no route between \"%s\" and \"%s\"", src,
                  dst);
      xbt_dynar_foreach(e_route_cnt->generic_route.link_list, cpt, link) {
        xbt_dynar_push(e_route->generic_route.link_list, &link);
      }
      generic_free_extended_route(e_route_cnt);
    }

  } else {                      /* SURF_ROUTING_RECURSIVE */

    route_extended_t e_route_bypass = NULL;

    if (common_father->get_bypass_route)
      e_route_bypass =
          (*(common_father->get_bypass_route)) (common_father, src, dst);

    if (e_route_bypass)
      e_route_cnt = e_route_bypass;
    else
      e_route_cnt =
          (*(common_father->get_route)) (common_father, src_father->name,
                                         dst_father->name);

    xbt_assert2(e_route_cnt, "no route between \"%s\" and \"%s\"",
                src_father->name, dst_father->name);

    xbt_assert2((e_route_cnt->src_gateway == NULL) ==
                (e_route_cnt->dst_gateway == NULL),
                "bad gateway for route between \"%s\" and \"%s\"", src,
                dst);

    if (strcmp(src, e_route_cnt->src_gateway)) {
      e_route_src = _get_route(src, e_route_cnt->src_gateway);
      xbt_assert2(e_route_src, "no route between \"%s\" and \"%s\"", src,
                  e_route_cnt->src_gateway);
      xbt_dynar_foreach(e_route_src->generic_route.link_list, cpt, link) {
        xbt_dynar_push(e_route->generic_route.link_list, &link);
      }
    }

    xbt_dynar_foreach(e_route_cnt->generic_route.link_list, cpt, link) {
      xbt_dynar_push(e_route->generic_route.link_list, &link);
    }

    if (strcmp(e_route_cnt->dst_gateway, dst)) {
      e_route_dst = _get_route(e_route_cnt->dst_gateway, dst);
      xbt_assert2(e_route_dst, "no route between \"%s\" and \"%s\"",
                  e_route_cnt->dst_gateway, dst);
      xbt_dynar_foreach(e_route_dst->generic_route.link_list, cpt, link) {
        xbt_dynar_push(e_route->generic_route.link_list, &link);
      }
    }

    e_route->src_gateway = xbt_strdup(e_route_cnt->src_gateway);
    e_route->dst_gateway = xbt_strdup(e_route_cnt->dst_gateway);

    generic_free_extended_route(e_route_src);
    generic_free_extended_route(e_route_cnt);
    generic_free_extended_route(e_route_dst);
  }

  xbt_dynar_free(&elem_father_list);

  return e_route;
}

static double _get_latency(const char *src, const char *dst)
{
  double latency, latency_src, latency_dst = 0.0;

  XBT_DEBUG("Solve route  \"%s\" to \"%s\"", src, dst);
  xbt_assert0(src && dst, "bad parameters for \"_get_route\" method");

  route_extended_t e_route_cnt;

  xbt_dynar_t elem_father_list = elements_father(src, dst);

  routing_component_t common_father =
      xbt_dynar_get_as(elem_father_list, 0, routing_component_t);
  routing_component_t src_father =
      xbt_dynar_get_as(elem_father_list, 1, routing_component_t);
  routing_component_t dst_father =
      xbt_dynar_get_as(elem_father_list, 2, routing_component_t);

  if (src_father == dst_father) {       /* SURF_ROUTING_BASE */

    if (strcmp(src, dst)) {
      latency =
          (*(common_father->get_latency)) (common_father, src, dst);
      xbt_assert2(latency>=0, "no route between \"%s\" and \"%s\"", src,
                  dst);
     } else latency = 0;
  } else {                      /* SURF_ROUTING_RECURSIVE */
     route_extended_t e_route_bypass = NULL;
    if (common_father->get_bypass_route)
      e_route_bypass =
          (*(common_father->get_bypass_route)) (common_father, src, dst);

    xbt_assert0(!e_route_bypass,"Bypass cannot work yet with get_latency"); 
                                                 
    e_route_cnt =
          (*(common_father->get_route)) (common_father, src_father->name,
                                         dst_father->name);

    xbt_assert2(e_route_cnt, "no route between \"%s\" and \"%s\"",
                src_father->name, dst_father->name);

    xbt_assert2((e_route_cnt->src_gateway == NULL) ==
                (e_route_cnt->dst_gateway == NULL),
                "bad gateway for route between \"%s\" and \"%s\"", src,
                dst);            
    latency =
          (*(common_father->get_latency)) (common_father, elements_As_name(src),
        		  elements_As_name(dst));

    xbt_assert2(latency>=0, "no route between \"%s\" and \"%s\"",
                src_father->name, dst_father->name);
    

    if (src != e_route_cnt->src_gateway) {

      latency_src = _get_latency(src, e_route_cnt->src_gateway);
      xbt_assert2(latency_src>=0, "no route between \"%s\" and \"%s\"", src,
                  e_route_cnt->src_gateway);
      latency += latency_src;
    }

    if (e_route_cnt->dst_gateway != dst) {
    
      latency_dst = _get_latency(e_route_cnt->dst_gateway, dst);
      xbt_assert2(latency_dst>=0, "no route between \"%s\" and \"%s\"",
                  e_route_cnt->dst_gateway, dst);
      latency += latency_dst;
    }
	
  }

  xbt_dynar_free(&elem_father_list);

  return latency;
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

  route_extended_t e_route;
  xbt_dynar_t elem_father_list = NULL;
  routing_component_t common_father = NULL;

  if (strcmp(src, dst))
    e_route = _get_route(src, dst);
  else {
    elem_father_list = elements_father(src, dst);
    common_father =
        xbt_dynar_get_as(elem_father_list, 0, routing_component_t);

    e_route = (*(common_father->get_route)) (common_father, src, dst);
    xbt_dynar_free(&elem_father_list);
  }

  xbt_assert2(e_route, "no route between \"%s\" and \"%s\"", src, dst);

  if (global_routing->last_route)
    xbt_dynar_free(&(global_routing->last_route));
  global_routing->last_route = e_route->generic_route.link_list;

  if (e_route->src_gateway)
    xbt_free(e_route->src_gateway);
  if (e_route->dst_gateway)
    xbt_free(e_route->dst_gateway);

  xbt_free(e_route);

/*
  if (xbt_dynar_length(global_routing->last_route) == 0)
    return NULL;
  else
*/
    return global_routing->last_route;
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
	xbt_dynar_t d = get_route(src,dst);
	global_routing->last_route = NULL;
	return d;
}

/*Get Latency*/
static double get_latency(const char *src, const char *dst)
{

  double latency = -1.0;
  xbt_dynar_t elem_father_list = elements_father(src, dst);
  routing_component_t common_father =
      xbt_dynar_get_as(elem_father_list, 0, routing_component_t);

  if (strcmp(src, dst))
    latency = _get_latency(src, dst);
  else
    latency = (*(common_father->get_latency)) (common_father, src, dst);

  xbt_assert2(latency>=0.0, "no route between \"%s\" and \"%s\"", src, dst);
  xbt_dynar_free(&elem_father_list);

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
    xbt_assert1(rc->finalize, "no defined method \"finalize\" in \"%s\"",
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
  xbt_dict_free(&(coordinates));
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

  coordinates = xbt_dict_new();

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
	surfxml_add_callback(STag_surfxml_prop_cb_list, &parse_properties);
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

/* ************************************************************************** */
/* *************************** FULL ROUTING ********************************* */

#define TO_ROUTE_FULL(i,j) routing->routing_table[(i)+(j)*table_size]

/* Routing model structure */

typedef struct {
  s_routing_component_t generic_routing;
  route_extended_t *routing_table;
} s_routing_component_full_t, *routing_component_full_t;

/* Business methods */
static xbt_dynar_t full_get_onelink_routes(routing_component_t rc)
{
  xbt_dynar_t ret = xbt_dynar_new(sizeof(onelink_t), xbt_free);

  routing_component_full_t routing = (routing_component_full_t) rc;
  size_t table_size = xbt_dict_length(routing->generic_routing.to_index);
  xbt_dict_cursor_t c1 = NULL, c2 = NULL;
  char *k1, *d1, *k2, *d2;
  xbt_dict_foreach(routing->generic_routing.to_index, c1, k1, d1) {
    xbt_dict_foreach(routing->generic_routing.to_index, c2, k2, d2) {
      int *src_id = xbt_dict_get_or_null(routing->generic_routing.to_index, k1);
      int *dst_id = xbt_dict_get_or_null(routing->generic_routing.to_index, k2);
      xbt_assert2(src_id
                  && dst_id,
                  "Ask for route \"from\"(%s)  or \"to\"(%s) no found in the local table",
                  src, dst);
      route_extended_t route = TO_ROUTE_FULL(*src_id, *dst_id);
      if (route) {
        if (xbt_dynar_length(route->generic_route.link_list) == 1) {
          void *link =
              *(void **) xbt_dynar_get_ptr(route->generic_route.link_list,
                                           0);
          onelink_t onelink = xbt_new0(s_onelink_t, 1);
          onelink->link_ptr = link;
          if (routing->generic_routing.hierarchy == SURF_ROUTING_BASE) {
            onelink->src = xbt_strdup(k1);
            onelink->dst = xbt_strdup(k2);
          } else if (routing->generic_routing.hierarchy ==
                     SURF_ROUTING_RECURSIVE) {
            onelink->src = xbt_strdup(route->src_gateway);
            onelink->dst = xbt_strdup(route->dst_gateway);
          }
          xbt_dynar_push(ret, &onelink);
        }
      }
    }
  }
  return ret;
}

static route_extended_t full_get_route(routing_component_t rc,
                                       const char *src, const char *dst)
{
  xbt_assert1(rc && src
              && dst,
              "Invalid params for \"get_route\" function at AS \"%s\"",
              rc->name);

  /* set utils vars */
  routing_component_full_t routing = (routing_component_full_t) rc;
  size_t table_size = xbt_dict_length(routing->generic_routing.to_index);

  generic_src_dst_check(rc, src, dst);
  int *src_id = xbt_dict_get_or_null(routing->generic_routing.to_index, src);
  int *dst_id = xbt_dict_get_or_null(routing->generic_routing.to_index, dst);
  xbt_assert2(src_id
              && dst_id,
              "Ask for route \"from\"(%s)  or \"to\"(%s) no found in the local table",
              src, dst);

  route_extended_t e_route = NULL;
  route_extended_t new_e_route = NULL;
  void *link;
  unsigned int cpt = 0;

  e_route = TO_ROUTE_FULL(*src_id, *dst_id);

  if (e_route) {
    new_e_route = xbt_new0(s_route_extended_t, 1);
    new_e_route->src_gateway = xbt_strdup(e_route->src_gateway);
    new_e_route->dst_gateway = xbt_strdup(e_route->dst_gateway);
    new_e_route->generic_route.link_list =
        xbt_dynar_new(global_routing->size_of_link, NULL);
    xbt_dynar_foreach(e_route->generic_route.link_list, cpt, link) {
      xbt_dynar_push(new_e_route->generic_route.link_list, &link);
    }
  }
  return new_e_route;
}

static void full_finalize(routing_component_t rc)
{
  routing_component_full_t routing = (routing_component_full_t) rc;
  size_t table_size = xbt_dict_length(routing->generic_routing.to_index);
  int i, j;
  if (routing) {
    /* Delete routing table */
    for (i = 0; i < table_size; i++)
      for (j = 0; j < table_size; j++)
        generic_free_extended_route(TO_ROUTE_FULL(i, j));
    xbt_free(routing->routing_table);
    /* Delete bypass dict */
    xbt_dict_free(&rc->bypassRoutes);
    /* Delete index dict */
    xbt_dict_free(&rc->to_index);
    /* Delete structure */
    xbt_free(rc);
  }
}

/* Creation routing model functions */

static void *model_full_create(void)
{
  routing_component_full_t new_component =
      xbt_new0(s_routing_component_full_t, 1);
  new_component->generic_routing.set_processing_unit =
      generic_set_processing_unit;
  new_component->generic_routing.set_autonomous_system =
      generic_set_autonomous_system;
  new_component->generic_routing.set_route = model_full_set_route;
  new_component->generic_routing.set_ASroute = model_full_set_route;
  new_component->generic_routing.set_bypassroute = generic_set_bypassroute;
  new_component->generic_routing.get_route = full_get_route;
  new_component->generic_routing.get_latency = generic_get_link_latency;
  new_component->generic_routing.get_onelink_routes =
      full_get_onelink_routes;
  new_component->generic_routing.get_bypass_route =
      generic_get_bypassroute;
  new_component->generic_routing.finalize = full_finalize;
  new_component->generic_routing.to_index = xbt_dict_new();
  new_component->generic_routing.bypassRoutes = xbt_dict_new();
  new_component->generic_routing.get_network_element_type = get_network_element_type;
  return new_component;
}

static void model_full_load(void)
{
  /* use "surfxml_add_callback" to add a parse function call */
}

static void model_full_unload(void)
{
  /* use "surfxml_del_callback" to remove a parse function call */
}

static void model_full_end(void)
{
  unsigned int i;
  route_extended_t e_route;

  /* set utils vars */
  routing_component_full_t routing =
      ((routing_component_full_t) current_routing);
  size_t table_size = xbt_dict_length(routing->generic_routing.to_index);

  /* Create table if necessary */
  if(!routing->routing_table)
	  routing->routing_table = xbt_new0(route_extended_t, table_size * table_size);

  /* Add the loopback if needed */
  if (current_routing->hierarchy == SURF_ROUTING_BASE) {
    for (i = 0; i < table_size; i++) {
      e_route = TO_ROUTE_FULL(i, i);
      if (!e_route) {
        e_route = xbt_new0(s_route_extended_t, 1);
        e_route->src_gateway = NULL;
        e_route->dst_gateway = NULL;
        e_route->generic_route.link_list =
            xbt_dynar_new(global_routing->size_of_link, NULL);
        xbt_dynar_push(e_route->generic_route.link_list,
                       &global_routing->loopback);
        TO_ROUTE_FULL(i, i) = e_route;
      }
    }
  }
}

static void model_full_set_route(routing_component_t rc, const char *src,
		const char *dst, name_route_extended_t route)
{
	int *src_id, *dst_id;
	src_id = xbt_dict_get_or_null(rc->to_index, src);
	dst_id = xbt_dict_get_or_null(rc->to_index, dst);
	routing_component_full_t routing = ((routing_component_full_t) rc);
	size_t table_size = xbt_dict_length(routing->generic_routing.to_index);

	xbt_assert2(src_id
			  && dst_id, "Network elements %s or %s not found", src, dst);

	xbt_assert2(xbt_dynar_length(route->generic_route.link_list) > 0,
			  "Invalid count of links, must be greater than zero (%s,%s)",
			  src, dst);

	if(!routing->routing_table)
		routing->routing_table = xbt_new0(route_extended_t, table_size * table_size);

	if(TO_ROUTE_FULL(*src_id, *dst_id))
	{
		char * link_name;
		unsigned int i;
		xbt_dynar_t link_route_to_test = xbt_dynar_new(global_routing->size_of_link, NULL);
		xbt_dynar_foreach(route->generic_route.link_list,i,link_name)
		{
			void *link = xbt_dict_get_or_null(surf_network_model->resource_set, link_name);
			xbt_assert1(link,"Link : '%s' doesn't exists.",link_name);
			xbt_dynar_push(link_route_to_test,&link);
		}
		xbt_assert2(!xbt_dynar_compare(
			  (void*)TO_ROUTE_FULL(*src_id, *dst_id)->generic_route.link_list,
			  (void*)link_route_to_test,
			  (int_f_cpvoid_cpvoid_t) surf_pointer_resource_cmp),
			  "The route between \"%s\" and \"%s\" already exists. If you are trying to define a reverse route, you must set the symmetrical=no attribute to your routes tags.", src,dst);
	}
	else
	{
		  if(!route->dst_gateway && !route->src_gateway)
			  XBT_DEBUG("Load Route from \"%s\" to \"%s\"", src, dst);
		  else{
			  XBT_DEBUG("Load ASroute from \"%s(%s)\" to \"%s(%s)\"", src,
			         route->src_gateway, dst, route->dst_gateway);
			  if(global_routing->get_network_element_type((const char*)route->dst_gateway) == SURF_NETWORK_ELEMENT_NULL)
				  xbt_die("The dst_gateway '%s' does not exist!",route->dst_gateway);
			  if(global_routing->get_network_element_type((const char*)route->src_gateway) == SURF_NETWORK_ELEMENT_NULL)
				  xbt_die("The src_gateway '%s' does not exist!",route->src_gateway);
		  }
	      TO_ROUTE_FULL(*src_id, *dst_id) = generic_new_extended_route(rc->hierarchy,route,1);
	      xbt_dynar_shrink(TO_ROUTE_FULL(*src_id, *dst_id)->generic_route.link_list, 0);
	}

	if( A_surfxml_route_symmetrical == A_surfxml_route_symmetrical_YES
		|| A_surfxml_ASroute_symmetrical == A_surfxml_ASroute_symmetrical_YES )
	{
		if(route->dst_gateway && route->src_gateway)
		{
                  char *gw_src = xbt_strdup(route->src_gateway);
                  char *gw_dst = xbt_strdup(route->dst_gateway);
                  route->src_gateway = gw_dst;
                  route->dst_gateway = gw_src;
		}
		if(TO_ROUTE_FULL(*dst_id, *src_id))
		{
			char * link_name;
			unsigned int i;
			xbt_dynar_t link_route_to_test = xbt_dynar_new(global_routing->size_of_link, NULL);
			for(i=xbt_dynar_length(route->generic_route.link_list) ;i>0 ;i--)
			{
				link_name = xbt_dynar_get_as(route->generic_route.link_list,i-1,void *);
				void *link = xbt_dict_get_or_null(surf_network_model->resource_set, link_name);
				xbt_assert1(link,"Link : '%s' doesn't exists.",link_name);
				xbt_dynar_push(link_route_to_test,&link);
			}
			xbt_assert2(!xbt_dynar_compare(
				  (void*)TO_ROUTE_FULL(*dst_id, *src_id)->generic_route.link_list,
			      (void*)link_route_to_test,
				  (int_f_cpvoid_cpvoid_t) surf_pointer_resource_cmp),
				  "The route between \"%s\" and \"%s\" already exists", src,dst);
		}
		else
		{
			  if(!route->dst_gateway && !route->src_gateway)
				  XBT_DEBUG("Load Route from \"%s\" to \"%s\"", dst, src);
			  else
				  XBT_DEBUG("Load ASroute from \"%s(%s)\" to \"%s(%s)\"", dst,
				         route->src_gateway, src, route->dst_gateway);
		      TO_ROUTE_FULL(*dst_id, *src_id) = generic_new_extended_route(rc->hierarchy,route,0);
		      xbt_dynar_shrink(TO_ROUTE_FULL(*dst_id, *src_id)->generic_route.link_list, 0);
		}

	}
}

/* ************************************************************************** */
/* *************************** FLOYD ROUTING ******************************** */

#define TO_FLOYD_COST(i,j) (routing->cost_table)[(i)+(j)*table_size]
#define TO_FLOYD_PRED(i,j) (routing->predecessor_table)[(i)+(j)*table_size]
#define TO_FLOYD_LINK(i,j) (routing->link_table)[(i)+(j)*table_size]

/* Routing model structure */

typedef struct {
  s_routing_component_t generic_routing;
  /* vars for calculate the floyd algorith. */
  int *predecessor_table;
  double *cost_table;
  route_extended_t *link_table; /* char* -> int* */
} s_routing_component_floyd_t, *routing_component_floyd_t;

static route_extended_t floyd_get_route(routing_component_t rc,
                                        const char *src, const char *dst);

/* Business methods */
static xbt_dynar_t floyd_get_onelink_routes(routing_component_t rc)
{
  xbt_dynar_t ret = xbt_dynar_new(sizeof(onelink_t), xbt_free);

  routing_component_floyd_t routing = (routing_component_floyd_t) rc;
  //size_t table_size = xbt_dict_length(routing->generic_routing.to_index);
  xbt_dict_cursor_t c1 = NULL, c2 = NULL;
  char *k1, *d1, *k2, *d2;
  xbt_dict_foreach(routing->generic_routing.to_index, c1, k1, d1) {
    xbt_dict_foreach(routing->generic_routing.to_index, c2, k2, d2) {
      route_extended_t route = floyd_get_route(rc, k1, k2);
      if (route) {
        if (xbt_dynar_length(route->generic_route.link_list) == 1) {
          void *link =
              *(void **) xbt_dynar_get_ptr(route->generic_route.link_list,
                                           0);
          onelink_t onelink = xbt_new0(s_onelink_t, 1);
          onelink->link_ptr = link;
          if (routing->generic_routing.hierarchy == SURF_ROUTING_BASE) {
            onelink->src = xbt_strdup(k1);
            onelink->dst = xbt_strdup(k2);
          } else if (routing->generic_routing.hierarchy ==
                     SURF_ROUTING_RECURSIVE) {
            onelink->src = xbt_strdup(route->src_gateway);
            onelink->dst = xbt_strdup(route->dst_gateway);
          }
          xbt_dynar_push(ret, &onelink);
        }
      }
    }
  }
  return ret;
}

static route_extended_t floyd_get_route(routing_component_t rc,
                                        const char *src, const char *dst)
{
  xbt_assert1(rc && src
              && dst,
              "Invalid params for \"get_route\" function at AS \"%s\"",
              rc->name);

  /* set utils vars */
  routing_component_floyd_t routing = (routing_component_floyd_t) rc;
  size_t table_size = xbt_dict_length(routing->generic_routing.to_index);

  generic_src_dst_check(rc, src, dst);
  int *src_id = xbt_dict_get_or_null(routing->generic_routing.to_index, src);
  int *dst_id = xbt_dict_get_or_null(routing->generic_routing.to_index, dst);
  xbt_assert2(src_id
              && dst_id,
              "Ask for route \"from\"(%s)  or \"to\"(%s) no found in the local table",
              src, dst);

  /* create a result route */
  route_extended_t new_e_route = xbt_new0(s_route_extended_t, 1);
  new_e_route->generic_route.link_list =
      xbt_dynar_new(global_routing->size_of_link, NULL);
  new_e_route->src_gateway = NULL;
  new_e_route->dst_gateway = NULL;

  int first = 1;
  int pred = *dst_id;
  int prev_pred = 0;
  char *gw_src = NULL, *gw_dst =
      NULL, *prev_gw_src, *prev_gw_dst, *first_gw = NULL;
  unsigned int cpt;
  void *link;
  xbt_dynar_t links;

  do {
    prev_pred = pred;
    pred = TO_FLOYD_PRED(*src_id, pred);
    if (pred == -1)             /* if no pred in route -> no route to host */
      break;
    xbt_assert2(TO_FLOYD_LINK(pred, prev_pred),
                "Invalid link for the route between \"%s\" or \"%s\"", src,
                dst);

    prev_gw_src = gw_src;
    prev_gw_dst = gw_dst;

    route_extended_t e_route = TO_FLOYD_LINK(pred, prev_pred);
    gw_src = e_route->src_gateway;
    gw_dst = e_route->dst_gateway;

    if (first)
      first_gw = gw_dst;

    if (rc->hierarchy == SURF_ROUTING_RECURSIVE && !first
        && strcmp(gw_dst, prev_gw_src)) {
      xbt_dynar_t e_route_as_to_as =
          (*(global_routing->get_route)) (gw_dst, prev_gw_src);
      xbt_assert2(e_route_as_to_as, "no route between \"%s\" and \"%s\"",
                  gw_dst, prev_gw_src);
      links = e_route_as_to_as;
      int pos = 0;
      xbt_dynar_foreach(links, cpt, link) {
        xbt_dynar_insert_at(new_e_route->generic_route.link_list, pos,
                            &link);
        pos++;
      }
    }

    links = e_route->generic_route.link_list;
    xbt_dynar_foreach(links, cpt, link) {
      xbt_dynar_unshift(new_e_route->generic_route.link_list, &link);
    }
    first = 0;

  } while (pred != *src_id);
  xbt_assert4(pred != -1, "no route from host %d to %d (\"%s\" to \"%s\")",
              *src_id, *dst_id, src, dst);

  if (rc->hierarchy == SURF_ROUTING_RECURSIVE) {
    new_e_route->src_gateway = xbt_strdup(gw_src);
    new_e_route->dst_gateway = xbt_strdup(first_gw);
  }

  return new_e_route;
}

static void floyd_finalize(routing_component_t rc)
{
  routing_component_floyd_t routing = (routing_component_floyd_t) rc;
  int i, j;
  size_t table_size;
  if (routing) {
    table_size = xbt_dict_length(routing->generic_routing.to_index);
    /* Delete link_table */
    for (i = 0; i < table_size; i++)
      for (j = 0; j < table_size; j++)
        generic_free_extended_route(TO_FLOYD_LINK(i, j));
    xbt_free(routing->link_table);
    /* Delete bypass dict */
    xbt_dict_free(&routing->generic_routing.bypassRoutes);
    /* Delete index dict */
    xbt_dict_free(&(routing->generic_routing.to_index));
    /* Delete dictionary index dict, predecessor and links table */
    xbt_free(routing->predecessor_table);
    /* Delete structure */
    xbt_free(rc);
  }
}

static void *model_floyd_create(void)
{
  routing_component_floyd_t new_component =
      xbt_new0(s_routing_component_floyd_t, 1);
  new_component->generic_routing.set_processing_unit =
      generic_set_processing_unit;
  new_component->generic_routing.set_autonomous_system =
      generic_set_autonomous_system;
  new_component->generic_routing.set_route = model_floyd_set_route;
  new_component->generic_routing.set_ASroute = model_floyd_set_route;
  new_component->generic_routing.set_bypassroute = generic_set_bypassroute;
  new_component->generic_routing.get_route = floyd_get_route;
  new_component->generic_routing.get_latency = generic_get_link_latency;
  new_component->generic_routing.get_onelink_routes =
      floyd_get_onelink_routes;
  new_component->generic_routing.get_bypass_route =
      generic_get_bypassroute;
  new_component->generic_routing.finalize = floyd_finalize;
  new_component->generic_routing.to_index = xbt_dict_new();
  new_component->generic_routing.bypassRoutes = xbt_dict_new();
  new_component->generic_routing.get_network_element_type = get_network_element_type;
  return new_component;
}

static void model_floyd_load(void)
{
  /* use "surfxml_add_callback" to add a parse function call */
}

static void model_floyd_unload(void)
{
  /* use "surfxml_del_callback" to remove a parse function call */
}

static void model_floyd_end(void)
{

	routing_component_floyd_t routing =
	  ((routing_component_floyd_t) current_routing);

	unsigned int i, j, a, b, c;

	/* set the size of table routing */
	size_t table_size = xbt_dict_length(routing->generic_routing.to_index);

	if(!routing->link_table)
	{
		/* Create Cost, Predecessor and Link tables */
		routing->cost_table = xbt_new0(double, table_size * table_size);       /* link cost from host to host */
		routing->predecessor_table = xbt_new0(int, table_size * table_size);  /* predecessor host numbers */
		routing->link_table = xbt_new0(route_extended_t, table_size * table_size);    /* actual link between src and dst */

		/* Initialize costs and predecessors */
		for (i = 0; i < table_size; i++)
		for (j = 0; j < table_size; j++) {
		  TO_FLOYD_COST(i, j) = DBL_MAX;
		  TO_FLOYD_PRED(i, j) = -1;
		  TO_FLOYD_LINK(i, j) = NULL;       /* fixed, missing in the previous version */
		}
	}

	/* Add the loopback if needed */
	if (current_routing->hierarchy == SURF_ROUTING_BASE) {
		for (i = 0; i < table_size; i++) {
		  route_extended_t e_route = TO_FLOYD_LINK(i, i);
		  if (!e_route) {
			e_route = xbt_new0(s_route_extended_t, 1);
			e_route->src_gateway = NULL;
			e_route->dst_gateway = NULL;
			e_route->generic_route.link_list =
				xbt_dynar_new(global_routing->size_of_link, NULL);
			xbt_dynar_push(e_route->generic_route.link_list,
						   &global_routing->loopback);
			TO_FLOYD_LINK(i, i) = e_route;
			TO_FLOYD_PRED(i, i) = i;
			TO_FLOYD_COST(i, i) = 1;
		  }
		}
	}
	/* Calculate path costs */
	for (c = 0; c < table_size; c++) {
		for (a = 0; a < table_size; a++) {
		  for (b = 0; b < table_size; b++) {
			if (TO_FLOYD_COST(a, c) < DBL_MAX && TO_FLOYD_COST(c, b) < DBL_MAX) {
			  if (TO_FLOYD_COST(a, b) == DBL_MAX ||
				  (TO_FLOYD_COST(a, c) + TO_FLOYD_COST(c, b) <
				   TO_FLOYD_COST(a, b))) {
				TO_FLOYD_COST(a, b) =
					TO_FLOYD_COST(a, c) + TO_FLOYD_COST(c, b);
				TO_FLOYD_PRED(a, b) = TO_FLOYD_PRED(c, b);
			  }
			}
		  }
		}
	}
}

static void model_floyd_set_route(routing_component_t rc, const char *src,
        const char *dst, name_route_extended_t route)
{
	routing_component_floyd_t routing = (routing_component_floyd_t) rc;

	/* set the size of table routing */
	size_t table_size = xbt_dict_length(rc->to_index);
	int *src_id, *dst_id;
	int i,j;

	src_id = xbt_dict_get_or_null(rc->to_index, src);
	dst_id = xbt_dict_get_or_null(rc->to_index, dst);

	if(!routing->link_table)
	{
		/* Create Cost, Predecessor and Link tables */
		routing->cost_table = xbt_new0(double, table_size * table_size);       /* link cost from host to host */
		routing->predecessor_table = xbt_new0(int, table_size * table_size);  /* predecessor host numbers */
		routing->link_table = xbt_new0(route_extended_t, table_size * table_size);    /* actual link between src and dst */

		/* Initialize costs and predecessors */
		for (i = 0; i < table_size; i++)
		for (j = 0; j < table_size; j++) {
		  TO_FLOYD_COST(i, j) = DBL_MAX;
		  TO_FLOYD_PRED(i, j) = -1;
		  TO_FLOYD_LINK(i, j) = NULL;       /* fixed, missing in the previous version */
		}
	}

	if(TO_FLOYD_LINK(*src_id, *dst_id))
	{
		if(!route->dst_gateway && !route->src_gateway)
			XBT_DEBUG("See Route from \"%s\" to \"%s\"", src, dst);
		else
			XBT_DEBUG("See ASroute from \"%s(%s)\" to \"%s(%s)\"", src,
				 route->src_gateway, dst, route->dst_gateway);
		char * link_name;
		unsigned int cpt;
		xbt_dynar_t link_route_to_test = xbt_dynar_new(global_routing->size_of_link, NULL);
		xbt_dynar_foreach(route->generic_route.link_list,cpt,link_name)
		{
			void *link = xbt_dict_get_or_null(surf_network_model->resource_set, link_name);
			xbt_assert1(link,"Link : '%s' doesn't exists.",link_name);
			xbt_dynar_push(link_route_to_test,&link);
		}
		xbt_assert2(!xbt_dynar_compare(
			  (void*)TO_FLOYD_LINK(*src_id, *dst_id)->generic_route.link_list,
			  (void*)link_route_to_test,
			  (int_f_cpvoid_cpvoid_t) surf_pointer_resource_cmp),
			  "The route between \"%s\" and \"%s\" already exists", src,dst);
	}
	else
	{
		if(!route->dst_gateway && !route->src_gateway)
		  XBT_DEBUG("Load Route from \"%s\" to \"%s\"", src, dst);
		else{
			XBT_DEBUG("Load ASroute from \"%s(%s)\" to \"%s(%s)\"", src,
				 route->src_gateway, dst, route->dst_gateway);
			if(global_routing->get_network_element_type((const char*)route->dst_gateway) == SURF_NETWORK_ELEMENT_NULL)
				xbt_die("The dst_gateway '%s' does not exist!",route->dst_gateway);
			if(global_routing->get_network_element_type((const char*)route->src_gateway) == SURF_NETWORK_ELEMENT_NULL)
				xbt_die("The src_gateway '%s' does not exist!",route->src_gateway);
		}
	    TO_FLOYD_LINK(*src_id, *dst_id) =
	    		generic_new_extended_route(rc->hierarchy, route, 1);
	    TO_FLOYD_PRED(*src_id, *dst_id) = *src_id;
	    TO_FLOYD_COST(*src_id, *dst_id) =
	    		((TO_FLOYD_LINK(*src_id, *dst_id))->generic_route.link_list)->used;   /* count of links, old model assume 1 */
	}

	if( A_surfxml_route_symmetrical == A_surfxml_route_symmetrical_YES
		|| A_surfxml_ASroute_symmetrical == A_surfxml_ASroute_symmetrical_YES )
	{
		if(TO_FLOYD_LINK(*dst_id, *src_id))
		{
			if(!route->dst_gateway && !route->src_gateway)
			  XBT_DEBUG("See Route from \"%s\" to \"%s\"", dst, src);
			else
			  XBT_DEBUG("See ASroute from \"%s(%s)\" to \"%s(%s)\"", dst,
					 route->src_gateway, src, route->dst_gateway);
			char * link_name;
			unsigned int i;
			xbt_dynar_t link_route_to_test = xbt_dynar_new(global_routing->size_of_link, NULL);
			for(i=xbt_dynar_length(route->generic_route.link_list) ;i>0 ;i--)
			{
				link_name = xbt_dynar_get_as(route->generic_route.link_list,i-1,void *);
				void *link = xbt_dict_get_or_null(surf_network_model->resource_set, link_name);
				xbt_assert1(link,"Link : '%s' doesn't exists.",link_name);
				xbt_dynar_push(link_route_to_test,&link);
			}
			xbt_assert2(!xbt_dynar_compare(
				  (void*)TO_FLOYD_LINK(*dst_id, *src_id)->generic_route.link_list,
			      (void*)link_route_to_test,
				  (int_f_cpvoid_cpvoid_t) surf_pointer_resource_cmp),
				  "The route between \"%s\" and \"%s\" already exists", src,dst);
		}
		else
		{
			if(route->dst_gateway && route->src_gateway)
			{
                          char *gw_src = xbt_strdup(route->src_gateway);
                          char *gw_dst = xbt_strdup(route->dst_gateway);
                          route->src_gateway = gw_dst;
                          route->dst_gateway = gw_src;
			}

			if(!route->dst_gateway && !route->src_gateway)
			  XBT_DEBUG("Load Route from \"%s\" to \"%s\"", dst, src);
			else
			  XBT_DEBUG("Load ASroute from \"%s(%s)\" to \"%s(%s)\"", dst,
					 route->src_gateway, src, route->dst_gateway);

		    TO_FLOYD_LINK(*dst_id, *src_id) =
		    		generic_new_extended_route(rc->hierarchy, route, 0);
		    TO_FLOYD_PRED(*dst_id, *src_id) = *dst_id;
		    TO_FLOYD_COST(*dst_id, *src_id) =
		    		((TO_FLOYD_LINK(*dst_id, *src_id))->generic_route.link_list)->used;   /* count of links, old model assume 1 */
		}
	}
}

/* ************************************************************************** */
/* ********** Dijkstra & Dijkstra Cached ROUTING **************************** */

typedef struct {
  s_routing_component_t generic_routing;
  xbt_graph_t route_graph;      /* xbt_graph */
  xbt_dict_t graph_node_map;    /* map */
  xbt_dict_t route_cache;       /* use in cache mode */
  int cached;
} s_routing_component_dijkstra_t, *routing_component_dijkstra_t;


typedef struct graph_node_data {
  int id;
  int graph_id;                 /* used for caching internal graph id's */
} s_graph_node_data_t, *graph_node_data_t;

typedef struct graph_node_map_element {
  xbt_node_t node;
} s_graph_node_map_element_t, *graph_node_map_element_t;

typedef struct route_cache_element {
  int *pred_arr;
  int size;
} s_route_cache_element_t, *route_cache_element_t;

/* Free functions */

static void route_cache_elem_free(void *e)
{
  route_cache_element_t elm = (route_cache_element_t) e;
  if (elm) {
    xbt_free(elm->pred_arr);
    xbt_free(elm);
  }
}

static void graph_node_map_elem_free(void *e)
{
  graph_node_map_element_t elm = (graph_node_map_element_t) e;
  if (elm) {
    xbt_free(elm);
  }
}

static void graph_edge_data_free(void *e)
{
  route_extended_t e_route = (route_extended_t) e;
  if (e_route) {
    xbt_dynar_free(&(e_route->generic_route.link_list));
    if (e_route->src_gateway)
      xbt_free(e_route->src_gateway);
    if (e_route->dst_gateway)
      xbt_free(e_route->dst_gateway);
    xbt_free(e_route);
  }
}

/* Utility functions */

static xbt_node_t route_graph_new_node(routing_component_dijkstra_t rc,
                                       int id, int graph_id)
{
  routing_component_dijkstra_t routing = (routing_component_dijkstra_t) rc;
  xbt_node_t node = NULL;
  graph_node_data_t data = NULL;
  graph_node_map_element_t elm = NULL;

  data = xbt_new0(struct graph_node_data, 1);
  data->id = id;
  data->graph_id = graph_id;
  node = xbt_graph_new_node(routing->route_graph, data);

  elm = xbt_new0(struct graph_node_map_element, 1);
  elm->node = node;
  xbt_dict_set_ext(routing->graph_node_map, (char *) (&id), sizeof(int),
                   (xbt_set_elm_t) elm, &graph_node_map_elem_free);

  return node;
}

static graph_node_map_element_t
graph_node_map_search(routing_component_dijkstra_t rc, int id)
{
  routing_component_dijkstra_t routing = (routing_component_dijkstra_t) rc;
  graph_node_map_element_t elm = (graph_node_map_element_t)
      xbt_dict_get_or_null_ext(routing->graph_node_map,
                               (char *) (&id),
                               sizeof(int));
  return elm;
}

/* Parsing */

static void route_new_dijkstra(routing_component_dijkstra_t rc, int src_id,
                               int dst_id, route_extended_t e_route)
{
  routing_component_dijkstra_t routing = (routing_component_dijkstra_t) rc;
  XBT_DEBUG("Load Route from \"%d\" to \"%d\"", src_id, dst_id);
  xbt_node_t src = NULL;
  xbt_node_t dst = NULL;

  graph_node_map_element_t src_elm = (graph_node_map_element_t)
      xbt_dict_get_or_null_ext(routing->graph_node_map,
                               (char *) (&src_id),
                               sizeof(int));
  graph_node_map_element_t dst_elm = (graph_node_map_element_t)
      xbt_dict_get_or_null_ext(routing->graph_node_map,
                               (char *) (&dst_id),
                               sizeof(int));


  if (src_elm)
    src = src_elm->node;

  if (dst_elm)
    dst = dst_elm->node;

  /* add nodes if they don't exist in the graph */
  if (src_id == dst_id && src == NULL && dst == NULL) {
    src = route_graph_new_node(rc, src_id, -1);
    dst = src;
  } else {
    if (src == NULL) {
      src = route_graph_new_node(rc, src_id, -1);
    }
    if (dst == NULL) {
      dst = route_graph_new_node(rc, dst_id, -1);
    }
  }

  /* add link as edge to graph */
  xbt_graph_new_edge(routing->route_graph, src, dst, e_route);
}

static void add_loopback_dijkstra(routing_component_dijkstra_t rc)
{
  routing_component_dijkstra_t routing = (routing_component_dijkstra_t) rc;

  xbt_dynar_t nodes = xbt_graph_get_nodes(routing->route_graph);

  xbt_node_t node = NULL;
  unsigned int cursor2;
  xbt_dynar_foreach(nodes, cursor2, node) {
    xbt_dynar_t out_edges = xbt_graph_node_get_outedges(node);
    xbt_edge_t edge = NULL;
    unsigned int cursor;

    int found = 0;
    xbt_dynar_foreach(out_edges, cursor, edge) {
      xbt_node_t other_node = xbt_graph_edge_get_target(edge);
      if (other_node == node) {
        found = 1;
        break;
      }
    }

    if (!found) {
      route_extended_t e_route = xbt_new0(s_route_extended_t, 1);
      e_route->src_gateway = NULL;
      e_route->dst_gateway = NULL;
      e_route->generic_route.link_list =
          xbt_dynar_new(global_routing->size_of_link, NULL);
      xbt_dynar_push(e_route->generic_route.link_list,
                     &global_routing->loopback);
      xbt_graph_new_edge(routing->route_graph, node, node, e_route);
    }
  }
}

/* Business methods */
static xbt_dynar_t dijkstra_get_onelink_routes(routing_component_t rc)
{
  xbt_die("\"dijkstra_get_onelink_routes\" function not implemented yet");
}

static route_extended_t dijkstra_get_route(routing_component_t rc,
                                           const char *src,
                                           const char *dst)
{
  xbt_assert1(rc && src
              && dst,
              "Invalid params for \"get_route\" function at AS \"%s\"",
              rc->name);

  /* set utils vars */
  routing_component_dijkstra_t routing = (routing_component_dijkstra_t) rc;

  generic_src_dst_check(rc, src, dst);
  int *src_id = xbt_dict_get_or_null(routing->generic_routing.to_index, src);
  int *dst_id = xbt_dict_get_or_null(routing->generic_routing.to_index, dst);
  xbt_assert2(src_id
              && dst_id,
              "Ask for route \"from\"(%s)  or \"to\"(%s) no found in the local table",
              src, dst);

  /* create a result route */
  route_extended_t new_e_route = xbt_new0(s_route_extended_t, 1);
  new_e_route->generic_route.link_list =
      xbt_dynar_new(global_routing->size_of_link, NULL);
  new_e_route->src_gateway = NULL;
  new_e_route->dst_gateway = NULL;

  int *pred_arr = NULL;
  int src_node_id = 0;
  int dst_node_id = 0;
  int *nodeid = NULL;
  int v;
  route_extended_t e_route;
  int size = 0;
  unsigned int cpt;
  void *link;
  xbt_dynar_t links = NULL;
  route_cache_element_t elm = NULL;
  xbt_dynar_t nodes = xbt_graph_get_nodes(routing->route_graph);

  /* Use the graph_node id mapping set to quickly find the nodes */
  graph_node_map_element_t src_elm =
      graph_node_map_search(routing, *src_id);
  graph_node_map_element_t dst_elm =
      graph_node_map_search(routing, *dst_id);
  xbt_assert2(src_elm != NULL
              && dst_elm != NULL, "src %d or dst %d does not exist",
              *src_id, *dst_id);
  src_node_id = ((graph_node_data_t)
                 xbt_graph_node_get_data(src_elm->node))->graph_id;
  dst_node_id = ((graph_node_data_t)
                 xbt_graph_node_get_data(dst_elm->node))->graph_id;

  /* if the src and dst are the same *//* fixed, missing in the previous version */
  if (src_node_id == dst_node_id) {

    xbt_node_t node_s_v = xbt_dynar_get_as(nodes, src_node_id, xbt_node_t);
    xbt_node_t node_e_v = xbt_dynar_get_as(nodes, dst_node_id, xbt_node_t);
    xbt_edge_t edge =
        xbt_graph_get_edge(routing->route_graph, node_s_v, node_e_v);

    xbt_assert2(edge != NULL, "no route between host %d and %d", *src_id,
                *dst_id);

    e_route = (route_extended_t) xbt_graph_edge_get_data(edge);

    links = e_route->generic_route.link_list;
    xbt_dynar_foreach(links, cpt, link) {
      xbt_dynar_unshift(new_e_route->generic_route.link_list, &link);
    }

    return new_e_route;
  }

  if (routing->cached) {
    /*check if there is a cached predecessor list avail */
    elm = (route_cache_element_t)
        xbt_dict_get_or_null_ext(routing->route_cache, (char *) (&src_id),
                                 sizeof(int));
  }

  if (elm) {                    /* cached mode and cache hit */
    pred_arr = elm->pred_arr;
  } else {                      /* not cached mode or cache miss */
    double *cost_arr = NULL;
    xbt_heap_t pqueue = NULL;
    int i = 0;

    int nr_nodes = xbt_dynar_length(nodes);
    cost_arr = xbt_new0(double, nr_nodes);      /* link cost from src to other hosts */
    pred_arr = xbt_new0(int, nr_nodes); /* predecessors in path from src */
    pqueue = xbt_heap_new(nr_nodes, xbt_free);

    /* initialize */
    cost_arr[src_node_id] = 0.0;

    for (i = 0; i < nr_nodes; i++) {
      if (i != src_node_id) {
        cost_arr[i] = DBL_MAX;
      }

      pred_arr[i] = 0;

      /* initialize priority queue */
      nodeid = xbt_new0(int, 1);
      *nodeid = i;
      xbt_heap_push(pqueue, nodeid, cost_arr[i]);

    }

    /* apply dijkstra using the indexes from the graph's node array */
    while (xbt_heap_size(pqueue) > 0) {
      int *v_id = xbt_heap_pop(pqueue);
      xbt_node_t v_node = xbt_dynar_get_as(nodes, *v_id, xbt_node_t);
      xbt_dynar_t out_edges = xbt_graph_node_get_outedges(v_node);
      xbt_edge_t edge = NULL;
      unsigned int cursor;

      xbt_dynar_foreach(out_edges, cursor, edge) {
        xbt_node_t u_node = xbt_graph_edge_get_target(edge);
        graph_node_data_t data = xbt_graph_node_get_data(u_node);
        int u_id = data->graph_id;
        route_extended_t tmp_e_route =
            (route_extended_t) xbt_graph_edge_get_data(edge);
        int cost_v_u = (tmp_e_route->generic_route.link_list)->used;    /* count of links, old model assume 1 */

        if (cost_v_u + cost_arr[*v_id] < cost_arr[u_id]) {
          pred_arr[u_id] = *v_id;
          cost_arr[u_id] = cost_v_u + cost_arr[*v_id];
          nodeid = xbt_new0(int, 1);
          *nodeid = u_id;
          xbt_heap_push(pqueue, nodeid, cost_arr[u_id]);
        }
      }

      /* free item popped from pqueue */
      xbt_free(v_id);
    }

    xbt_free(cost_arr);
    xbt_heap_free(pqueue);
  }

  /* compose route path with links */
  char *gw_src = NULL, *gw_dst =
      NULL, *prev_gw_src, *prev_gw_dst, *first_gw = NULL;

  for (v = dst_node_id; v != src_node_id; v = pred_arr[v]) {
    xbt_node_t node_pred_v =
        xbt_dynar_get_as(nodes, pred_arr[v], xbt_node_t);
    xbt_node_t node_v = xbt_dynar_get_as(nodes, v, xbt_node_t);
    xbt_edge_t edge =
        xbt_graph_get_edge(routing->route_graph, node_pred_v, node_v);

    xbt_assert2(edge != NULL, "no route between host %d and %d", *src_id,
                *dst_id);

    prev_gw_src = gw_src;
    prev_gw_dst = gw_dst;

    e_route = (route_extended_t) xbt_graph_edge_get_data(edge);
    gw_src = e_route->src_gateway;
    gw_dst = e_route->dst_gateway;

    if (v == dst_node_id)
      first_gw = gw_dst;

    if (rc->hierarchy == SURF_ROUTING_RECURSIVE && v != dst_node_id
        && strcmp(gw_dst, prev_gw_src)) {
      xbt_dynar_t e_route_as_to_as =
          (*(global_routing->get_route)) (gw_dst, prev_gw_src);
      xbt_assert2(e_route_as_to_as, "no route between \"%s\" and \"%s\"",
                  gw_dst, prev_gw_src);
      links = e_route_as_to_as;
      int pos = 0;
      xbt_dynar_foreach(links, cpt, link) {
        xbt_dynar_insert_at(new_e_route->generic_route.link_list, pos,
                            &link);
        pos++;
      }
    }

    links = e_route->generic_route.link_list;
    xbt_dynar_foreach(links, cpt, link) {
      xbt_dynar_unshift(new_e_route->generic_route.link_list, &link);
    }
    size++;
  }

  if (rc->hierarchy == SURF_ROUTING_RECURSIVE) {
    new_e_route->src_gateway = xbt_strdup(gw_src);
    new_e_route->dst_gateway = xbt_strdup(first_gw);
  }

  if (routing->cached && elm == NULL) {
    /* add to predecessor list of the current src-host to cache */
    elm = xbt_new0(struct route_cache_element, 1);
    elm->pred_arr = pred_arr;
    elm->size = size;
    xbt_dict_set_ext(routing->route_cache, (char *) (&src_id), sizeof(int),
                     (xbt_set_elm_t) elm, &route_cache_elem_free);
  }

  if (!routing->cached)
    xbt_free(pred_arr);

  return new_e_route;
}

static void dijkstra_finalize(routing_component_t rc)
{
  routing_component_dijkstra_t routing = (routing_component_dijkstra_t) rc;

  if (routing) {
    xbt_graph_free_graph(routing->route_graph, &xbt_free,
                         &graph_edge_data_free, &xbt_free);
    xbt_dict_free(&routing->graph_node_map);
    if (routing->cached)
      xbt_dict_free(&routing->route_cache);
    /* Delete bypass dict */
    xbt_dict_free(&routing->generic_routing.bypassRoutes);
    /* Delete index dict */
    xbt_dict_free(&(routing->generic_routing.to_index));
    /* Delete structure */
    xbt_free(routing);
  }
}

/* Creation routing model functions */

static void *model_dijkstra_both_create(int cached)
{
  routing_component_dijkstra_t new_component =
      xbt_new0(s_routing_component_dijkstra_t, 1);
  new_component->generic_routing.set_processing_unit =
      generic_set_processing_unit;
  new_component->generic_routing.set_autonomous_system =
      generic_set_autonomous_system;
  new_component->generic_routing.set_route = model_dijkstra_both_set_route;
  new_component->generic_routing.set_ASroute = model_dijkstra_both_set_route;
  new_component->generic_routing.set_bypassroute = generic_set_bypassroute;
  new_component->generic_routing.get_route = dijkstra_get_route;
  new_component->generic_routing.get_latency = generic_get_link_latency;
  new_component->generic_routing.get_onelink_routes =
      dijkstra_get_onelink_routes;
  new_component->generic_routing.get_bypass_route =
      generic_get_bypassroute;
  new_component->generic_routing.finalize = dijkstra_finalize;
  new_component->cached = cached;
  new_component->generic_routing.to_index = xbt_dict_new();
  new_component->generic_routing.bypassRoutes = xbt_dict_new();
  new_component->generic_routing.get_network_element_type = get_network_element_type;
  return new_component;
}

static void *model_dijkstra_create(void)
{
  return model_dijkstra_both_create(0);
}

static void *model_dijkstracache_create(void)
{
  return model_dijkstra_both_create(1);
}

static void model_dijkstra_both_load(void)
{
  /* use "surfxml_add_callback" to add a parse function call */
}

static void model_dijkstra_both_unload(void)
{
  /* use "surfxml_del_callback" to remove a parse function call */
}

static void model_dijkstra_both_end(void)
{
  routing_component_dijkstra_t routing =
      (routing_component_dijkstra_t) current_routing;

  xbt_node_t node = NULL;
  unsigned int cursor2;
  xbt_dynar_t nodes = NULL;

  /* Create the topology graph */
  if(!routing->route_graph)
  routing->route_graph = xbt_graph_new_graph(1, NULL);
  if(!routing->graph_node_map)
  routing->graph_node_map = xbt_dict_new();

  if (routing->cached && !routing->route_cache)
  routing->route_cache = xbt_dict_new();

  /* Add the loopback if needed */
  if (current_routing->hierarchy == SURF_ROUTING_BASE)
    add_loopback_dijkstra(routing);

  /* initialize graph indexes in nodes after graph has been built */
  nodes = xbt_graph_get_nodes(routing->route_graph);

  xbt_dynar_foreach(nodes, cursor2, node) {
    graph_node_data_t data = xbt_graph_node_get_data(node);
    data->graph_id = cursor2;
  }

}
static void model_dijkstra_both_set_route (routing_component_t rc, const char *src,
                     const char *dst, name_route_extended_t route)
{
	routing_component_dijkstra_t routing = (routing_component_dijkstra_t) rc;
	int *src_id, *dst_id;
	src_id = xbt_dict_get_or_null(rc->to_index, src);
	dst_id = xbt_dict_get_or_null(rc->to_index, dst);

    /* Create the topology graph */
	if(!routing->route_graph)
	routing->route_graph = xbt_graph_new_graph(1, NULL);
	if(!routing->graph_node_map)
	routing->graph_node_map = xbt_dict_new();

	if (routing->cached && !routing->route_cache)
	routing->route_cache = xbt_dict_new();

	if( A_surfxml_route_symmetrical == A_surfxml_route_symmetrical_YES
		|| A_surfxml_ASroute_symmetrical == A_surfxml_ASroute_symmetrical_YES )
		xbt_die("Route symmetrical not supported on model dijkstra");

	if(!route->dst_gateway && !route->src_gateway)
	  XBT_DEBUG("Load Route from \"%s\" to \"%s\"", src, dst);
	else{
	  XBT_DEBUG("Load ASroute from \"%s(%s)\" to \"%s(%s)\"", src,
			 route->src_gateway, dst, route->dst_gateway);
	  if(global_routing->get_network_element_type((const char*)route->dst_gateway) == SURF_NETWORK_ELEMENT_NULL)
		  xbt_die("The dst_gateway '%s' does not exist!",route->dst_gateway);
	  if(global_routing->get_network_element_type((const char*)route->src_gateway) == SURF_NETWORK_ELEMENT_NULL)
		  xbt_die("The src_gateway '%s' does not exist!",route->src_gateway);
	}

	route_extended_t e_route =
		generic_new_extended_route(current_routing->hierarchy, route, 1);
	route_new_dijkstra(routing, *src_id, *dst_id, e_route);
}

#ifdef HAVE_PCRE_LIB
/* ************************************************** */
/* ************** RULE-BASED ROUTING **************** */

/* Routing model structure */

typedef struct {
  s_routing_component_t generic_routing;
  xbt_dict_t dict_processing_units;
  xbt_dict_t dict_autonomous_systems;
  xbt_dynar_t list_route;
  xbt_dynar_t list_ASroute;
} s_routing_component_rulebased_t, *routing_component_rulebased_t;

typedef struct s_rule_route s_rule_route_t, *rule_route_t;
typedef struct s_rule_route_extended s_rule_route_extended_t,
    *rule_route_extended_t;

struct s_rule_route {
  xbt_dynar_t re_str_link;      // dynar of char*
  pcre *re_src;
  pcre *re_dst;
};

struct s_rule_route_extended {
  s_rule_route_t generic_rule_route;
  char *re_src_gateway;
  char *re_dst_gateway;
};

static void rule_route_free(void *e)
{
  rule_route_t *elem = (rule_route_t *) (e);
  if (*elem) {
    xbt_dynar_free(&(*elem)->re_str_link);
    pcre_free((*elem)->re_src);
    pcre_free((*elem)->re_dst);
    xbt_free(*elem);
  }
  *elem = NULL;
}

static void rule_route_extended_free(void *e)
{
  rule_route_extended_t *elem = (rule_route_extended_t *) e;
  if (*elem) {
    xbt_dynar_free(&(*elem)->generic_rule_route.re_str_link);
    pcre_free((*elem)->generic_rule_route.re_src);
    pcre_free((*elem)->generic_rule_route.re_dst);
    xbt_free((*elem)->re_src_gateway);
    xbt_free((*elem)->re_dst_gateway);
    xbt_free(*elem);
  }
}

/* Parse routing model functions */

static void model_rulebased_set_processing_unit(routing_component_t rc,
                                                const char *name)
{
  routing_component_rulebased_t routing =
      (routing_component_rulebased_t) rc;
  xbt_dict_set(routing->dict_processing_units, name, (void *) (-1), NULL);
}

static void model_rulebased_set_autonomous_system(routing_component_t rc,
                                                  const char *name)
{
  routing_component_rulebased_t routing =
      (routing_component_rulebased_t) rc;
  xbt_dict_set(routing->dict_autonomous_systems, name, (void *) (-1),
               NULL);
}

static void model_rulebased_set_route(routing_component_t rc,
                                      const char *src, const char *dst,
                                      name_route_extended_t route)
{
  routing_component_rulebased_t routing =
      (routing_component_rulebased_t) rc;
  rule_route_t ruleroute = xbt_new0(s_rule_route_t, 1);
  const char *error;
  int erroffset;
  ruleroute->re_src = pcre_compile(src, 0, &error, &erroffset, NULL);
  xbt_assert3(ruleroute->re_src,
              "PCRE compilation failed at offset %d (\"%s\"): %s\n",
              erroffset, src, error);
  ruleroute->re_dst = pcre_compile(dst, 0, &error, &erroffset, NULL);
  xbt_assert3(ruleroute->re_src,
              "PCRE compilation failed at offset %d (\"%s\"): %s\n",
              erroffset, dst, error);
  ruleroute->re_str_link = route->generic_route.link_list;
  xbt_dynar_push(routing->list_route, &ruleroute);
  xbt_free(route);
}

static void model_rulebased_set_ASroute(routing_component_t rc,
                                        const char *src, const char *dst,
                                        name_route_extended_t route)
{
  routing_component_rulebased_t routing =
      (routing_component_rulebased_t) rc;
  rule_route_extended_t ruleroute_e = xbt_new0(s_rule_route_extended_t, 1);
  const char *error;
  int erroffset;
  ruleroute_e->generic_rule_route.re_src =
      pcre_compile(src, 0, &error, &erroffset, NULL);
  xbt_assert3(ruleroute_e->generic_rule_route.re_src,
              "PCRE compilation failed at offset %d (\"%s\"): %s\n",
              erroffset, src, error);
  ruleroute_e->generic_rule_route.re_dst =
      pcre_compile(dst, 0, &error, &erroffset, NULL);
  xbt_assert3(ruleroute_e->generic_rule_route.re_src,
              "PCRE compilation failed at offset %d (\"%s\"): %s\n",
              erroffset, dst, error);
  ruleroute_e->generic_rule_route.re_str_link =
      route->generic_route.link_list;
  ruleroute_e->re_src_gateway = route->src_gateway;
  ruleroute_e->re_dst_gateway = route->dst_gateway;
  xbt_dynar_push(routing->list_ASroute, &ruleroute_e);
//  xbt_free(route->src_gateway);
//  xbt_free(route->dst_gateway);
  xbt_free(route);
}

static void model_rulebased_set_bypassroute(routing_component_t rc,
                                            const char *src,
                                            const char *dst,
                                            route_extended_t e_route)
{
  xbt_die("bypass routing not supported for Route-Based model");
}

#define BUFFER_SIZE 4096        /* result buffer size */
#define OVECCOUNT 30            /* should be a multiple of 3 */

static char *remplace(char *value, const char **src_list, int src_size,
                      const char **dst_list, int dst_size)
{

  char result_result[BUFFER_SIZE];
  int i_result_buffer;
  int value_length = (int) strlen(value);
  int number = 0;

  int i = 0;
  i_result_buffer = 0;
  do {
    if (value[i] == '$') {
      i++;                      // skip $

      // find the number      
      int number_length = 0;
      while ('0' <= value[i + number_length]
             && value[i + number_length] <= '9') {
        number_length++;
      }
      xbt_assert2(number_length != 0,
                  "bad string parameter, no number indication, at offset: %d (\"%s\")",
                  i, value);

      // solve number
      number = atoi(value + i);
      i = i + number_length;
      xbt_assert2(i + 2 < value_length,
                  "bad string parameter, too few chars, at offset: %d (\"%s\")",
                  i, value);

      // solve the indication
      const char **param_list;
      int param_size;
      if (value[i] == 's' && value[i + 1] == 'r' && value[i + 2] == 'c') {
        param_list = src_list;
        param_size = src_size;
      } else if (value[i] == 'd' && value[i + 1] == 's'
                 && value[i + 2] == 't') {
        param_list = dst_list;
        param_size = dst_size;
      } else {
        xbt_die("bad string parameter, support only \"src\" and \"dst\", "
                "at offset: %d (\"%s\")", i, value);
      }
      i = i + 3;

      xbt_assert4(param_size >= number,
                  "bad string parameter, not enough length param_size, at offset: %d (\"%s\") %d %d",
                  i, value, param_size, number);

      const char *param = param_list[number];
      int size = strlen(param);
      int cp;
      for (cp = 0; cp < size; cp++) {
        result_result[i_result_buffer] = param[cp];
        i_result_buffer++;
        if (i_result_buffer >= BUFFER_SIZE)
          break;
      }
    } else {
      result_result[i_result_buffer] = value[i];
      i_result_buffer++;
      i++;                      // next char
    }

  } while (i < value_length && i_result_buffer < BUFFER_SIZE);

  xbt_assert2(i_result_buffer < BUFFER_SIZE,
              "solving string \"%s\", small buffer size (%d)", value,
              BUFFER_SIZE);
  result_result[i_result_buffer] = 0;
  return xbt_strdup(result_result);
}

static route_extended_t rulebased_get_route(routing_component_t rc,
                                            const char *src,
                                            const char *dst);
static xbt_dynar_t rulebased_get_onelink_routes(routing_component_t rc)
{
  xbt_dynar_t ret = xbt_dynar_new (sizeof(onelink_t), xbt_free);
  routing_component_rulebased_t routing = (routing_component_rulebased_t)rc;

  xbt_dict_cursor_t c1 = NULL;
  char *k1, *d1;

  //find router
  char *router = NULL;
  xbt_dict_foreach(routing->dict_processing_units, c1, k1, d1) {
    if (strstr (k1, "router")){
      router = k1;
    }
  }
  if (!router){
    xbt_die ("rulebased_get_onelink_routes works only if the AS is a cluster, sorry.");
  }

  xbt_dict_foreach(routing->dict_processing_units, c1, k1, d1) {
    route_extended_t route = rulebased_get_route (rc, router, k1);

    int number_of_links = xbt_dynar_length(route->generic_route.link_list);
    if (number_of_links != 3) {
      xbt_die ("rulebased_get_onelink_routes works only if the AS is a cluster, sorry.");
    }

    void *link_ptr;
    xbt_dynar_get_cpy (route->generic_route.link_list, 2, &link_ptr);
    onelink_t onelink = xbt_new0 (s_onelink_t, 1);
    onelink->src = xbt_strdup (k1);
    onelink->dst = xbt_strdup (router);
    onelink->link_ptr = link_ptr;
    xbt_dynar_push (ret, &onelink);
  }
  return ret;
}

/* Business methods */
static route_extended_t rulebased_get_route(routing_component_t rc,
                                            const char *src,
                                            const char *dst)
{
  xbt_assert1(rc && src
              && dst,
              "Invalid params for \"get_route\" function at AS \"%s\"",
              rc->name);

  /* set utils vars */
  routing_component_rulebased_t routing =
      (routing_component_rulebased_t) rc;

  int are_processing_units=0;
  xbt_dynar_t rule_list;
  if (xbt_dict_get_or_null(routing->dict_processing_units, src)
      && xbt_dict_get_or_null(routing->dict_processing_units, dst)) {
    are_processing_units = 1;
    rule_list = routing->list_route;
  } else if (xbt_dict_get_or_null(routing->dict_autonomous_systems, src)
             && xbt_dict_get_or_null(routing->dict_autonomous_systems,
                                     dst)) {
    are_processing_units = 0;
    rule_list = routing->list_ASroute;
  } else
    xbt_die("Ask for route \"from\"(%s)  or \"to\"(%s) no found in "
            "the local table", src, dst);

  int rc_src = -1;
  int rc_dst = -1;
  int src_length = (int) strlen(src);
  int dst_length = (int) strlen(dst);

  xbt_dynar_t links_list =
      xbt_dynar_new(global_routing->size_of_link, NULL);

  rule_route_t ruleroute;
  unsigned int cpt;
  int ovector_src[OVECCOUNT];
  int ovector_dst[OVECCOUNT];
  const char **list_src = NULL;
  const char **list_dst = NULL;
  int res;
  xbt_dynar_foreach(rule_list, cpt, ruleroute) {
    rc_src =
        pcre_exec(ruleroute->re_src, NULL, src, src_length, 0, 0,
                  ovector_src, OVECCOUNT);
    if (rc_src >= 0) {
      rc_dst =
          pcre_exec(ruleroute->re_dst, NULL, dst, dst_length, 0, 0,
                    ovector_dst, OVECCOUNT);
      if (rc_dst >= 0) {
        res = pcre_get_substring_list(src, ovector_src, rc_src, &list_src);
        xbt_assert1(!res, "error solving substring list for src \"%s\"", src);
        res = pcre_get_substring_list(dst, ovector_dst, rc_dst, &list_dst);
        xbt_assert1(!res, "error solving substring list for src \"%s\"", dst);
        char *link_name;
        xbt_dynar_foreach(ruleroute->re_str_link, cpt, link_name) {
          char *new_link_name =
              remplace(link_name, list_src, rc_src, list_dst, rc_dst);
          void *link =
              xbt_dict_get_or_null(surf_network_model->resource_set,
                                   new_link_name);
          if (link)
            xbt_dynar_push(links_list, &link);
          else
            THROW1(mismatch_error, 0, "Link %s not found", new_link_name);
          xbt_free(new_link_name);
        }
      }
    }
    if (rc_src >= 0 && rc_dst >= 0)
      break;
  }

  route_extended_t new_e_route = NULL;
  if (rc_src >= 0 && rc_dst >= 0) {
    new_e_route = xbt_new0(s_route_extended_t, 1);
    new_e_route->generic_route.link_list = links_list;
  } else if (!strcmp(src, dst) && are_processing_units) {
    new_e_route = xbt_new0(s_route_extended_t, 1);
    xbt_dynar_push(links_list, &(global_routing->loopback));
    new_e_route->generic_route.link_list = links_list;
  } else {
    xbt_dynar_free(&link_list);
  }

  if (!are_processing_units && new_e_route) {
    rule_route_extended_t ruleroute_extended =
        (rule_route_extended_t) ruleroute;
    new_e_route->src_gateway =
        remplace(ruleroute_extended->re_src_gateway, list_src, rc_src,
                 list_dst, rc_dst);
    new_e_route->dst_gateway =
        remplace(ruleroute_extended->re_dst_gateway, list_src, rc_src,
                 list_dst, rc_dst);
  }

  if (list_src)
    pcre_free_substring_list(list_src);
  if (list_dst)
    pcre_free_substring_list(list_dst);

  return new_e_route;
}

static route_extended_t rulebased_get_bypass_route(routing_component_t rc,
                                                   const char *src,
                                                   const char *dst)
{
  return NULL;
}

static void rulebased_finalize(routing_component_t rc)
{
  routing_component_rulebased_t routing =
      (routing_component_rulebased_t) rc;
  if (routing) {
    xbt_dict_free(&routing->dict_processing_units);
    xbt_dict_free(&routing->dict_autonomous_systems);
    xbt_dynar_free(&routing->list_route);
    xbt_dynar_free(&routing->list_ASroute);
    /* Delete structure */
    xbt_free(routing);
  }
}

/* Creation routing model functions */
static void *model_rulebased_create(void)
{
  routing_component_rulebased_t new_component =
      xbt_new0(s_routing_component_rulebased_t, 1);
  new_component->generic_routing.set_processing_unit =
      model_rulebased_set_processing_unit;
  new_component->generic_routing.set_autonomous_system =
      model_rulebased_set_autonomous_system;
  new_component->generic_routing.set_route = model_rulebased_set_route;
  new_component->generic_routing.set_ASroute = model_rulebased_set_ASroute;
  new_component->generic_routing.set_bypassroute = model_rulebased_set_bypassroute;
  new_component->generic_routing.get_onelink_routes = rulebased_get_onelink_routes;
  new_component->generic_routing.get_route = rulebased_get_route;
  new_component->generic_routing.get_latency = generic_get_link_latency;
  new_component->generic_routing.get_bypass_route = rulebased_get_bypass_route;
  new_component->generic_routing.finalize = rulebased_finalize;
  new_component->generic_routing.get_network_element_type = get_network_element_type;
  /* initialization of internal structures */
  new_component->dict_processing_units = xbt_dict_new();
  new_component->dict_autonomous_systems = xbt_dict_new();
  new_component->list_route = xbt_dynar_new(sizeof(rule_route_t), &rule_route_free);
  new_component->list_ASroute =
      xbt_dynar_new(sizeof(rule_route_extended_t),
                    &rule_route_extended_free);
  return new_component;
}

static void model_rulebased_load(void)
{
  /* use "surfxml_add_callback" to add a parse function call */
}

static void model_rulebased_unload(void)
{
  /* use "surfxml_del_callback" to remove a parse function call */
}

static void model_rulebased_end(void)
{
}

#endif                          /* HAVE_PCRE_LIB */

/* ************************************************************************** */
/* ******************************* NO ROUTING ******************************* */

/* Routing model structure */
typedef struct {
  s_routing_component_t generic_routing;
} s_routing_component_none_t, *routing_component_none_t;

/* Business methods */
static xbt_dynar_t none_get_onelink_routes(routing_component_t rc)
{
  return NULL;
}

static route_extended_t none_get_route(routing_component_t rc,
                                       const char *src, const char *dst)
{
  return NULL;
}

static route_extended_t none_get_bypass_route(routing_component_t rc,
                                              const char *src,
                                              const char *dst)
{
  return NULL;
}

static void none_finalize(routing_component_t rc)
{
  xbt_free(rc);
}

static void none_set_processing_unit(routing_component_t rc,
                                     const char *name)
{
}

static void none_set_autonomous_system(routing_component_t rc,
                                       const char *name)
{
}

/* Creation routing model functions */
static void *model_none_create(void)
{
  routing_component_none_t new_component =
      xbt_new0(s_routing_component_none_t, 1);
  new_component->generic_routing.set_processing_unit =
      none_set_processing_unit;
  new_component->generic_routing.set_autonomous_system =
      none_set_autonomous_system;
  new_component->generic_routing.set_route = NULL;
  new_component->generic_routing.set_ASroute = NULL;
  new_component->generic_routing.set_bypassroute = NULL;
  new_component->generic_routing.get_route = none_get_route;
  new_component->generic_routing.get_onelink_routes =
      none_get_onelink_routes;
  new_component->generic_routing.get_bypass_route = none_get_bypass_route;
  new_component->generic_routing.finalize = none_finalize;
  return new_component;
}

static void model_none_load(void)
{
}

static void model_none_unload(void)
{
}

static void model_none_end(void)
{
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

static void generic_set_processing_unit(routing_component_t rc,
                                        const char *name)
{
  XBT_DEBUG("Load process unit \"%s\"", name);
  int *id = xbt_new0(int, 1);
  xbt_dict_t _to_index;
  _to_index = current_routing->to_index;
  *id = xbt_dict_length(_to_index);
  xbt_dict_set(_to_index, name, id, xbt_free);
}

static void generic_set_autonomous_system(routing_component_t rc,
                                          const char *name)
{
  XBT_DEBUG("Load Autonomous system \"%s\"", name);
  int *id = xbt_new0(int, 1);
  xbt_dict_t _to_index;
  _to_index = current_routing->to_index;
  *id = xbt_dict_length(_to_index);
  xbt_dict_set(_to_index, name, id, xbt_free);
}

static int surf_pointer_resource_cmp(const void *a, const void *b)
{
  return a != b;
}

static int surf_link_resource_cmp(const void *a, const void *b)
{
  return !!memcmp(a,b,global_routing->size_of_link);
}

static void generic_set_bypassroute(routing_component_t rc,
                                    const char *src, const char *dst,
                                    route_extended_t e_route)
{
  XBT_DEBUG("Load bypassRoute from \"%s\" to \"%s\"", src, dst);
  xbt_dict_t dict_bypassRoutes = rc->bypassRoutes;
  char *route_name;

  route_name = bprintf("%s#%s", src, dst);
  xbt_assert2(xbt_dynar_length(e_route->generic_route.link_list) > 0,
              "Invalid count of links, must be greater than zero (%s,%s)",
              src, dst);
  xbt_assert4(!xbt_dict_get_or_null(dict_bypassRoutes, route_name),
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

static double generic_get_link_latency(routing_component_t rc,
                                       const char *src, const char *dst)
{
	route_extended_t route = rc->get_route(rc,src,dst);
	void * link;
	unsigned int i;
	double latency = 0.0;

	xbt_dynar_foreach(route->generic_route.link_list,i,link) {
		latency += get_link_latency(link);
	}
	generic_free_extended_route(route);
  return latency;
}

static xbt_dynar_t generic_get_onelink_routes(routing_component_t rc)
{
  xbt_die("\"generic_get_onelink_routes\" not implemented yet");
}

static route_extended_t generic_get_bypassroute(routing_component_t rc,
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
  src_as = ((network_element_info_t)
            xbt_lib_get_or_null(host_lib, src, ROUTING_HOST_LEVEL))->rc_component;
  dst_as = ((network_element_info_t)
            xbt_lib_get_or_null(host_lib, dst, ROUTING_HOST_LEVEL))->rc_component;
  xbt_assert2(src_as
              && dst_as,
              "Ask for route \"from\"(%s) or \"to\"(%s) no found", src,
              dst);

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

static route_t
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

  xbt_assert0(hierarchy == SURF_ROUTING_BASE,
              "the hierarchy type is not SURF_ROUTING_BASE");

  links = ((route_t) data)->link_list;


  links_id = new_route->link_list;

  xbt_dynar_foreach(links, cpt, link_name) {

    void *link =
        xbt_dict_get_or_null(surf_network_model->resource_set, link_name);
    if (link) {
      if (order)
        xbt_dynar_push(links_id, &link);
      else
        xbt_dynar_unshift(links_id, &link);
    } else
      THROW1(mismatch_error, 0, "Link %s not found", link_name);
  }

  return new_route;
}

static route_extended_t
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

  xbt_assert0(hierarchy == SURF_ROUTING_BASE
              || hierarchy == SURF_ROUTING_RECURSIVE,
              "the hierarchy type is not defined");

  if (hierarchy == SURF_ROUTING_BASE) {

    route = (route_t) data;
    links = route->link_list;

  } else if (hierarchy == SURF_ROUTING_RECURSIVE) {

    e_route = (route_extended_t) data;
    xbt_assert0(e_route->src_gateway
                && e_route->dst_gateway, "bad gateway, is null");
    links = e_route->generic_route.link_list;

    /* remeber not erase the gateway names */
    new_e_route->src_gateway = e_route->src_gateway;
    new_e_route->dst_gateway = e_route->dst_gateway;
  }

  links_id = new_e_route->generic_route.link_list;

  xbt_dynar_foreach(links, cpt, link_name) {

    void *link =
        xbt_dict_get_or_null(surf_network_model->resource_set, link_name);
    if (link) {
      if (order)
        xbt_dynar_push(links_id, &link);
      else
        xbt_dynar_unshift(links_id, &link);
    } else
      THROW1(mismatch_error, 0, "Link %s not found", link_name);
  }

  return new_e_route;
}

static void generic_free_route(route_t route)
{
  if (route) {
    xbt_dynar_free(&(route->link_list));
    xbt_free(route);
  }
}

static void generic_free_extended_route(route_extended_t e_route)
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

static routing_component_t
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

static routing_component_t
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

static void generic_src_dst_check(routing_component_t rc, const char *src,
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
	 xbt_die("The routing component of src and dst is not the same as the network elements belong (%s==%s)",
     rc->name, dst_as->name);
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

static void routing_parse_Scluster(void)
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
  char *router_id, *link_router, *link_backbone;
  char *availability_file = bprintf("%s",cluster_availability_file);
  char *state_file = bprintf("%s",cluster_state_file);

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
      temp_cluster_power = bprintf("%s",cluster_power);
      temp_cluster_power = replace_random_parameter(temp_cluster_power);
      XBT_DEBUG("<host\tid=\"%s\"\tpower=\"%s\">", host_id, temp_cluster_power);
      A_surfxml_host_state = A_surfxml_host_state_ON;
      SURFXML_BUFFER_SET(host_id, host_id);
      SURFXML_BUFFER_SET(host_power, temp_cluster_power);
      SURFXML_BUFFER_SET(host_core, cluster_core);
      SURFXML_BUFFER_SET(host_availability, "1.0");
      SURFXML_BUFFER_SET(host_coordinates, "");
      xbt_free(availability_file);
      availability_file = bprintf("%s",cluster_availability_file);
      xbt_free(state_file);
      state_file = bprintf("%s",cluster_state_file);
      XBT_DEBUG("\tavailability_file=\"%s\"",xbt_str_varsubst(availability_file,patterns));
      XBT_DEBUG("\tstate_file=\"%s\"",xbt_str_varsubst(state_file,patterns));
      SURFXML_BUFFER_SET(host_availability_file, xbt_str_varsubst(availability_file,patterns));
      SURFXML_BUFFER_SET(host_state_file, xbt_str_varsubst(state_file,patterns));
      XBT_DEBUG("</host>");
      SURFXML_START_TAG(host);
      SURFXML_END_TAG(host);


      temp_cluster_bw = bprintf("%s",cluster_bw);
      temp_cluster_bw = replace_random_parameter(temp_cluster_bw);
      temp_cluster_lat = bprintf("%s",cluster_lat);
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
        temp_cluster_power = bprintf("%s",cluster_power);
        temp_cluster_power = replace_random_parameter(temp_cluster_power);
        XBT_DEBUG("<host\tid=\"%s\"\tpower=\"%s\">", host_id, temp_cluster_power);
        A_surfxml_host_state = A_surfxml_host_state_ON;
        SURFXML_BUFFER_SET(host_id, host_id);
        SURFXML_BUFFER_SET(host_power, temp_cluster_power);
        SURFXML_BUFFER_SET(host_core, cluster_core);
        SURFXML_BUFFER_SET(host_availability, "1.0");
        SURFXML_BUFFER_SET(host_coordinates, "");
        xbt_free(availability_file);
        availability_file = bprintf("%s",cluster_availability_file);
        xbt_free(state_file);
        state_file = bprintf("%s",cluster_state_file);
        XBT_DEBUG("\tavailability_file=\"%s\"",xbt_str_varsubst(availability_file,patterns));
        XBT_DEBUG("\tstate_file=\"%s\"",xbt_str_varsubst(state_file,patterns));
        SURFXML_BUFFER_SET(host_availability_file, xbt_str_varsubst(availability_file,patterns));
        SURFXML_BUFFER_SET(host_state_file, xbt_str_varsubst(state_file,patterns));
        XBT_DEBUG("</host>");
        SURFXML_START_TAG(host);
        SURFXML_END_TAG(host);

        xbt_free(temp_cluster_power);

        temp_cluster_bw = bprintf("%s",cluster_bw);
        temp_cluster_bw = replace_random_parameter(temp_cluster_bw);
        temp_cluster_lat = bprintf("%s",cluster_lat);
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
  link_router = bprintf("%s_link_%s_router", cluster_id, cluster_id);
  link_backbone = bprintf("%s_backbone", cluster_id);

  XBT_DEBUG("<router id=\"%s\"/>", router_id);
  SURFXML_BUFFER_SET(router_id, router_id);
  SURFXML_START_TAG(router);
  SURFXML_END_TAG(router);

  //TODO
  xbt_dict_set(patterns, "radical", bprintf("_router"), xbt_free);
  temp_cluster_bw = bprintf("%s",cluster_bw);
  temp_cluster_bw = replace_random_parameter(temp_cluster_bw);
  temp_cluster_lat = bprintf("%s",cluster_lat);
  temp_cluster_lat = replace_random_parameter(temp_cluster_lat);
  XBT_DEBUG("<link\tid=\"%s\" bw=\"%s\" lat=\"%s\"/>", link_router,temp_cluster_bw, temp_cluster_lat);
  A_surfxml_link_state = A_surfxml_link_state_ON;
  A_surfxml_link_sharing_policy = A_surfxml_link_sharing_policy_SHARED;
  if(cluster_sharing_policy == A_surfxml_cluster_sharing_policy_FULLDUPLEX)
  {A_surfxml_link_sharing_policy =  A_surfxml_link_sharing_policy_FULLDUPLEX;}
  if(cluster_sharing_policy == A_surfxml_cluster_sharing_policy_FATPIPE)
  {A_surfxml_link_sharing_policy =  A_surfxml_link_sharing_policy_FATPIPE;}
  SURFXML_BUFFER_SET(link_id, link_router);
  SURFXML_BUFFER_SET(link_bandwidth, temp_cluster_bw);
  SURFXML_BUFFER_SET(link_latency, temp_cluster_lat);
  SURFXML_BUFFER_SET(link_bandwidth_file, "");
  SURFXML_BUFFER_SET(link_latency_file, "");
  SURFXML_BUFFER_SET(link_state_file, "");
  SURFXML_START_TAG(link);
  SURFXML_END_TAG(link);

  xbt_free(temp_cluster_bw);
  xbt_free(temp_cluster_lat);

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

      if (i == xbt_dynar_length(tab_elements_num)) {
        route_src = link_router;
      } else {
        route_src =
            bprintf("%s_link_%d", cluster_id,
                    xbt_dynar_get_as(tab_elements_num, i, int));
      }

      if (j == xbt_dynar_length(tab_elements_num)) {
        route_dst = link_router;
      } else {
        route_dst =
            bprintf("%s_link_%d", cluster_id,
                    xbt_dynar_get_as(tab_elements_num, j, int));
      }

      XBT_DEBUG("<link_ctn\tid=\"%s\"/>", route_src);
      SURFXML_BUFFER_SET(link_ctn_id, route_src);
      A_surfxml_link_ctn_direction = A_surfxml_link_ctn_direction_NONE;
      if(cluster_sharing_policy == A_surfxml_cluster_sharing_policy_FULLDUPLEX)
      {A_surfxml_link_ctn_direction = A_surfxml_link_ctn_direction_UP;}
      SURFXML_START_TAG(link_ctn);
      SURFXML_END_TAG(link_ctn);

      XBT_DEBUG("<link_ctn\tid=\"%s_backbone\"/>", cluster_id);
      SURFXML_BUFFER_SET(link_ctn_id, bprintf("%s_backbone", cluster_id));
      A_surfxml_link_ctn_direction = A_surfxml_link_ctn_direction_NONE;
      SURFXML_START_TAG(link_ctn);
      SURFXML_END_TAG(link_ctn);

      XBT_DEBUG("<link_ctn\tid=\"%s\"/>", route_dst);
      SURFXML_BUFFER_SET(link_ctn_id, route_dst);
      A_surfxml_link_ctn_direction = A_surfxml_link_ctn_direction_NONE;
      if(cluster_sharing_policy == A_surfxml_cluster_sharing_policy_FULLDUPLEX)
      {A_surfxml_link_ctn_direction = A_surfxml_link_ctn_direction_DOWN;}
      SURFXML_START_TAG(link_ctn);
      SURFXML_END_TAG(link_ctn);

      XBT_DEBUG("</route>");
      SURFXML_END_TAG(route);
    }
  }
  xbt_dynar_free(&tab_elements_num);

#endif

  free(router_id);
  free(link_backbone);
  free(link_router);
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
	XBT_DEBUG("Clean dict for random");
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

#ifdef HAVE_PCRE_LIB

#endif

  int peer_sharing_policy = AX_surfxml_peer_sharing_policy;

#ifndef HAVE_PCRE_LIB
  //xbt_dynar_t tab_elements_num = xbt_dynar_new(sizeof(int), NULL);
  //char *route_src, *route_dst;
  //int j;
#endif

  static unsigned int surfxml_buffer_stack_stack_ptr = 1;
  static unsigned int surfxml_buffer_stack_stack[1024];

  surfxml_buffer_stack_stack[0] = 0;

  surfxml_bufferstack_push(1);

  SURFXML_BUFFER_SET(AS_id, peer_id);
  SURFXML_BUFFER_SET(AS_coordinates, peer_coord);
#ifdef HAVE_PCRE_LIB
  SURFXML_BUFFER_SET(AS_routing, "RuleBased");
  XBT_DEBUG("<AS id=\"%s\"\trouting=\"RuleBased\">", peer_id);
#else
  SURFXML_BUFFER_SET(AS_routing, "Full");
  XBT_DEBUG("<AS id=\"%s\"\trouting=\"Full\">", peer_id);
#endif
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
  if(peer_sharing_policy == A_surfxml_peer_sharing_policy_FULLDUPLEX)
{A_surfxml_link_sharing_policy =  A_surfxml_link_sharing_policy_FULLDUPLEX;}
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
  if(peer_sharing_policy == A_surfxml_cluster_sharing_policy_FULLDUPLEX)
{A_surfxml_link_sharing_policy =  A_surfxml_link_sharing_policy_FULLDUPLEX;}
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
  XBT_DEBUG("<route\tsrc=\"%s\"\tdst=\"%s\"", peer_id, router_id);
  XBT_DEBUG("symmetrical=\"NO\">");
  SURFXML_BUFFER_SET(route_src, peer_id);
  SURFXML_BUFFER_SET(route_dst, router_id);
  A_surfxml_route_symmetrical = A_surfxml_route_symmetrical_NO;
  SURFXML_START_TAG(route);

  XBT_DEBUG("<link_ctn\tid=\"%s\"/>", link_id_up);
  SURFXML_BUFFER_SET(link_ctn_id, link_id_up);
  A_surfxml_link_ctn_direction = A_surfxml_link_ctn_direction_NONE;
  if(peer_sharing_policy == A_surfxml_peer_sharing_policy_FULLDUPLEX)
  {A_surfxml_link_ctn_direction = A_surfxml_link_ctn_direction_UP;}
  SURFXML_START_TAG(link_ctn);
  SURFXML_END_TAG(link_ctn);

  XBT_DEBUG("</route>");
  SURFXML_END_TAG(route);

  //Opposite Route
  XBT_DEBUG("<route\tsrc=\"%s\"\tdst=\"%s\"", router_id, peer_id);
  XBT_DEBUG("symmetrical=\"NO\">");
  SURFXML_BUFFER_SET(route_src, router_id);
  SURFXML_BUFFER_SET(route_dst, peer_id);
  A_surfxml_route_symmetrical = A_surfxml_route_symmetrical_NO;
  SURFXML_START_TAG(route);

  XBT_DEBUG("<link_ctn\tid=\"%s\"/>", link_id_down);
  SURFXML_BUFFER_SET(link_ctn_id, link_id_down);
  A_surfxml_link_ctn_direction = A_surfxml_link_ctn_direction_NONE;
  if(peer_sharing_policy == A_surfxml_peer_sharing_policy_FULLDUPLEX)
  {A_surfxml_link_ctn_direction = A_surfxml_link_ctn_direction_DOWN;}
  SURFXML_START_TAG(link_ctn);
  SURFXML_END_TAG(link_ctn);

  XBT_DEBUG("</route>");
  SURFXML_END_TAG(route);

  XBT_DEBUG("</AS>");
  SURFXML_END_TAG(AS);
  XBT_DEBUG(" ");

  //xbt_dynar_free(&tab_elements_num);

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
	    THROW2(arg_error, 0, "random->max < random->min (%f < %f)", max, min);
	  if (mean < min)
	    THROW2(arg_error, 0, "random->mean < random->min (%f < %f)", mean,
		   min);
	  if (mean > max)
	    THROW2(arg_error, 0, "random->mean > random->max (%f > %f)", mean,
		   max);

	  /* normalize the mean and standard deviation before storing */
	  random->mean = (mean - min) / (max - min);
	  random->std = std / (max - min);

	  if (random->mean * (1 - random->mean) < random->std * random->std)
	    THROW2(arg_error, 0, "Invalid mean and standard deviation (%f and %f)",
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
					  xbt_assert1(!xbt_dict_get_or_null(random_value,random_id),"Custom Random '%s' already exists !",random_id);
					  res = random_generate(random);
                                          tmpbuf = bprintf("%s%d",random_id,atoi(xbt_dynar_getfirst_as(radical_ends,char *)));
                                          xbt_dict_set(random_value, tmpbuf, bprintf("%f",res), free);
                                          xbt_free(tmpbuf);
					  break;

			case 2:	  surf_parse_get_int(&start,
										 xbt_dynar_get_as(radical_ends, 0, char *));
					  surf_parse_get_int(&end, xbt_dynar_get_as(radical_ends, 1, char *));
					  for (i = start; i <= end; i++) {
						  xbt_assert1(!xbt_dict_get_or_null(random_value,random_id),"Custom Random '%s' already exists !",bprintf("%s%d",random_id,i));
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
