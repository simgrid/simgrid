/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <float.h>
#include "surf_private.h"
#include "xbt/dynar.h"
#include "xbt/str.h"
#include "xbt/config.h"
#include "xbt/graph.h"
#include "xbt/set.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route,surf,"Routing part of surf");

    
////////////////////////////////////////////////////////////////////////////////
// HERE START THE NEW CODE
////////////////////////////////////////////////////////////////////////////////

//...... DEBUG ONLY .... //
static void print_tree(routing_component_t rc);
static void print_global();
static void print_AS_start(void);
static void print_AS_end(void);
static void print_host(void);
static void print_link(void);
static void print_route(void);
static void print_ctn(void);
static void DEBUG_exit(void);

////////////////////////////////////////////////////////////////////////////////
// HERE START THE NEW CODE
////////////////////////////////////////////////////////////////////////////////

/* Global vars */
routing_global_t global_routing = NULL;
routing_component_t current_routing = NULL;
model_type_t current_routing_model = NULL;

/* Prototypes of each model */
static void* model_full_create(void); /* create structures for full routing model */
static void  model_full_load(void);   /* load parse functions for full routing model */
static void  model_full_unload(void); /* unload parse functions for full routing model */
static void  model_full_end(void);    /* finalize the creation of full routing model */

static void* model_floyd_create(void); /* create structures for floyd routing model */
static void  model_floyd_load(void);   /* load parse functions for floyd routing model */
static void  model_floyd_unload(void); /* unload parse functions for floyd routing model */
static void  model_floyd_end(void);    /* finalize the creation of floyd routing model */

static void* model_dijkstra_create(void); /* create structures for dijkstra routing model */
static void  model_dijkstra_load(void);   /* load parse functions for dijkstra routing model */
static void  model_dijkstra_unload(void); /* unload parse functions for dijkstra routing model */
static void  model_dijkstra_end(void);    /* finalize the creation of dijkstra routing model */

static void* model_dijkstracache_create(void); /* create structures for dijkstracache routing model */
static void  model_dijkstracache_load(void);   /* load parse functions for dijkstracache routing model */
static void  model_dijkstracache_unload(void); /* unload parse functions for dijkstracache routing model */
static void  model_dijkstracache_end(void);    /* finalize the creation of dijkstracache routing model */

static void* model_none_create(void); /* none routing model */
static void  model_none_load(void);   /* none routing model */
static void  model_none_unload(void); /* none routing model */
static void  model_none_end(void);    /* none routing model */

/* Table of routing models */
// typedef enum {
//   SURF_MODEL_FULL = 0,
//   SURF_MODEL_FLOYD,
//   SURF_MODEL_DIJKSTRA,
//   SURF_MODEL_DIJKSTRACACHE,
//   SURF_MODEL_NONE
// } e_surf_model_type_t;

#define SURF_MODEL_FULL           0
#define SURF_MODEL_FLOYD          1
#define SURF_MODEL_DIJKSTRA       2
#define SURF_MODEL_DIJKSTRACACHE  3
#define SURF_MODEL_NONE           4

/* must be finish with null and carefull if change de order */
struct s_model_type routing_models[] =
{ {"Full", "Full routing data (fast, large memory requirements, fully expressive)",
  model_full_create, model_full_load, model_full_unload, model_full_end },  
  {"Floyd", "Floyd routing data (slow initialization, fast lookup, lesser memory requirements, shortest path routing only)",
  model_floyd_create, model_floyd_load, model_floyd_unload, model_floyd_end },
  {"Dijkstra", "Dijkstra routing data (fast initialization, slow lookup, small memory requirements, shortest path routing only)",
  model_dijkstra_create ,model_dijkstra_load, model_dijkstra_unload, model_dijkstra_end },
  {"DijkstraCache", "Dijkstra routing data (fast initialization, fast lookup, small memory requirements, shortest path routing only)",
  model_dijkstracache_create, model_dijkstracache_load, model_dijkstracache_unload, model_dijkstracache_end },
  {"none", "No routing (usable with Constant network only)",
  model_none_create, model_none_load, model_none_unload, model_none_end },
  {NULL,NULL,NULL,NULL,NULL,NULL}};

/* global parse functions */

static char* src = NULL;    /* temporary store the source name of a route */
static char* dst = NULL;    /* temporary store the destination name of a route */
static char* gw_src = NULL; /* temporary store the gateway source name of a route */
static char* gw_dst = NULL; /* temporary store the gateway destination name of a route */
static xbt_dynar_t link_list = NULL; /* temporary store of current list link of a route */ 

/**
 * \brief Add a "host" to the network element list
 */
static void  parse_S_host(void) {
  if( current_routing->hierarchy == SURF_ROUTING_NULL ) current_routing->hierarchy = SURF_ROUTING_BASE;
  xbt_assert1(!xbt_dict_get_or_null(global_routing->where_network_elements,A_surfxml_host_id),
      "Reading a host, processing unit \"%s\" already exist",A_surfxml_host_id);
// FIXME: checked by parser
//   xbt_assert1(current_routing->hierarchy==SURF_ROUTING_BASE,
//       "Bad declaration of processing unit \"%s\"",A_surfxml_host_id);
  (*(current_routing->set_processing_units))(current_routing,A_surfxml_host_id);
  xbt_dict_set(global_routing->where_network_elements,A_surfxml_host_id,(void*)current_routing,NULL); 
}

/**
 * \brief Add a "router" to the network element list
 */
static void parse_S_router(void) {
  if( current_routing->hierarchy == SURF_ROUTING_NULL ) current_routing->hierarchy = SURF_ROUTING_BASE;
  xbt_assert1(!xbt_dict_get_or_null(global_routing->where_network_elements,A_surfxml_router_id),
      "Reading a router, processing unit \"%s\" already exist",A_surfxml_router_id);
// FIXME: checked by parser
//   xbt_assert1(current_routing->hierarchy==SURF_ROUTING_BASE,
//       "Bad declaration of processing unit \"%s\"",A_surfxml_host_id);
  (*(current_routing->set_processing_units))(current_routing,A_surfxml_router_id);
  xbt_dict_set(global_routing->where_network_elements,A_surfxml_router_id,(void*)current_routing,NULL); 
}

/**
 * \brief Set the endponints for a route
 */
static void parse_S_route_new_and_endpoints(void) {
  // FIXME: checked by parser
//   xbt_assert1(current_routing->hierarchy==SURF_ROUTING_BASE,
//     "Bad declaration of route in \"%s\"",current_routing->name);
  if( src != NULL && dst != NULL && link_list != NULL )
    THROW2(arg_error,0,"Route between %s to %s can not be defined",A_surfxml_route_src,A_surfxml_route_dst);
  src = A_surfxml_route_src;
  dst = A_surfxml_route_dst;
  link_list = xbt_dynar_new(sizeof(char*), &xbt_free_ref);
}

/**
 * \brief Set the endponints and gateways for a ASroute
 */
static void parse_S_ASroute_new_and_endpoints(void) {
// FIXME: checked by parser
//   xbt_assert1(current_routing->hierarchy==SURF_ROUTING_RECURSIVE,
//     "Bad declaration of ASroute in \"%s\"",current_routing->name);
  if( src != NULL && dst != NULL && link_list != NULL )
    THROW2(arg_error,0,"Route between %s to %s can not be defined",A_surfxml_ASroute_src,A_surfxml_ASroute_dst);
  src = A_surfxml_ASroute_src;
  dst = A_surfxml_ASroute_dst;
  gw_src = A_surfxml_ASroute_gw_src;
  gw_dst = A_surfxml_ASroute_gw_dst;
  link_list = xbt_dynar_new(sizeof(char*), &xbt_free_ref);
}

/**
 * \brief Set a new link on the actual list of link for a route or ASroute
 */
static void parse_E_link_c_ctn_new_elem(void) {
  char *val;
  val = xbt_strdup(A_surfxml_link_c_ctn_id);
  xbt_dynar_push(link_list, &val);
}

/**
 * \brief Store de route by calling the set_route function of the current routing component
 */
static void parse_E_route_store_route(void) {
// FIXME: checked by parser
//   xbt_assert1(current_routing->hierarchy==SURF_ROUTING_BASE,
//     "Bad declaration of route in \"%s\"",current_routing->name);
  route_t route = xbt_new(s_route_t,1);
  route->link_list = link_list;
  // TODO check if are correct
  (*(current_routing->set_route))(current_routing,src,dst,route);
  link_list = NULL;
  src = NULL;
  dst = NULL;
}

/**
 * \brief Store de ASroute by calling the set_ASroute function of the current routing component
 */
static void parse_E_ASroute_store_route(void) {
// FIXME: checked by parser
//   xbt_assert1(current_routing->hierarchy==SURF_ROUTING_RECURSIVE,
//     "Bad declaration of ASroute in \"%s\"",current_routing->name);
  route_extended_t e_route = xbt_new(s_route_extended_t,1);
  e_route->generic_route.link_list = link_list;
  e_route->src_gateway = xbt_strdup(gw_src);
  e_route->dst_gateway = xbt_strdup(gw_dst);
  // TODO check if are correct
  (*(current_routing->set_ASroute))(current_routing,src,dst,e_route);
  link_list = NULL;
  src = NULL;
  dst = NULL;
  gw_src = NULL;
  gw_dst = NULL;
}

/**
 * \brief Make a new routing component
 *
 * Detect the routing model type of the routing component, make the new structure and
 * set the parsing functions to allows parsing the part of the routing tree
 */
static void parse_S_AS(void) { 
  routing_component_t new_routing;
  model_type_t model = NULL;
  char* wanted = A_surfxml_AS_routing;
  int cpt;
  /* seach the routing model */
  for (cpt=0;routing_models[cpt].name;cpt++)
    if (!strcmp(wanted,routing_models[cpt].name))
          model = &routing_models[cpt];
  /* if its not exist, error */
  if( model == NULL ) {
    fprintf(stderr,"Routing model %s not found. Existing models:\n",wanted);
    for (cpt=0;routing_models[cpt].name;cpt++)
      if (!strcmp(wanted,routing_models[cpt].name))
        fprintf(stderr,"   %s: %s\n",routing_models[cpt].name,routing_models[cpt].desc);
    xbt_die(NULL);
  }

  /* make a new routing component */
  new_routing = (routing_component_t)(*(model->create))();
  
  /* FIXME: for now, if I forget to declare */  
  xbt_assert1( new_routing->set_processing_units,
       "Bad routing type, \"set_processing_units\" is not declared for \"%s\"",A_surfxml_AS_id);
  xbt_assert1( new_routing->set_autonomous_system,
       "Bad routing type, \"set_autonomous_system\" is not declared for \"%s\"",A_surfxml_AS_id);
  xbt_assert1( new_routing->set_route,
       "Bad routing type, \"set_route\" is not declared for \"%s\"",A_surfxml_AS_id);
  xbt_assert1( new_routing->set_ASroute,
       "Bad routing type, \"set_ASroute\" is not declared for \"%s\"",A_surfxml_AS_id);
  xbt_assert1( new_routing->finalize,
       "Bad routing type, \"finalize\" is not declared for \"%s\"",A_surfxml_AS_id);
       
  new_routing->routing = model;
  new_routing->hierarchy = SURF_ROUTING_NULL;
  new_routing->name = xbt_strdup(A_surfxml_AS_id);
  new_routing->routing_sons = xbt_dict_new();
  INFO2("Routing %s for AS %s",A_surfxml_AS_routing,A_surfxml_AS_id);
  
  if( current_routing == NULL && global_routing->root == NULL ){
    
    /* it is the first one */
    new_routing->routing_father = NULL;
    global_routing->root = new_routing;
    
  } else if( current_routing != NULL && global_routing->root != NULL ) { 
    
    xbt_assert1(!xbt_dict_get_or_null(current_routing->routing_sons,A_surfxml_AS_id),
           "The AS \"%s\" already exist",A_surfxml_AS_id);
     /* it is a part of the tree */
    new_routing->routing_father = current_routing;
    /* set the father behavior */
    if( current_routing->hierarchy == SURF_ROUTING_NULL ) current_routing->hierarchy = SURF_ROUTING_RECURSIVE;
// FIXME: checked by parser
//     xbt_assert1(current_routing->hierarchy==SURF_ROUTING_RECURSIVE,
//       "Bad declaration of AS \"%s\"",A_surfxml_AS_id);
    /* add to the sons dictionary */
    xbt_dict_set(current_routing->routing_sons,A_surfxml_AS_id,(void*)new_routing,NULL);
    /* add to the father element list */
    (*(current_routing->set_autonomous_system))(current_routing,A_surfxml_AS_id);
    /* unload the prev parse rules */
    (*(current_routing->routing->unload))();
    
  } else {
    THROW0(arg_error,0,"All defined components must be belong to a AS");
  }
  /* set the new parse rules */
  (*(new_routing->routing->load))();
  /* set the new current component of the tree */
  current_routing = new_routing;
}

/**
 * \brief Finish the creation of a new routing component
 *
 * When you finish to read the routing component, other structures must be created. 
 * the "end" method allow to do that for any routing model type
 */
static void parse_E_AS(void) {

  if( current_routing == NULL ) {
    THROW1(arg_error,0,"Close AS(%s), that never open",A_surfxml_AS_id);
  } else {
      (*(current_routing->routing->unload))();
      (*(current_routing->routing->end))();
      current_routing = current_routing->routing_father;
      if( current_routing != NULL )
        (*(current_routing->routing->load))();
  }
}

/**
 * \brief Recursive function for add the differents AS to global dict of network element
 *
 * This fuction is call by "parse_E_platform_add_parse_AS". It allow to add the 
 * AS or routing components, to the where_network_elements dictionary. In the same
 * way as "parse_S_host", "parse_S_router" and "parse_S_gateway" do.
 */
static void _add_parse_AS(routing_component_t rc) {
  xbt_assert1(!xbt_dict_get_or_null(global_routing->where_network_elements,rc->name),
     "The AS \"%s\" already exist",rc->name);
  xbt_dict_set(global_routing->where_network_elements,rc->name,rc->routing_father,NULL);
  xbt_dict_cursor_t cursor = NULL;
  char *key;
  routing_component_t elem;
  xbt_dict_foreach(rc->routing_sons, cursor, key, elem) {
    _add_parse_AS(elem);
  } 
}

/**
 * \brief Add all "AS" to the global dict of network element
 *
 * Allows find a "AS" in any routing component
 */
static void parse_E_platform_add_parse_AS(void) {
  _add_parse_AS(global_routing->root);
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
static xbt_dynar_t elements_father(const char* src,const char* dst) {
  
  xbt_assert0(src&&dst,"bad parameters for \"elements_father\" method");
  
  xbt_dynar_t result = xbt_dynar_new(sizeof(char*), NULL); // &xbt_free_ref
  
  routing_component_t src_as, dst_as;
  int index_src, index_dst, index_father_src, index_father_dst;
  xbt_dynar_t path_src = NULL;
  xbt_dynar_t path_dst = NULL;
  routing_component_t current = NULL;
  routing_component_t* current_src = NULL;
  routing_component_t* current_dst = NULL;
  routing_component_t* father = NULL;
  
  /* (1) find the as where the src and dst are located */
  src_as = xbt_dict_get_or_null(global_routing->where_network_elements,src);
  dst_as = xbt_dict_get_or_null(global_routing->where_network_elements,dst); 
  xbt_assert2(src_as&&dst_as, "Ask for route \"from\"(%s) or \"to\"(%s) no found",src,dst);
  
  /* (2) find the path to the root routing component */
  path_src = xbt_dynar_new(sizeof(routing_component_t), NULL);
  current = src_as; 
  while( current != NULL ) {
    xbt_dynar_push(path_src,&current);
    current = current->routing_father;
  }
  path_dst = xbt_dynar_new(sizeof(routing_component_t), NULL);
  current = dst_as; 
  while( current != NULL ) {
    xbt_dynar_push(path_dst,&current);
    current = current->routing_father;
  }
  
  /* (3) find the common father */
  index_src  = (path_src->used)-1;
  index_dst  = (path_dst->used)-1;
  current_src = xbt_dynar_get_ptr(path_src,index_src);
  current_dst = xbt_dynar_get_ptr(path_dst,index_dst);
  while( index_src >= 0 && index_dst >= 0 && *current_src == *current_dst ) {
    current_src = xbt_dynar_get_ptr(path_src,index_src);
    current_dst = xbt_dynar_get_ptr(path_dst,index_dst);
    index_src--;
    index_dst--;
  }
  index_src++;
  index_dst++;
  current_src = xbt_dynar_get_ptr(path_src,index_src);
  current_dst = xbt_dynar_get_ptr(path_dst,index_dst);

  /* (4) they are not in the same routing component, make the path */
  index_father_src = index_src+1;
  index_father_dst = index_dst+1;
   
  if(*current_src==*current_dst)
    father = current_src;
  else
    father = xbt_dynar_get_ptr(path_src,index_father_src);
  
  /* (5) result generation */
  xbt_dynar_push(result,father); // first same the father of src and dst
  xbt_dynar_push(result,current_src); // second the first different father of src 
  xbt_dynar_push(result,current_dst); // three  the first different father of dst
  
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
static route_extended_t _get_route(const char* src,const char* dst) {
  
  void* link;
  unsigned int cpt=0;
  
  DEBUG2("Solve route  \"%s\" to \"%s\"",src,dst);
     
  xbt_assert0(src&&dst,"bad parameters for \"_get_route\" method");
  
  route_extended_t e_route, e_route_cnt, e_route_src, e_route_dst;
  
  xbt_dynar_t elem_father_list = elements_father(src,dst);
  
  routing_component_t common_father = xbt_dynar_get_as(elem_father_list, 0, routing_component_t);
  routing_component_t src_father    = xbt_dynar_get_as(elem_father_list, 1, routing_component_t);
  routing_component_t dst_father    = xbt_dynar_get_as(elem_father_list, 2, routing_component_t);
  
  e_route = xbt_new0(s_route_extended_t,1);
  e_route->src_gateway = NULL;
  e_route->dst_gateway = NULL;
  e_route->generic_route.link_list = xbt_dynar_new(global_routing->size_of_link,NULL);
  
  if(src_father==dst_father) { /* SURF_ROUTING_BASE */
    
    if( strcmp(src,dst) ){
      e_route_cnt = (*(common_father->get_route))(common_father,src,dst);
      xbt_dynar_foreach(e_route_cnt->generic_route.link_list, cpt, link) {
        xbt_dynar_push(e_route->generic_route.link_list,&link);
      }
      xbt_dynar_free( &(e_route_cnt->generic_route.link_list) );
      xbt_free(e_route_cnt);
    }
    
  } else { /* SURF_ROUTING_RECURSIVE */
    
      e_route_cnt = (*(common_father->get_route))(common_father,src_father->name,dst_father->name);
      
      xbt_assert2( (e_route_cnt->src_gateway==NULL) == (e_route_cnt->dst_gateway==NULL) ,
        "bad gateway for route between \"%s\" and \"%s\"",src,dst);

      if( src != e_route_cnt->src_gateway ) {
        e_route_src = _get_route(src,e_route_cnt->src_gateway);
        xbt_dynar_foreach(e_route_src->generic_route.link_list, cpt, link) {
          xbt_dynar_push(e_route->generic_route.link_list,&link);
        }
      }
      
      xbt_dynar_foreach(e_route_cnt->generic_route.link_list, cpt, link) {
        xbt_dynar_push(e_route->generic_route.link_list,&link);
      }
      
      if( e_route_cnt->dst_gateway != dst ) {
        e_route_dst = _get_route(e_route_cnt->dst_gateway,dst);
        xbt_dynar_foreach(e_route_dst->generic_route.link_list, cpt, link) {
          xbt_dynar_push(e_route->generic_route.link_list,&link);
        }
      }
      
      e_route->src_gateway = e_route_cnt->src_gateway;
      e_route->dst_gateway = e_route_cnt->dst_gateway;

      xbt_dynar_free( &(e_route_src->generic_route.link_list) );
      xbt_free(e_route_src);
      xbt_dynar_free( &(e_route_cnt->generic_route.link_list) );
      xbt_free(e_route_cnt);
      xbt_dynar_free( &(e_route_dst->generic_route.link_list) );
      xbt_free(e_route_dst);
  }
  
  xbt_dynar_free(&elem_father_list);
  
  return e_route; 
}

/**
 * \brief Generic method: find a route between hosts
 *
 * \param src the source host name 
 * \param dst the destination host name
 * 
 * walk through the routing components tree and find a route between hosts
 * by calling the differents "get_route" functions in each routing component
 */
static xbt_dynar_t get_route(const char* src,const char* dst) {
  
  if(global_routing->last_route) xbt_dynar_free( &(global_routing->last_route) );
  
  route_extended_t e_route;
  xbt_dynar_t elem_father_list = elements_father(src,dst);
  routing_component_t common_father = xbt_dynar_get_as(elem_father_list, 0, routing_component_t);
  
  if(strcmp(src,dst))
    e_route = _get_route(src,dst);
  else
    e_route = (*(common_father->get_route))(common_father,src,dst);
  
  global_routing->last_route = e_route->generic_route.link_list;
 
  xbt_free(e_route);
  xbt_dynar_free(&elem_father_list);
  
  if( xbt_dynar_length(global_routing->last_route)==0 )
    return NULL;
  else
    return global_routing->last_route;
}

/**
 * \brief Recursive function for finalize
 *
 * \param rc the source host name 
 * 
 * This fuction is call by "finalize". It allow to finalize the 
 * AS or routing components. It delete all the structures.
 */
static void _finalize(routing_component_t rc) {
  if(rc) {
    xbt_dict_cursor_t cursor = NULL;
    char *key;
    routing_component_t elem;
    xbt_dict_foreach(rc->routing_sons, cursor, key, elem) {
      _finalize(elem);
    }
    xbt_dict_t tmp_sons = rc->routing_sons;
    char* tmp_name = rc->name;
    xbt_dict_free(&tmp_sons);
    xbt_free(tmp_name);
    (*(rc->finalize))(rc);
  }
}

/**
 * \brief Generic method: delete all the routing structures
 * 
 * walk through the routing components tree and delete the structures
 * by calling the differents "finalize" functions in each routing component
 */
static void finalize(void) {
  /* delete recursibly all the tree */  
  _finalize(global_routing->root);
  /* delete "where" dict */
  xbt_dict_free(&(global_routing->where_network_elements));
  /* delete last_route */
  xbt_dynar_free(&(global_routing->last_route));
  /* delete global routing structure */
  xbt_free(global_routing);
}

/**
 * \brief Generic method: create the global routing schema
 * 
 * Make a global routing structure and set all the parsing functions.
 */
void routing_model_create(size_t size_of_links, void* loopback) {
  
  /* config the uniq global routing */
  global_routing = xbt_new0(s_routing_global_t,1);
  global_routing->where_network_elements = xbt_dict_new();
  global_routing->root = NULL;
  global_routing->get_route = get_route;
  global_routing->finalize = finalize;
  global_routing->loopback = loopback;
  global_routing->size_of_link = size_of_links;
  global_routing->last_route = xbt_dynar_new(size_of_links, NULL);
  
  /* no current routing at moment */
  current_routing = NULL;

  /* parse generic elements */
  surfxml_add_callback(STag_surfxml_host_cb_list, &parse_S_host);
  surfxml_add_callback(STag_surfxml_router_cb_list, &parse_S_router);

  surfxml_add_callback(STag_surfxml_route_cb_list, &parse_S_route_new_and_endpoints);
  surfxml_add_callback(STag_surfxml_ASroute_cb_list, &parse_S_ASroute_new_and_endpoints);
  
  surfxml_add_callback(ETag_surfxml_link_c_ctn_cb_list, &parse_E_link_c_ctn_new_elem);
  
  surfxml_add_callback(ETag_surfxml_route_cb_list, &parse_E_route_store_route);
  surfxml_add_callback(ETag_surfxml_ASroute_cb_list, &parse_E_ASroute_store_route);
  
  surfxml_add_callback(STag_surfxml_AS_cb_list, &parse_S_AS);
  surfxml_add_callback(ETag_surfxml_AS_cb_list, &parse_E_AS);
  
  /* set all the as in the global where table (recursive fuction) */
  surfxml_add_callback(ETag_surfxml_platform_cb_list, &parse_E_platform_add_parse_AS);
  
  /* DEBUG ONLY */  
  //surfxml_add_callback(ETag_surfxml_platform_cb_list, &DEBUG_exit);
}

/* ************************************************************************** */
/* ***************** GENERIC PARSE FUNCTIONS (declarations) ***************** */

static void generic_set_processing_units(routing_component_t rc, const char* name);
static void generic_set_autonomous_system(routing_component_t rc, const char* name);
static void generic_set_route(routing_component_t rc, const char* src, const char* dst, route_t route);
static void generic_set_ASroute(routing_component_t rc, const char* src, const char* dst, route_extended_t e_route);

/* ************************************************************************** */
/* ****************** GENERIC AUX FUNCTIONS (declarations) ****************** */

static route_extended_t generic_new_extended_route(routing_component_t rc, void* data);

/* ************************************************************************** */
/* *************************** FULL ROUTING ********************************* */

#define TO_ROUTE_FULL(i,j) routing->routing_table[(i)+(j)*table_size]

/* Routing model structure */

typedef struct {
  s_routing_component_t generic_routing;
  xbt_dict_t parse_routes; /* store data during the parse process */
  xbt_dict_t to_index; /* char* -> network_element_t */
  route_extended_t *routing_table;
} s_routing_component_full_t,*routing_component_full_t;

/* Business methods */

static route_extended_t full_get_route(routing_component_t rc, const char* src,const char* dst) {

  routing_component_full_t routing = (routing_component_full_t)rc;
  int table_size = xbt_dict_length(routing->to_index);
  
  xbt_assert1(src && dst, "Invalid params for \"get_route\" function at AS \"%s\"",rc->name);
   
  routing_component_t src_as, dst_as;
  int *src_id,*dst_id;

  src_as = xbt_dict_get_or_null(global_routing->where_network_elements,src);
  dst_as = xbt_dict_get_or_null(global_routing->where_network_elements,dst);
   
  xbt_assert3(src_as != NULL && dst_as  != NULL, "Ask for route \"from\"(%s) or \"to\"(%s) no found at AS \"%s\"",src,dst,rc->name);
  xbt_assert4(src_as == dst_as, "The src(%s in %s) and dst(%s in %s) are in differents AS",src,src_as->name,dst,dst_as->name);
  xbt_assert2(rc == dst_as, "The routing component of src and dst is not the same as the network elements belong (%s==%s)",rc->name,dst_as->name);

  src_id = xbt_dict_get(routing->to_index,src);
  dst_id = xbt_dict_get(routing->to_index,dst);
  xbt_assert2(src_id && dst_id, "Ask for route \"from\"(%s)  or \"to\"(%s) no found in the local table",src,dst); 
  
  void* link;
  unsigned int cpt=0;
  route_extended_t new_e_route;
  route_extended_t e_route = TO_ROUTE_FULL(*src_id,*dst_id);
  if(e_route) {
    new_e_route = xbt_new0(s_route_extended_t,1);
    new_e_route->src_gateway = e_route->src_gateway;
    new_e_route->dst_gateway = e_route->dst_gateway;
    new_e_route->generic_route.link_list = xbt_dynar_new(global_routing->size_of_link,NULL);
    xbt_dynar_foreach(e_route->generic_route.link_list, cpt, link) {
      xbt_dynar_push(new_e_route->generic_route.link_list,&link);
    }
  } else {
    new_e_route = NULL;
  }
    return new_e_route;
}

static void full_finalize(routing_component_t rc) {
  routing_component_full_t routing = (routing_component_full_t)rc;
  int table_size = xbt_dict_length(routing->to_index);
  int i,j;
  if (routing) {
    /* Delete routing table */
    for (i=0;i<table_size;i++) {
      for (j=0;j<table_size;j++) {
        route_extended_t e_route = TO_ROUTE_FULL(i,j);
        if(e_route) {
          xbt_dynar_free(&(e_route->generic_route.link_list));
          if(e_route->src_gateway) xbt_free(e_route->src_gateway);
          if(e_route->dst_gateway) xbt_free(e_route->dst_gateway);
          xbt_free(e_route);
        }
      }
    }
    xbt_free(routing->routing_table);
    /* Delete index dict */
    xbt_dict_free(&(routing->to_index));
    /* Delete structure */
    xbt_free(rc);
  }
}

/* Creation routing model functions */

static void* model_full_create() {
  routing_component_full_t new_component =  xbt_new0(s_routing_component_full_t,1);
  new_component->generic_routing.set_processing_units = generic_set_processing_units;
  new_component->generic_routing.set_autonomous_system = generic_set_autonomous_system;
  new_component->generic_routing.set_route = generic_set_route;
  new_component->generic_routing.set_ASroute = generic_set_ASroute;
  new_component->generic_routing.get_route = full_get_route;
  new_component->generic_routing.finalize = full_finalize;
  new_component->to_index = xbt_dict_new();
  new_component->parse_routes = xbt_dict_new();
  return new_component;
}

static void model_full_load() {
 /* use "surfxml_add_callback" to add a parse function call */
}

static void model_full_unload() {
 /* use "surfxml_del_callback" to remove a parse function call */
}

static void  model_full_end() {
  
  char *key, *src_name, *dst_name;
  const char* sep = "#";
  int *src_id, *dst_id;
  unsigned int i, j;
  route_t route;
  route_extended_t e_route;
  void* data;
  
  xbt_dict_cursor_t cursor = NULL;
  xbt_dynar_t keys = NULL;
  
  /* set utils vars */
  routing_component_full_t routing = ((routing_component_full_t)current_routing);
  int table_size = xbt_dict_length(routing->to_index);
  
  /* Create the routing table */
  routing->routing_table = xbt_new0(route_extended_t, table_size * table_size);
  
  /* Put the routes in position */
  xbt_dict_foreach(routing->parse_routes, cursor, key, data) {
    
    keys = xbt_str_split_str(key, sep);

    src_name = xbt_dynar_get_as(keys, 0, char*);
    dst_name = xbt_dynar_get_as(keys, 1, char*);
    
    src_id = xbt_dict_get_or_null(routing->to_index, src_name);
    dst_id = xbt_dict_get_or_null(routing->to_index, dst_name);
    
    if (src_id == NULL || dst_id == NULL )
      THROW2(mismatch_error,0,"Network elements %s or %s not found", src_name, dst_name);
    
    TO_ROUTE_FULL(*src_id,*dst_id) = generic_new_extended_route(current_routing,data);

     xbt_dynar_free(&keys);
   }

   /* delete the parse table */
  xbt_dict_foreach(routing->parse_routes, cursor, key, data) {
    route = (route_t)data;
    xbt_dynar_free(&(route->link_list));
    xbt_free(data);
  }
      
  /* delete parse dict */
  xbt_dict_free(&(routing->parse_routes));

  /* Add the loopback if needed */
  for (i = 0; i < table_size; i++) {
    e_route = TO_ROUTE_FULL(i, i);
    if(!e_route) { // && !xbt_dynar_length(e_route->generic_route.link_list)
      e_route = xbt_new0(s_route_extended_t,1);
      e_route->src_gateway = NULL;
      e_route->dst_gateway = NULL;
      e_route->generic_route.link_list = xbt_dynar_new(global_routing->size_of_link,NULL);
      xbt_dynar_push(e_route->generic_route.link_list,&global_routing->loopback);
      TO_ROUTE_FULL(i, i) = e_route;
    }
  }

  /* Shrink the dynar routes (save unused slots) */
  for (i=0;i<table_size;i++)
    for (j=0;j<table_size;j++)
      if(TO_ROUTE_FULL(i,j))
	xbt_dynar_shrink(TO_ROUTE_FULL(i,j)->generic_route.link_list,0);
}

/* ************************************************************************** */
/* *************************** FLOYD ROUTING ******************************** */

#define TO_FLOYD_COST(i,j) cost_table[(i)+(j)*table_size]
#define TO_FLOYD_PRED(i,j) (routing->predecessor_table)[(i)+(j)*table_size]
#define TO_FLOYD_LINK(i,j) (routing->link_table)[(i)+(j)*table_size]

/* Routing model structure */

typedef struct {
  s_routing_component_t generic_routing;
  /* vars for calculate the floyd algorith. */
  int *predecessor_table;
  route_extended_t *link_table; /* char* -> int* */
  xbt_dict_t to_index;
  /* store data during the parse process */  
  xbt_dict_t parse_routes; 
} s_routing_component_floyd_t,*routing_component_floyd_t;

/* Business methods */

static route_extended_t floyd_get_route(routing_component_t rc, const char* src,const char* dst) {
  
  xbt_assert1(src && dst, "Invalid params for \"get_route\" function at AS \"%s\"",rc->name);
  
  /* set utils vars */
  routing_component_floyd_t routing = (routing_component_floyd_t) rc;
  int table_size = xbt_dict_length(routing->to_index);
  
  route_extended_t new_e_route = xbt_new0(s_route_extended_t,1);
  new_e_route->generic_route.link_list = xbt_dynar_new(global_routing->size_of_link,NULL);
  new_e_route->src_gateway = NULL;
  new_e_route->dst_gateway = NULL;
  
  int *src_id = xbt_dict_get(routing->to_index,src);
  int *dst_id = xbt_dict_get(routing->to_index,dst);
  xbt_assert2(src_id && dst_id, "Ask for route \"from\"(%s)  or \"to\"(%s) no found in the local table",src,dst); 

  int first = 1;
  int pred = *dst_id;
  int prev_pred = 0;
  char *gw_src,*gw_dst, *prev_gw_src,*prev_gw_dst, *first_gw;
  unsigned int cpt;
  void* link;
  xbt_dynar_t links;
  route_extended_t e_route_as_to_as;
  do {
    prev_pred = pred;
    pred = TO_FLOYD_PRED(*src_id, pred);
    if(pred == -1) // if no pred in route -> no route to host
      break;      
    xbt_assert2(TO_FLOYD_LINK(pred,prev_pred),"Invalid link for the route between \"%s\" or \"%s\"", src, dst);
    
    prev_gw_src = gw_src;
    prev_gw_dst = gw_dst;
    
    route_extended_t e_route = TO_FLOYD_LINK(pred,prev_pred);
    gw_src = e_route->src_gateway;
    gw_dst = e_route->dst_gateway;
    
    if(first) first_gw = gw_src;
    
    if(rc->hierarchy == SURF_ROUTING_RECURSIVE && !first && strcmp(prev_gw_dst,gw_src)) {
      
      routing_component_t src_as = xbt_dict_get_or_null(global_routing->where_network_elements,prev_gw_dst);
      routing_component_t dst_as = xbt_dict_get_or_null(global_routing->where_network_elements,gw_src);
      xbt_assert2(src_as==dst_as,"bad routing, differents AS gateways in route \"%s\" to \"%s\"",src,dst);
      e_route_as_to_as = (*(src_as->get_route))(src_as,prev_gw_dst,gw_src);
      links = e_route_as_to_as->generic_route.link_list;
      xbt_dynar_foreach(links, cpt, link) {
        xbt_dynar_push(new_e_route->generic_route.link_list,&link);
      }
    }
    
    links = e_route->generic_route.link_list;
    xbt_dynar_foreach(links, cpt, link) {
      xbt_dynar_push(new_e_route->generic_route.link_list,&link);
    }
    first=0;
    
  } while(pred != *src_id);
  xbt_assert4(pred != -1, "no route from host %d to %d (\"%s\" to \"%s\")", *src_id, *dst_id,src,dst);
  
  if(rc->hierarchy == SURF_ROUTING_RECURSIVE) {
    new_e_route->src_gateway = first_gw;
    new_e_route->dst_gateway = gw_dst;
  }
  
  return new_e_route;
}

static void floyd_finalize(routing_component_t rc) {
   routing_component_floyd_t routing = (routing_component_floyd_t)rc;
  int i,j,table_size;
  if (routing) {
    table_size = xbt_dict_length(routing->to_index);
    /* Delete link_table */
    for (i=0;i<table_size;i++) {
      for (j=0;j<table_size;j++) {
        route_extended_t e_route = TO_FLOYD_LINK(i,j);
        if(e_route) {
          xbt_dynar_free(&(e_route->generic_route.link_list));
          if(e_route->src_gateway) xbt_free(e_route->src_gateway);
          if(e_route->dst_gateway) xbt_free(e_route->dst_gateway);
          xbt_free(e_route);
        }
      }
    }
    xbt_free(routing->link_table);
    /* Delete index dict */
    xbt_dict_free(&(routing->to_index));
    /* Delete dictionary index dict, predecessor and links table */
    xbt_free(routing->predecessor_table);
    /* Delete structure */
    xbt_free(rc);
  }
}

static void* model_floyd_create(void) {
  routing_component_floyd_t new_component = xbt_new0(s_routing_component_floyd_t,1);
  new_component->generic_routing.set_processing_units = generic_set_processing_units;
  new_component->generic_routing.set_autonomous_system = generic_set_autonomous_system;
  new_component->generic_routing.set_route = generic_set_route;
  new_component->generic_routing.set_ASroute = generic_set_ASroute;
  new_component->generic_routing.get_route = floyd_get_route;
  new_component->generic_routing.finalize = floyd_finalize;
  new_component->to_index = xbt_dict_new();
  new_component->parse_routes = xbt_dict_new();
  return new_component;
}

static void  model_floyd_load() {
 /* use "surfxml_add_callback" to add a parse function call */
}

static void  model_floyd_unload() {
 /* use "surfxml_del_callback" to remove a parse function call */
}

static void  model_floyd_end() {
  
  routing_component_floyd_t routing = ((routing_component_floyd_t)current_routing);
  xbt_dict_cursor_t cursor = NULL;
  double * cost_table;
  char *key,*data, *src_name, *dst_name;
  const char *sep = "#";
  xbt_dynar_t keys;
  int *src_id, *dst_id;
  unsigned int i,j,a,b,c;

  /* set the size of inicial table */
  int table_size = xbt_dict_length(routing->to_index);
  
  /* Create Cost, Predecessor and Link tables */
  cost_table = xbt_new0(double, table_size*table_size); //link cost from host to host
  routing->predecessor_table = xbt_new0(int, table_size*table_size); //predecessor host numbers
  routing->link_table = xbt_new0(route_extended_t, table_size*table_size); //actual link between src and dst

  /* Initialize costs and predecessors*/
  for(i = 0; i<table_size;i++)
    for(j = 0; j<table_size;j++) {
        TO_FLOYD_COST(i,j) = DBL_MAX;
        TO_FLOYD_PRED(i,j) = -1;
        TO_FLOYD_LINK(i,j) = NULL; // FIXED DAVID
    }

   /* Put the routes in position */
  xbt_dict_foreach(routing->parse_routes, cursor, key, data) {
    
    keys = xbt_str_split_str(key, sep);

    src_name = xbt_dynar_get_as(keys, 0, char*);
    dst_name = xbt_dynar_get_as(keys, 1, char*);
    
    src_id = xbt_dict_get_or_null(routing->to_index, src_name);
    dst_id = xbt_dict_get_or_null(routing->to_index, dst_name);
    
    if (src_id == NULL || dst_id == NULL )
      THROW2(mismatch_error,0,"Network elements %s or %s not found", src_name, dst_name);

    TO_FLOYD_LINK(*src_id,*dst_id) = generic_new_extended_route(current_routing,data);
    TO_FLOYD_PRED(*src_id,*dst_id) = *src_id;
    
    //link cost
    TO_FLOYD_COST(*src_id,*dst_id) = 1; // assume 1 for now
    
    xbt_dynar_free(&keys);
  }

  /* Add the loopback if needed */
  for (i = 0; i < table_size; i++) {
    route_extended_t e_route = TO_FLOYD_LINK(i, i);
    if(!e_route) { // && !xbt_dynar_length(e_route->generic_route.link_list)
      e_route = xbt_new0(s_route_extended_t,1);
      e_route->src_gateway = NULL;
      e_route->dst_gateway = NULL;
      e_route->generic_route.link_list = xbt_dynar_new(global_routing->size_of_link,NULL);
      xbt_dynar_push(e_route->generic_route.link_list,&global_routing->loopback);
      TO_FLOYD_LINK(i,i) = e_route;
      TO_FLOYD_PRED(i,i) = i;
      TO_FLOYD_COST(i,i) = 1;
    }
  }

  //Calculate path costs 
  for(c=0;c<table_size;c++) {
    for(a=0;a<table_size;a++) {
      for(b=0;b<table_size;b++) {
        if(TO_FLOYD_COST(a,c) < DBL_MAX && TO_FLOYD_COST(c,b) < DBL_MAX) {
          if(TO_FLOYD_COST(a,b) == DBL_MAX || 
            (TO_FLOYD_COST(a,c)+TO_FLOYD_COST(c,b) < TO_FLOYD_COST(a,b))) {
            TO_FLOYD_COST(a,b) = TO_FLOYD_COST(a,c)+TO_FLOYD_COST(c,b);
            TO_FLOYD_PRED(a,b) = TO_FLOYD_PRED(c,b);
          }
        }
      }
    }
  }

   /* delete the parse table */
  xbt_dict_foreach(routing->parse_routes, cursor, key, data) {
    route_t route = (route_t)data;
    xbt_dynar_free(&(route->link_list));
    xbt_free(data);
  }
  
  /* delete parse dict */
  xbt_dict_free(&(routing->parse_routes));

  //cleanup
  free(cost_table);
}

/* ************************************************************************** */
/* ********** Dijkstra & Dijkstra Cached ROUTING **************************** */

typedef struct {
  s_routing_component_t generic_routing;
  xbt_graph_t route_graph;
  xbt_dict_t graph_node_map;
  xbt_dict_t route_cache;
  xbt_dynar_t last_route;
  int cached;
} s_routing_component_dijkstra_t,*routing_component_dijkstra_t;

/* Parse routing model functions */

/* Business methods */

/* Creation routing model functions */

static void* model_dijkstra_create() {
  return NULL;
}

static void  model_dijkstra_load() {
}

static void  model_dijkstra_unload() {
}

static void  model_dijkstra_end() {
}

static void* model_dijkstracache_create() {
  return NULL;
}

static void  model_dijkstracache_load() {
}

static void  model_dijkstracache_unload() {
}

static void  model_dijkstracache_end() {
}


/* ************************************************************************** */
/* ******************************* NO ROUTING ******************************* */

/* Routing model structure */
typedef struct {
  s_routing_component_t generic_routing;
} s_routing_component_none_t,*routing_component_none_t;

/* Business methods */
static route_extended_t none_get_route(routing_component_t rc, const char* src,const char* dst){
  return NULL;
}
static void none_finalize(routing_component_t rc) {
  xbt_free(rc);
}

/* Creation routing model functions */
static void* model_none_create() {
  routing_component_none_t new_component =  xbt_new0(s_routing_component_none_t,1);
  new_component->generic_routing.set_processing_units = NULL;
  new_component->generic_routing.set_autonomous_system = NULL;
  new_component->generic_routing.set_route = NULL;
  new_component->generic_routing.set_ASroute = NULL;
  new_component->generic_routing.get_route = none_get_route;
  new_component->generic_routing.finalize = none_finalize;
  return new_component;
}

static void  model_none_load() {}
static void  model_none_unload() {}
static void  model_none_end() {}

/* ************************************************** */
/* ********** PATERN FOR NEW ROUTING **************** */

/* The minimal configuration of a new routing model need the next functions,
 * also you need to set at the start of the file, the new model in the model
 * list. Remember keep the null ending of the list.
 */
/* Routing model structure */
typedef struct {
  s_routing_component_t generic_routing;
  /* things that your routing model need */
} s_routing_component_NEW_t,*routing_component_NEW_t;

/* Parse routing model functions */
static void NEW_parse_S_host(void) {} /* example*/

/* Business methods */
static route_extended_t NEW_get_route(routing_component_t rc, const char* src,const char* dst) {return NULL;} /* mandatory */
static void NEW_finalize(routing_component_t rc) {} /* mandatory */

/* Creation routing model functions */
static void* model_NEW_create() {return NULL;} /* mandatory */
static void  model_NEW_load() {}   /* mandatory */
static void  model_NEW_unload() {} /* mandatory */
static void  model_NEW_end() {}    /* mandatory */

/* ************************************************************************** */
/* ************************* GENERIC PARSE FUNCTIONS ************************ */ 

static void generic_set_processing_units(routing_component_t rc, const char* name) {
   DEBUG1("Full - Load process unit \"%s\"",name);
   model_type_t modeltype = rc->routing;
   int *id = xbt_new0(int,1); // xbt_malloc(sizeof(int)); ?
   xbt_dict_t index;
   if(modeltype==&routing_models[SURF_MODEL_FULL])
     index = ((routing_component_full_t)current_routing)->to_index;
   else if(modeltype==&routing_models[SURF_MODEL_FLOYD])
     index = ((routing_component_floyd_t)current_routing)->to_index;   
   else xbt_die("\"generic_set_processing_units\" not support");
   *id = xbt_dict_length(index);
   xbt_dict_set(index,name,id,xbt_free);
}

static void generic_set_autonomous_system(routing_component_t rc, const char* name) {
   DEBUG1("Full - Load Autonomous system \"%s\"",name);
   model_type_t modeltype = rc->routing;
   int *id = xbt_new0(int,1); // xbt_malloc(sizeof(int)); ?
   xbt_dict_t index;
   if(modeltype==&routing_models[SURF_MODEL_FULL])
     index = ((routing_component_full_t)current_routing)->to_index;
   else if(modeltype==&routing_models[SURF_MODEL_FLOYD])
     index = ((routing_component_floyd_t)current_routing)->to_index;
   else xbt_die("\"generic_set_autonomous_system\" not support");
   *id = xbt_dict_length(index);
   xbt_dict_set(index,name,id,xbt_free);
}

static void generic_set_route(routing_component_t rc, const char* src, const char* dst, route_t route) {
  DEBUG2("Full - Load Route from \"%s\" to \"%s\"",src,dst);
  model_type_t modeltype = rc->routing;
  xbt_dict_t parseroutes;
  if(modeltype==&routing_models[SURF_MODEL_FULL])
    parseroutes = ((routing_component_full_t)current_routing)->parse_routes;
  else if(modeltype==&routing_models[SURF_MODEL_FLOYD])
    parseroutes = ((routing_component_floyd_t)current_routing)->parse_routes;
  else xbt_die("\"generic_set_autonomous_system\" not support");
  char *route_name;
  route_name = bprintf("%s#%s",src,dst);
  xbt_assert2(xbt_dynar_length(link_list)>0, "Invalid count of links, must be greater than zero (%s,%s)",src,dst);   
  xbt_assert2(!xbt_dict_get_or_null(parseroutes,route_name),
    "The route between \"%s\" and \"%s\" already exist",src,dst);
  xbt_dict_set(parseroutes, route_name, route, NULL);
  free(route_name);
}

static void generic_set_ASroute(routing_component_t rc, const char* src, const char* dst, route_extended_t e_route) {
  DEBUG4("Full - Load ASroute from \"%s(%s)\" to \"%s(%s)\"",src,e_route->src_gateway,dst,e_route->dst_gateway);
  model_type_t modeltype = rc->routing;
  xbt_dict_t parseroutes;
  if(modeltype==&routing_models[SURF_MODEL_FULL])
    parseroutes = ((routing_component_full_t)current_routing)->parse_routes;
  else if(modeltype==&routing_models[SURF_MODEL_FLOYD])
    parseroutes = ((routing_component_floyd_t)current_routing)->parse_routes;
  else xbt_die("\"generic_set_autonomous_system\" not support");
  char *route_name;
  route_name = bprintf("%s#%s",src,dst);
  xbt_assert2(xbt_dynar_length(link_list)>0, "Invalid count of links, must be greater than zero (%s,%s)",src,dst);   
  xbt_assert4(!xbt_dict_get_or_null(parseroutes,route_name),
    "The route between \"%s\"(\"%s\") and \"%s\"(\"%s\") already exist",src,e_route->src_gateway,dst,e_route->dst_gateway);
  xbt_dict_set(parseroutes, route_name, e_route, NULL);
  free(route_name);
}

/* ************************************************************************** */
/* ************************* GENERIC AUX FUNCTIONS ************************** */

static route_extended_t generic_new_extended_route(routing_component_t rc, void* data) {
  
  char *link_name;
  route_extended_t e_route, new_e_route;
  route_t route;
  unsigned int cpt;
  xbt_dynar_t links, links_id;
  
  new_e_route = xbt_new0(s_route_extended_t,1);
  new_e_route->generic_route.link_list = xbt_dynar_new(global_routing->size_of_link,NULL);
  new_e_route->src_gateway = NULL;
  new_e_route->dst_gateway = NULL;

  xbt_assert0(rc->hierarchy == SURF_ROUTING_BASE || rc->hierarchy == SURF_ROUTING_RECURSIVE,
      "the hierarchy type is not defined");
  
  if(rc->hierarchy == SURF_ROUTING_BASE ) {
    
    route = (route_t)data;
    links = route->link_list;
    
  } else if(rc->hierarchy == SURF_ROUTING_RECURSIVE ) {

    e_route = (route_extended_t)data;
    
    // FIXME: HERE CHECK THE GATEWAY!!!!!!!!!
    xbt_assert0(e_route->src_gateway&&e_route->dst_gateway,"bad gateway, is null");
    
    links = e_route->generic_route.link_list;
    
    /* remeber not erase the gateway names */
    new_e_route->src_gateway = e_route->src_gateway;
    new_e_route->dst_gateway = e_route->dst_gateway;
  }
  
  links_id = new_e_route->generic_route.link_list;
  
  xbt_dynar_foreach(links, cpt, link_name) {
    
    void* link = xbt_dict_get_or_null(surf_network_model->resource_set, link_name);
    if (link)
      xbt_dynar_push(links_id,&link);
    else
      THROW1(mismatch_error,0,"Link %s not found", link_name);
  }
 
  return new_e_route;
}

////////////////////////////////////////////////////////////////////////////////
// HERE FINISH THE NEW CODE
////////////////////////////////////////////////////////////////////////////////

//...... DEBUG ONLY .... //
static void print_tree(routing_component_t rc) {
  printf("(%s %s)\n",rc->name,rc->routing->name);
  printf("  ");
  xbt_dict_cursor_t cursor = NULL;
  char *key;
  int *val;  
  if( rc->routing == &(routing_models[SURF_MODEL_FULL]) )
  { xbt_dict_foreach(((routing_component_full_t)rc)->to_index, cursor, key, val) { printf("<%s-%d> ",key,*val); } }
  else if( rc->routing == &(routing_models[SURF_MODEL_FLOYD]) )
  { xbt_dict_foreach(((routing_component_floyd_t)rc)->to_index, cursor, key, val) { printf("<%s-%d> ",key,*val); } 
  }
  else
      xbt_assert0(0, "Invalid model for call \"print_tree\"");
  printf("\n");  
  routing_component_t elem;
  xbt_dict_foreach(rc->routing_sons, cursor, key, elem) {
    printf("<--\n");  
    print_tree(elem);
    printf("-->\n");
  }
}

//...... DEBUG ONLY .... //
static void print_global() {
  xbt_dict_cursor_t cursor = NULL;
  char *key;
  routing_component_t elem;  
  xbt_dict_foreach(global_routing->where_network_elements, cursor, key, elem) { printf("<%s>\n",key); }
}

//...... DEBUG ONLY .... //
static void print_AS_start(void) { printf("AS!!! %s y se rutea \"%s\"\n",A_surfxml_AS_id,A_surfxml_AS_routing); }
static void print_AS_end(void)   { printf("AS!!! %s\n",A_surfxml_AS_id); }
static void print_host(void)     { printf("host!!! %s\n",A_surfxml_host_id); }
static void print_link(void)     { printf("link!!! %s\n",A_surfxml_link_id); }
static void print_route(void)    { printf("route!!! %s a %s\n",A_surfxml_route_src,A_surfxml_route_dst); }
static void print_ctn(void)      { printf("ctn!!! %s\n",A_surfxml_link_c_ctn_id); }

//...... DEBUG ONLY .... //
static void DEBUG_exit(void) {
  
  printf("-------- print tree elements -----\n");
  print_tree(global_routing->root);
  printf("----------------------------------\n\n");
  
  printf("-------- network_elements --------\n");
  print_global();
  printf("----------------------------------\n\n");
  
//   char* names[] = { "11.A","11.B","11.C","121.A","121.B","122.A","13.A","13.B"};
//   int i,j,total = 8;

//   printf("-- print all the example routes --\n");
//   char* names[] = { "11.A","11.B","11.C"};
//   int i,j,total = 3;

//   char* names[] = { "141.A","141.B","143.A","143.B"};
//   int i,j,total = 4;

//   char* names[] = { "142.A","142.B","142.C" };
//   int i,j,total = 3;

//   const char* names[] = { "142.A","142.B","142.C", "11.A","11.B","11.C","141.A","141.B","143.A","143.B","11.A","11.B","11.C","121.A","121.B","122.A","13.A","13.B"};
//   int i,j,total = 3+4+8+3;

   const char* names[] = { "11.A","11.B","11.C",
                           "121.A","121.B",
                           "122.A",
                           "13.A","13.B",
                           "141.A","141.B",
                           "142.A","142.B","142.C",
                           "143.A","143.B",
                           "15.A","15.B","15.C"};
   unsigned int i,j,total = 3+2+1+2+2+3+2+3;

  printf("-- print all the example routes --\n");
  xbt_dynar_t links;
  void* link;
  for(i=0;i<total;i++) {
    for(j=0;j<total;j++) {
      printf("route from %s to %s >>> ",names[i],names[j]);
      links = (*(global_routing->get_route))(names[i],names[j]);
      unsigned int cpt=0;
      xbt_dynar_foreach(links, cpt, link) {
        s_surf_resource_t* generic_resource = link;
        printf(" %s",generic_resource->name);
      }
      printf("\n");
    }
  }
  
  printf("----------------------------------\n\n");
  
  printf("---------- call finalize ---------\n");
  (*(global_routing->finalize))();
  printf("----------------------------------\n");
  
  exit(0); 
}

////////////////////////////////////////////////////////////////////////////////
// HERE END THE NEW CODE
////////////////////////////////////////////////////////////////////////////////

// /* ************************************************************************** */
// /* *************************** FULL ROUTING ********************************* */
// typedef struct {
//   s_routing_t generic_routing;
//   xbt_dynar_t *routing_table;
//   void *loopback;
//   size_t size_of_link;
// } s_routing_full_t,*routing_full_t;
// 
// #define ROUTE_FULL(i,j) ((routing_full_t)used_routing)->routing_table[(i)+(j)*(used_routing)->host_count]
// #define HOST2ROUTER(id) ((id)+(2<<29))
// #define ROUTER2HOST(id) ((id)-(2>>29))
// #define ISROUTER(id) ((id)>=(2<<29))
// 
// /*
//  * Free the onelink routes
//  */
// static void onelink_route_elem_free(void *e) {
//   s_onelink_t tmp = (s_onelink_t)e;
//   if(tmp) {
//     free(tmp);
//   }
// }
// 
// /*
//  * Parsing
//  */
// static void routing_full_parse_Shost(void) {
//   int *val = xbt_malloc(sizeof(int));
//   DEBUG2("Seen host %s (#%d)",A_surfxml_host_id,used_routing->host_count);
//   *val = used_routing->host_count++;
//   xbt_dict_set(used_routing->host_id,A_surfxml_host_id,val,xbt_free);
// #ifdef HAVE_TRACING
//   TRACE_surf_host_define_id (A_surfxml_host_id, *val);
// #endif
// }
// 
// static void routing_full_parse_Srouter(void) {
// 	int *val = xbt_malloc(sizeof(int));
//   DEBUG3("Seen router %s (%d -> #%d)",A_surfxml_router_id,used_routing->router_count,
//   		HOST2ROUTER(used_routing->router_count));
//   *val = HOST2ROUTER(used_routing->router_count++);
//   xbt_dict_set(used_routing->host_id,A_surfxml_router_id,val,xbt_free);
// #ifdef HAVE_TRACING
//   TRACE_surf_host_define_id (A_surfxml_host_id, *val);
//   TRACE_surf_host_declaration (A_surfxml_host_id, 0);
// #endif
// }
// 
// static int src_id = -1;
// static int dst_id = -1;
// static void routing_full_parse_Sroute_set_endpoints(void)
// {
//   src_id = *(int*)xbt_dict_get(used_routing->host_id,A_surfxml_route_src);
//   dst_id = *(int*)xbt_dict_get(used_routing->host_id,A_surfxml_route_dst);
//   DEBUG4("Route %s %d -> %s %d",A_surfxml_route_src,src_id,A_surfxml_route_dst,dst_id);
//   route_action = A_surfxml_route_action;
// }
// 
// static void routing_full_parse_Eroute(void)
// {
//   char *name;
//   if (src_id != -1 && dst_id != -1) {
//     name = bprintf("%x#%x", src_id, dst_id);
//     manage_route(route_table, name, route_action, 0);
//     free(name);
//   }
// }
// 
// /* Cluster tag functions */
// 
// static void routing_full_parse_change_cpu_data(const char *hostName,
//                                   const char *surfxml_host_power,
//                                   const char *surfxml_host_availability,
//                                   const char *surfxml_host_availability_file,
//                                   const char *surfxml_host_state_file)
// {
//   int AX_ptr = 0;
// 
//   SURFXML_BUFFER_SET(host_id, hostName);
//   SURFXML_BUFFER_SET(host_power, surfxml_host_power /*hostPower */ );
//   SURFXML_BUFFER_SET(host_availability, surfxml_host_availability);
//   SURFXML_BUFFER_SET(host_availability_file, surfxml_host_availability_file);
//   SURFXML_BUFFER_SET(host_state_file, surfxml_host_state_file);
// }
// 
// static void routing_full_parse_change_link_data(const char *linkName,
//                                    const char *surfxml_link_bandwidth,
//                                    const char *surfxml_link_bandwidth_file,
//                                    const char *surfxml_link_latency,
//                                    const char *surfxml_link_latency_file,
//                                    const char *surfxml_link_state_file)
// {
//   int AX_ptr = 0;
// 
//   SURFXML_BUFFER_SET(link_id, linkName);
//   SURFXML_BUFFER_SET(link_bandwidth, surfxml_link_bandwidth);
//   SURFXML_BUFFER_SET(link_bandwidth_file, surfxml_link_bandwidth_file);
//   SURFXML_BUFFER_SET(link_latency, surfxml_link_latency);
//   SURFXML_BUFFER_SET(link_latency_file, surfxml_link_latency_file);
//   SURFXML_BUFFER_SET(link_state_file, surfxml_link_state_file);
// }
// 
// static void routing_full_parse_Scluster(void)
// {
//   static int AX_ptr = 0;
// 
//   char *cluster_id = A_surfxml_cluster_id;
//   char *cluster_prefix = A_surfxml_cluster_prefix;
//   char *cluster_suffix = A_surfxml_cluster_suffix;
//   char *cluster_radical = A_surfxml_cluster_radical;
//   char *cluster_power = A_surfxml_cluster_power;
//   char *cluster_bw = A_surfxml_cluster_bw;
//   char *cluster_lat = A_surfxml_cluster_lat;
//   char *cluster_bb_bw = A_surfxml_cluster_bb_bw;
//   char *cluster_bb_lat = A_surfxml_cluster_bb_lat;
//   char *backbone_name;
//   unsigned int it1,it2;
//   char *name1,*name2;
//   xbt_dynar_t names = NULL;
//   surfxml_bufferstack_push(1);
// 
//   /* Make set a set to parse the prefix/suffix/radical into a neat list of names */
//   DEBUG4("Make <set id='%s' prefix='%s' suffix='%s' radical='%s'>",
//       cluster_id,cluster_prefix,cluster_suffix,cluster_radical);
//   SURFXML_BUFFER_SET(set_id, cluster_id);
//   SURFXML_BUFFER_SET(set_prefix, cluster_prefix);
//   SURFXML_BUFFER_SET(set_suffix, cluster_suffix);
//   SURFXML_BUFFER_SET(set_radical, cluster_radical);
// 
//   SURFXML_START_TAG(set);
//   SURFXML_END_TAG(set);
// 
//   names = xbt_dict_get(set_list,cluster_id);
// 
//   xbt_dynar_foreach(names,it1,name1) {
//     /* create the host */
//     routing_full_parse_change_cpu_data(name1, cluster_power, "1.0", "", "");
//     A_surfxml_host_state = A_surfxml_host_state_ON;
// 
//     SURFXML_START_TAG(host);
//     SURFXML_END_TAG(host);
// 
//     /* Here comes the link */
//     routing_full_parse_change_link_data(name1, cluster_bw, "", cluster_lat, "", "");
//     A_surfxml_link_state = A_surfxml_link_state_ON;
//     A_surfxml_link_sharing_policy = A_surfxml_link_sharing_policy_SHARED;
// 
//     SURFXML_START_TAG(link);
//     SURFXML_END_TAG(link);
//   }
// 
//   /* Make backbone link */
//   backbone_name = bprintf("%s_bb", cluster_id);
//   routing_full_parse_change_link_data(backbone_name, cluster_bb_bw, "", cluster_bb_lat, "",
//                          "");
//   A_surfxml_link_state = A_surfxml_link_state_ON;
//   A_surfxml_link_sharing_policy = A_surfxml_link_sharing_policy_FATPIPE;
// 
//   SURFXML_START_TAG(link);
//   SURFXML_END_TAG(link);
// 
//   /* And now the internal routes */
//   xbt_dynar_foreach(names,it1,name1) {
//     xbt_dynar_foreach(names,it2,name2) {
//       if (strcmp(name1,name2)) {
//         A_surfxml_route_action = A_surfxml_route_action_POSTPEND;
//         SURFXML_BUFFER_SET(route_src,name1);
//         SURFXML_BUFFER_SET(route_dst,name2);
//         SURFXML_START_TAG(route); {
//           /* FIXME: name1 link is added by error about 20 lines below, so don't add it here
//           SURFXML_BUFFER_SET(link_c_ctn_id, name1);
//           SURFXML_START_TAG(link_c_ctn);
//           SURFXML_END_TAG(link_c_ctn);
//            */
//           SURFXML_BUFFER_SET(link_c_ctn_id, backbone_name);
//           SURFXML_START_TAG(link_c_ctn);
//           SURFXML_END_TAG(link_c_ctn);
// 
//           SURFXML_BUFFER_SET(link_c_ctn_id, name2);
//           SURFXML_START_TAG(link_c_ctn);
//           SURFXML_END_TAG(link_c_ctn);
// 
//         } SURFXML_END_TAG(route);
//       }
//     }
//   }
// 
//   /* Make route multi with the outside world, i.e. cluster->$* */
// 
//   /* FIXME
//    * This also adds an elements to the routes within the cluster,
//    * and I guess it's wrong, but since this element is commented out in the above
//    * code creating the internal routes, we're good.
//    * To fix it, I'd say that we need a way to understand "$*-${cluster_id}" as "whole world, but the guys in that cluster"
//    * But for that, we need to install a real expression parser for src/dst attributes
//    *
//    * FIXME
//    * This also adds a dumb element (the private link) in place of the loopback. Then, since
//    * the loopback is added only if no link to self already exist, this fails.
//    * That's really dumb.
//    *
//    * FIXME
//    * It seems to me that it does not add the backbone to the path to outside world...
//    */
//   SURFXML_BUFFER_SET(route_c_multi_src, cluster_id);
//   SURFXML_BUFFER_SET(route_c_multi_dst, "$*");
//   A_surfxml_route_c_multi_symmetric = A_surfxml_route_c_multi_symmetric_NO;
//   A_surfxml_route_c_multi_action = A_surfxml_route_c_multi_action_PREPEND;
// 
//   SURFXML_START_TAG(route_c_multi);
// 
//   SURFXML_BUFFER_SET(link_c_ctn_id, "$src");
// 
//   SURFXML_START_TAG(link_c_ctn);
//   SURFXML_END_TAG(link_c_ctn);
// 
//   SURFXML_END_TAG(route_c_multi);
// 
//   free(backbone_name);
// 
//   /* Restore buff */
//   surfxml_bufferstack_pop(1);
// }
// 
// 
// static void routing_full_parse_end(void) {
//   routing_full_t routing = (routing_full_t) used_routing;
//   int nb_link = 0;
//   unsigned int cpt = 0;
//   xbt_dict_cursor_t cursor = NULL;
//   char *key, *data, *end;
//   const char *sep = "#";
//   xbt_dynar_t links, keys;
//   char *link_name = NULL;
//   int i,j;
// 
//   int host_count = routing->generic_routing.host_count;
// 
//   /* Create the routing table */
//   routing->routing_table = xbt_new0(xbt_dynar_t, host_count * host_count);
//   for (i=0;i<host_count;i++)
//     for (j=0;j<host_count;j++)
//       ROUTE_FULL(i,j) = xbt_dynar_new(routing->size_of_link,NULL);
// 
//   /* Put the routes in position */
//   xbt_dict_foreach(route_table, cursor, key, data) {
//     nb_link = 0;
//     links = (xbt_dynar_t) data;
//     keys = xbt_str_split_str(key, sep);
// 
//     src_id = strtol(xbt_dynar_get_as(keys, 0, char *), &end, 16);
//     dst_id = strtol(xbt_dynar_get_as(keys, 1, char *), &end, 16);
//     xbt_dynar_free(&keys);
// 
//     if(xbt_dynar_length(links) == 1){
//       s_onelink_t new_link = (s_onelink_t) xbt_malloc0(sizeof(s_onelink));
//       new_link->src_id = src_id;
//       new_link->dst_id = dst_id;
//       link_name = xbt_dynar_getfirst_as(links, char*);
//       new_link->link_ptr = xbt_dict_get_or_null(surf_network_model->resource_set, link_name);
//       DEBUG3("Adding onelink route from (#%d) to (#%d), link_name %s",src_id, dst_id, link_name);
//       xbt_dict_set(onelink_routes, link_name, (void *)new_link, onelink_route_elem_free);
// #ifdef HAVE_TRACING
//       TRACE_surf_link_save_endpoints (link_name, src_id, dst_id);
// #endif
//     }
// 
//     if(ISROUTER(src_id) || ISROUTER(dst_id)) {
// 				DEBUG2("There is route with a router here: (%d ,%d)",src_id,dst_id);
// 				/* Check there is only one link in the route and store the information */
// 				continue;
//     }
// 
//     DEBUG4("Handle %d %d (from %d hosts): %ld links",
//         src_id,dst_id,routing->generic_routing.host_count,xbt_dynar_length(links));
//     xbt_dynar_foreach(links, cpt, link_name) {
//       void* link = xbt_dict_get_or_null(surf_network_model->resource_set, link_name);
//       if (link)
//         xbt_dynar_push(ROUTE_FULL(src_id,dst_id),&link);
//       else
//         THROW1(mismatch_error,0,"Link %s not found", link_name);
//     }
//   }
// 
//   /* Add the loopback if needed */
//   for (i = 0; i < host_count; i++)
//     if (!xbt_dynar_length(ROUTE_FULL(i, i)))
//       xbt_dynar_push(ROUTE_FULL(i,i),&routing->loopback);
// 
//   /* Shrink the dynar routes (save unused slots) */
//   for (i=0;i<host_count;i++)
//     for (j=0;j<host_count;j++)
//       xbt_dynar_shrink(ROUTE_FULL(i,j),0);
// }
// 
// /*
//  * Business methods
//  */
// static xbt_dynar_t routing_full_get_route(int src,int dst) {
//   xbt_assert0(!(ISROUTER(src) || ISROUTER(dst)), "Ask for route \"from\" or \"to\" a router node");
//   return ROUTE_FULL(src,dst);
// }
// 
// static xbt_dict_t routing_full_get_onelink_routes(void){
//   return onelink_routes;
// }
// 
// static int routing_full_is_router(int id){
// 	return ISROUTER(id);
// }
// 
// static void routing_full_finalize(void) {
//   routing_full_t routing = (routing_full_t)used_routing;
//   int i,j;
// 
//   if (routing) {
//     for (i = 0; i < used_routing->host_count; i++)
//       for (j = 0; j < used_routing->host_count; j++)
//         xbt_dynar_free(&ROUTE_FULL(i, j));
//     free(routing->routing_table);
//     xbt_dict_free(&used_routing->host_id);
//     xbt_dict_free(&onelink_routes);
//     free(routing);
//     routing=NULL;
//   }
// }
// 
// static void routing_model_full_create(size_t size_of_link,void *loopback) {
//   /* initialize our structure */
//   routing_full_t routing = xbt_new0(s_routing_full_t,1);
//   routing->generic_routing.name = "Full";
//   routing->generic_routing.host_count = 0;
//   routing->generic_routing.get_route = routing_full_get_route;
//   routing->generic_routing.get_onelink_routes = routing_full_get_onelink_routes;
//   routing->generic_routing.is_router = routing_full_is_router;
//   routing->generic_routing.finalize = routing_full_finalize;
// 
//   routing->size_of_link = size_of_link;
//   routing->loopback = loopback;
// 
//   /* Set it in position */
//   used_routing = (routing_t) routing;
// 
//   /* Set the dict for onehop routes */
//   onelink_routes =  xbt_dict_new();
// 
//   routing->generic_routing.host_id = xbt_dict_new();
//   
//   /* Setup the parsing callbacks we need */
// //   surfxml_add_callback(STag_surfxml_host_cb_list, &routing_full_parse_Shost);
// //   surfxml_add_callback(STag_surfxml_router_cb_list, &routing_full_parse_Srouter);
// //   surfxml_add_callback(ETag_surfxml_platform_cb_list, &routing_full_parse_end);
// //   surfxml_add_callback(STag_surfxml_route_cb_list, &routing_full_parse_Sroute_set_endpoints);
// //   surfxml_add_callback(ETag_surfxml_route_cb_list, &routing_full_parse_Eroute);
// //   surfxml_add_callback(STag_surfxml_cluster_cb_list, &routing_full_parse_Scluster);
// 
// //   surfxml_add_callback(STag_surfxml_host_cb_list, &routing_full_parse_Shost);
// //   surfxml_add_callback(STag_surfxml_router_cb_list, &routing_full_parse_Srouter);
//   
// }

/* ************************************************************************** */

// static void routing_shortest_path_parse_Scluster(void)
// {
//   static int AX_ptr = 0;
// 
//   char *cluster_id = A_surfxml_cluster_id;
//   char *cluster_prefix = A_surfxml_cluster_prefix;
//   char *cluster_suffix = A_surfxml_cluster_suffix;
//   char *cluster_radical = A_surfxml_cluster_radical;
//   char *cluster_power = A_surfxml_cluster_power;
//   char *cluster_bb_bw = A_surfxml_cluster_bb_bw;
//   char *cluster_bb_lat = A_surfxml_cluster_bb_lat;
//   char *backbone_name;
// 
//   surfxml_bufferstack_push(1);
// 
//   /* Make set */
//   SURFXML_BUFFER_SET(set_id, cluster_id);
//   SURFXML_BUFFER_SET(set_prefix, cluster_prefix);
//   SURFXML_BUFFER_SET(set_suffix, cluster_suffix);
//   SURFXML_BUFFER_SET(set_radical, cluster_radical);
// 
//   SURFXML_START_TAG(set);
//   SURFXML_END_TAG(set);
// 
//   /* Make foreach */
//   SURFXML_BUFFER_SET(foreach_set_id, cluster_id);
// 
//   SURFXML_START_TAG(foreach);
// 
//   /* Make host for the foreach */
//   routing_full_parse_change_cpu_data("$1", cluster_power, "1.0", "", "");
//   A_surfxml_host_state = A_surfxml_host_state_ON;
// 
//   SURFXML_START_TAG(host);
//   SURFXML_END_TAG(host);
// 
//   SURFXML_END_TAG(foreach);
// 
//   /* Make backbone link */
//   backbone_name = bprintf("%s_bb", cluster_id);
//   routing_full_parse_change_link_data(backbone_name, cluster_bb_bw, "", cluster_bb_lat, "",
//                          "");
//   A_surfxml_link_state = A_surfxml_link_state_ON;
//   A_surfxml_link_sharing_policy = A_surfxml_link_sharing_policy_FATPIPE;
// 
//   SURFXML_START_TAG(link);
//   SURFXML_END_TAG(link);
// 
//   free(backbone_name);
// 
//   /* Restore buff */
//   surfxml_bufferstack_pop(1);
// }
// 
// /* ************************************************************************** */
// /* *************************** FLOYD ROUTING ********************************* */
// typedef struct {
//   s_routing_t generic_routing;
//   int *predecessor_table;
//   void** link_table;
//   xbt_dynar_t last_route;
//   void *loopback;
//   size_t size_of_link;
// } s_routing_floyd_t,*routing_floyd_t;
// 
// #define FLOYD_COST(i,j) cost_table[(i)+(j)*(used_routing)->host_count]
// #define FLOYD_PRED(i,j) ((routing_floyd_t)used_routing)->predecessor_table[(i)+(j)*(used_routing)->host_count]
// #define FLOYD_LINK(i,j) ((routing_floyd_t)used_routing)->link_table[(i)+(j)*(used_routing)->host_count]
// 
// static void routing_floyd_parse_end(void) {
// 
//   routing_floyd_t routing = (routing_floyd_t) used_routing;
//   int nb_link = 0;
//   void* link_list = NULL;
//   double * cost_table;
//   xbt_dict_cursor_t cursor = NULL;
//   char *key,*data, *end;
//   const char *sep = "#";
//   xbt_dynar_t links, keys;
// 
//   unsigned int i,j;
//   unsigned int a,b,c;
//   int host_count = routing->generic_routing.host_count;
//   char * link_name = NULL;
//   void * link = NULL;
// 
//   /* Create Cost, Predecessor and Link tables */
//   cost_table = xbt_new0(double, host_count * host_count); //link cost from host to host
//   routing->predecessor_table = xbt_new0(int, host_count*host_count); //predecessor host numbers
//   routing->link_table = xbt_new0(void*,host_count*host_count); //actual link between src and dst
//   routing->last_route = xbt_dynar_new(routing->size_of_link, NULL);
// 
//   /* Initialize costs and predecessors*/
//   for(i = 0; i<host_count;i++)
//     for(j = 0; j<host_count;j++) {
//         FLOYD_COST(i,j) = DBL_MAX;
//         FLOYD_PRED(i,j) = -1;
//     }
// 
//    /* Put the routes in position */
//   xbt_dict_foreach(route_table, cursor, key, data) {
//     nb_link = 0;
//     links = (xbt_dynar_t)data;
//     keys = xbt_str_split_str(key, sep);
// 
//     
//     src_id = strtol(xbt_dynar_get_as(keys, 0, char*), &end, 16);
//     dst_id = strtol(xbt_dynar_get_as(keys, 1, char*), &end, 16);
//     xbt_dynar_free(&keys);
//  
//     DEBUG4("Handle %d %d (from %d hosts): %ld links",
//         src_id,dst_id,routing->generic_routing.host_count,xbt_dynar_length(links));
//     xbt_assert3(xbt_dynar_length(links) == 1, "%ld links in route between host %d and %d, should be 1", xbt_dynar_length(links), src_id, dst_id);
//     
//     link_name = xbt_dynar_getfirst_as(links, char*);
//     link = xbt_dict_get_or_null(surf_network_model->resource_set, link_name);
// 
//     if (link)
//       link_list = link;
//     else
//       THROW1(mismatch_error,0,"Link %s not found", link_name);
//     
// 
//     FLOYD_LINK(src_id,dst_id) = link_list;
//     FLOYD_PRED(src_id, dst_id) = src_id;
// 
//     //link cost
//     FLOYD_COST(src_id, dst_id) = 1; // assume 1 for now
// 
//   }
// 
//     /* Add the loopback if needed */
//   for (i = 0; i < host_count; i++)
//     if (!FLOYD_PRED(i, i)) {
//       FLOYD_PRED(i, i) = i;
//       FLOYD_COST(i, i) = 1;
//       FLOYD_LINK(i, i) = routing->loopback;
//     }
// 
// 
//   //Calculate path costs 
// 
//   for(c=0;c<host_count;c++) {
//     for(a=0;a<host_count;a++) {
//       for(b=0;b<host_count;b++) {
//         if(FLOYD_COST(a,c) < DBL_MAX && FLOYD_COST(c,b) < DBL_MAX) {
//           if(FLOYD_COST(a,b) == DBL_MAX || (FLOYD_COST(a,c)+FLOYD_COST(c,b) < FLOYD_COST(a,b))) {
//             FLOYD_COST(a,b) = FLOYD_COST(a,c)+FLOYD_COST(c,b);
//             FLOYD_PRED(a,b) = FLOYD_PRED(c,b);
//           }
//         }
//       }
//     }
//   }
// 
//   //cleanup
//   free(cost_table);
// }
// 
// /*
//  * Business methods
//  */
// static xbt_dynar_t routing_floyd_get_route(int src_id,int dst_id) {
// 
//   routing_floyd_t routing = (routing_floyd_t) used_routing;
// 
//   int pred = dst_id;
//   int prev_pred = 0;
// 
//   xbt_dynar_reset(routing->last_route);
// 
//   do {
//     prev_pred = pred;
//     pred = FLOYD_PRED(src_id, pred);
// 
//     if(pred == -1) // if no pred in route -> no route to host
//         break;
// 
//     xbt_dynar_unshift(routing->last_route, &FLOYD_LINK(pred,prev_pred));
// 
//   } while(pred != src_id);
// 
//   xbt_assert2(pred != -1, "no route from host %d to %d", src_id, dst_id);
// 
//   return routing->last_route;
// }
// 
// static void routing_floyd_finalize(void) {
//   routing_floyd_t routing = (routing_floyd_t)used_routing;
// 
//   if (routing) {
//     free(routing->link_table);
//     free(routing->predecessor_table);
//     xbt_dynar_free(&routing->last_route);
//     xbt_dict_free(&used_routing->host_id);
//     free(routing);
//     routing=NULL;
//   }
// }
// 
// static xbt_dict_t routing_floyd_get_onelink_routes(void){
//   xbt_assert0(0,"The get_onelink_routes feature is not supported in routing model Floyd");
// }
// 
// static int routing_floyd_is_router(int id){
//   xbt_assert0(0,"The get_is_router feature is not supported in routing model Floyd");
// }
// 
// static void routing_model_floyd_create(size_t size_of_link,void *loopback) {
//   /* initialize our structure */
//   routing_floyd_t routing = xbt_new0(s_routing_floyd_t,1);
//   routing->generic_routing.name = "Floyd";
//   routing->generic_routing.host_count = 0;
//   routing->generic_routing.host_id = xbt_dict_new();
//   routing->generic_routing.get_route = routing_floyd_get_route;
//   routing->generic_routing.get_onelink_routes = routing_floyd_get_onelink_routes;
//   routing->generic_routing.is_router = routing_floyd_is_router;
//   routing->generic_routing.finalize = routing_floyd_finalize;
//   routing->size_of_link = size_of_link;
//   routing->loopback = loopback;
// 
//   /* Set it in position */
//   used_routing = (routing_t) routing;
//   
//   /* Setup the parsing callbacks we need */
//   surfxml_add_callback(STag_surfxml_host_cb_list, &routing_full_parse_Shost);
//   surfxml_add_callback(ETag_surfxml_platform_cb_list, &routing_floyd_parse_end);
//   surfxml_add_callback(STag_surfxml_route_cb_list, 
//       &routing_full_parse_Sroute_set_endpoints);
//   surfxml_add_callback(ETag_surfxml_route_cb_list, &routing_full_parse_Eroute);
//   surfxml_add_callback(STag_surfxml_cluster_cb_list, &routing_shortest_path_parse_Scluster);
//   
// }
// 
// /* ************************************************************************** */
// /* ********** Dijkstra & Dijkstra Cached ROUTING **************************** */
// typedef struct {
//   s_routing_t generic_routing;
//   xbt_graph_t route_graph;
//   xbt_dict_t graph_node_map;
//   xbt_dict_t route_cache;
//   xbt_dynar_t last_route;
//   int cached;
//   void *loopback;
//   size_t size_of_link;
// } s_routing_dijkstra_t,*routing_dijkstra_t;
// 
// 
// typedef struct graph_node_data {
//   int id; 
//   int graph_id; //used for caching internal graph id's
// } s_graph_node_data_t, * graph_node_data_t;
// 
// typedef struct graph_node_map_element {
//   xbt_node_t node;
// } s_graph_node_map_element_t, * graph_node_map_element_t;
// 
// typedef struct route_cache_element {
//   int * pred_arr;
//   int size;
// } s_route_cache_element_t, * route_cache_element_t;	
// 
// /*
//  * Free functions
//  */
// static void route_cache_elem_free(void *e) {
//   route_cache_element_t elm=(route_cache_element_t)e;
// 
//   if (elm) {
//     free(elm->pred_arr);
//     free(elm);
//   }
// }
// 
// static void graph_node_map_elem_free(void *e) {
//   graph_node_map_element_t elm = (graph_node_map_element_t)e;
// 
//   if(elm) {
//     free(elm);
//   }
// }
// 
// /*
//  * Utility functions
// */
// static xbt_node_t route_graph_new_node(int id, int graph_id) {
//   xbt_node_t node = NULL;
//   graph_node_data_t data = NULL;
//   graph_node_map_element_t elm = NULL;
//   routing_dijkstra_t routing = (routing_dijkstra_t) used_routing;
// 
//   data = xbt_new0(struct graph_node_data, sizeof(struct graph_node_data));
//   data->id = id;
//   data->graph_id = graph_id;
//   node = xbt_graph_new_node(routing->route_graph, data);
// 
//   elm = xbt_new0(struct graph_node_map_element, sizeof(struct graph_node_map_element));
//   elm->node = node;
//   xbt_dict_set_ext(routing->graph_node_map, (char*)(&id), sizeof(int), (xbt_set_elm_t)elm, &graph_node_map_elem_free);
// 
//   return node;
// }
// 
// static graph_node_map_element_t graph_node_map_search(int id) {
//   routing_dijkstra_t routing = (routing_dijkstra_t) used_routing;
// 
//   graph_node_map_element_t elm = (graph_node_map_element_t)xbt_dict_get_or_null_ext(routing->graph_node_map, (char*)(&id), sizeof(int));
// 
//   return elm;
// }
// 
// /*
//  * Parsing
//  */
// static void route_new_dijkstra(int src_id, int dst_id, void* link) {
//   routing_dijkstra_t routing = (routing_dijkstra_t) used_routing;
// 
//   xbt_node_t src = NULL;
//   xbt_node_t dst = NULL;
//   graph_node_map_element_t src_elm = (graph_node_map_element_t)xbt_dict_get_or_null_ext(routing->graph_node_map, (char*)(&src_id), sizeof(int));
//   graph_node_map_element_t dst_elm = (graph_node_map_element_t)xbt_dict_get_or_null_ext(routing->graph_node_map, (char*)(&dst_id), sizeof(int));
// 
//   if(src_elm)
//     src = src_elm->node;
// 
//   if(dst_elm)
//     dst = dst_elm->node;
// 
//   //add nodes if they don't exist in the graph
//   if(src_id == dst_id && src == NULL && dst == NULL) {
//     src = route_graph_new_node(src_id, -1);
//     dst = src;
//   } else {
//     if(src == NULL) {
//       src = route_graph_new_node(src_id, -1);
//     }
// 	
//     if(dst == NULL) {
//       dst = route_graph_new_node(dst_id, -1);
//     }
//   }
// 
//   //add link as edge to graph
//   xbt_graph_new_edge(routing->route_graph, src, dst, link);
//   
// }
// 
// static void add_loopback_dijkstra(void) {
//   routing_dijkstra_t routing = (routing_dijkstra_t) used_routing;
// 
//  	xbt_dynar_t nodes = xbt_graph_get_nodes(routing->route_graph);
// 	
// 	xbt_node_t node = NULL;
// 	unsigned int cursor2;
// 	xbt_dynar_foreach(nodes, cursor2, node) {
// 		xbt_dynar_t out_edges = xbt_graph_node_get_outedges(node); 
// 		xbt_edge_t edge = NULL;
// 		unsigned int cursor;
// 	
// 		int found = 0;
// 		xbt_dynar_foreach(out_edges, cursor, edge) {
// 			xbt_node_t other_node = xbt_graph_edge_get_target(edge);
// 			if(other_node == node) {
// 				found = 1;
// 				break;
// 			}
// 		}
// 
// 		if(!found)
// 			xbt_graph_new_edge(routing->route_graph, node, node, &routing->loopback);
// 	}
// }
// 
// static void routing_dijkstra_parse_end(void) {
//   routing_dijkstra_t routing = (routing_dijkstra_t) used_routing;
//   int nb_link = 0;
//   xbt_dict_cursor_t cursor = NULL;
//   char *key, *data, *end;
//   const char *sep = "#";
//   xbt_dynar_t links, keys;
//   char* link_name = NULL;
//   void* link = NULL;
//   xbt_node_t node = NULL;
//   unsigned int cursor2;
//   xbt_dynar_t nodes = NULL;
//   /* Create the topology graph */
//   routing->route_graph = xbt_graph_new_graph(1, NULL);
//   routing->graph_node_map = xbt_dict_new();
//   routing->last_route = xbt_dynar_new(routing->size_of_link, NULL);
//   if(routing->cached)
//     routing->route_cache = xbt_dict_new();
// 
// 
//   /* Put the routes in position */
//   xbt_dict_foreach(route_table, cursor, key, data) {
//     nb_link = 0;
//     links = (xbt_dynar_t) data;
//     keys = xbt_str_split_str(key, sep);
// 
//     src_id = strtol(xbt_dynar_get_as(keys, 0, char *), &end, 16);
//     dst_id = strtol(xbt_dynar_get_as(keys, 1, char *), &end, 16);
//     xbt_dynar_free(&keys);
// 
//     DEBUG4("Handle %d %d (from %d hosts): %ld links",
//         src_id,dst_id,routing->generic_routing.host_count,xbt_dynar_length(links));
// 
//     xbt_assert3(xbt_dynar_length(links) == 1, "%ld links in route between host %d and %d, should be 1", xbt_dynar_length(links), src_id, dst_id);
// 
//     link_name = xbt_dynar_getfirst_as(links, char*);
//     link = xbt_dict_get_or_null(surf_network_model->resource_set, link_name);
//     if (link)
//       route_new_dijkstra(src_id,dst_id,link);
//     else
//       THROW1(mismatch_error,0,"Link %s not found", link_name);
//     
//   }
// 
//   /* Add the loopback if needed */
//   add_loopback_dijkstra();
// 
//   /* initialize graph indexes in nodes after graph has been built */
//   nodes = xbt_graph_get_nodes(routing->route_graph);
// 
//   xbt_dynar_foreach(nodes, cursor2, node) {
//     graph_node_data_t data = xbt_graph_node_get_data(node);
//     data->graph_id = cursor2;
//   }
// 
// }
// 
// /*
//  * Business methods
//  */
// static xbt_dynar_t routing_dijkstra_get_route(int src_id,int dst_id) {
// 
//   routing_dijkstra_t routing = (routing_dijkstra_t) used_routing;
//   int * pred_arr = NULL;
//   int src_node_id = 0;
//   int dst_node_id = 0;
//   int * nodeid = NULL;
//   int v;
//   int size = 0;
//   void * link = NULL;
//   route_cache_element_t elm = NULL;
//   xbt_dynar_t nodes = xbt_graph_get_nodes(routing->route_graph);
// 
//   /*Use the graph_node id mapping set to quickly find the nodes */
//   graph_node_map_element_t src_elm = graph_node_map_search(src_id);
//   graph_node_map_element_t dst_elm = graph_node_map_search(dst_id);
//   xbt_assert2(src_elm != NULL && dst_elm != NULL, "src %d or dst %d does not exist", src_id, dst_id);
//   src_node_id = ((graph_node_data_t)xbt_graph_node_get_data(src_elm->node))->graph_id;
//   dst_node_id = ((graph_node_data_t)xbt_graph_node_get_data(dst_elm->node))->graph_id;
// 
//   if(routing->cached) {
//     /*check if there is a cached predecessor list avail */
//     elm = (route_cache_element_t)xbt_dict_get_or_null_ext(routing->route_cache, (char*)(&src_id), sizeof(int));
//   }
// 
//   if(elm) { //cached mode and cache hit
//     pred_arr = elm->pred_arr;
//   } else { //not cached mode or cache miss
//     double * cost_arr = NULL;
//     xbt_heap_t pqueue = NULL;
//     int i = 0;
// 
//     int nr_nodes = xbt_dynar_length(nodes);
//     cost_arr = xbt_new0(double, nr_nodes); //link cost from src to other hosts
//     pred_arr = xbt_new0(int, nr_nodes); //predecessors in path from src
//     pqueue = xbt_heap_new(nr_nodes, free);
// 
//     //initialize
//     cost_arr[src_node_id] = 0.0;
// 
//     for(i = 0; i < nr_nodes; i++) {
//       if(i != src_node_id) {
//         cost_arr[i] = DBL_MAX;
//       }
// 
//       pred_arr[i] = 0;
// 
//       //initialize priority queue
//       nodeid = xbt_new0(int, 1);
//       *nodeid = i;
//       xbt_heap_push(pqueue, nodeid, cost_arr[i]);
// 
//     }
// 
//     // apply dijkstra using the indexes from the graph's node array
//     while(xbt_heap_size(pqueue) > 0) {
//       int * v_id = xbt_heap_pop(pqueue);
//       xbt_node_t v_node = xbt_dynar_get_as(nodes, *v_id, xbt_node_t);
//       xbt_dynar_t out_edges = xbt_graph_node_get_outedges(v_node); 
//       xbt_edge_t edge = NULL;
//       unsigned int cursor;
// 
//       xbt_dynar_foreach(out_edges, cursor, edge) {
//         xbt_node_t u_node = xbt_graph_edge_get_target(edge);
//         graph_node_data_t data = xbt_graph_node_get_data(u_node);
//         int u_id = data->graph_id;
//         int cost_v_u = 1; //fixed link cost for now
// 
//         if(cost_v_u + cost_arr[*v_id] < cost_arr[u_id]) {
//           pred_arr[u_id] = *v_id;
//           cost_arr[u_id] = cost_v_u + cost_arr[*v_id];
//           nodeid = xbt_new0(int, 1);
//           *nodeid = u_id;
//           xbt_heap_push(pqueue, nodeid, cost_arr[u_id]);
//         }
//       }
// 
//       //free item popped from pqueue
//       free(v_id);
//     }
// 
//     free(cost_arr);
//     xbt_heap_free(pqueue);
// 
//   }
// 
//   //compose route path with links
//   xbt_dynar_reset(routing->last_route);
// 
//   for(v = dst_node_id; v != src_node_id; v = pred_arr[v]) {
//     xbt_node_t node_pred_v = xbt_dynar_get_as(nodes, pred_arr[v], xbt_node_t);
//     xbt_node_t node_v = xbt_dynar_get_as(nodes, v, xbt_node_t);
//     xbt_edge_t edge = xbt_graph_get_edge(routing->route_graph, node_pred_v, node_v);
// 
//     xbt_assert2(edge != NULL, "no route between host %d and %d", src_id, dst_id);
// 
//     link = xbt_graph_edge_get_data(edge);
//     xbt_dynar_unshift(routing->last_route, &link);
//     size++;
//   }
// 
// 
//   if(routing->cached && elm == NULL) {
//     //add to predecessor list of the current src-host to cache
//     elm = xbt_new0(struct route_cache_element, sizeof(struct route_cache_element));
//     elm->pred_arr = pred_arr;
//     elm->size = size;
//     xbt_dict_set_ext(routing->route_cache, (char*)(&src_id), sizeof(int), (xbt_set_elm_t)elm, &route_cache_elem_free);
//   }
// 
//   if(!routing->cached)
//     free(pred_arr);
// 
//   return routing->last_route;
// }
// 
// 
// static void routing_dijkstra_finalize(void) {
//   routing_dijkstra_t routing = (routing_dijkstra_t)used_routing;
// 
//   if (routing) {
//     xbt_graph_free_graph(routing->route_graph, &free, NULL, &free);
//     xbt_dict_free(&routing->graph_node_map);
//     if(routing->cached)
//       xbt_dict_free(&routing->route_cache);
//     xbt_dynar_free(&routing->last_route);
//     xbt_dict_free(&used_routing->host_id);
//     free(routing);
//     routing=NULL;
//   }
// }
// 
// static xbt_dict_t routing_dijkstraboth_get_onelink_routes(void){
//   xbt_assert0(0,"The get_onelink_routes feature is not supported in routing model dijkstraboth");
// }
// 
// static int routing_dijkstraboth_is_router(int id){
//   xbt_assert0(0,"The get_is_router feature is not supported in routing model dijkstraboth");
// }
// 
// /*
//  *
//  */
// static void routing_model_dijkstraboth_create(size_t size_of_link,void *loopback, int cached) {
//   /* initialize our structure */
//   routing_dijkstra_t routing = xbt_new0(s_routing_dijkstra_t,1);
//   routing->generic_routing.name = "Dijkstra";
//   routing->generic_routing.host_count = 0;
//   routing->generic_routing.get_route = routing_dijkstra_get_route;
//   routing->generic_routing.get_onelink_routes = routing_dijkstraboth_get_onelink_routes;
//   routing->generic_routing.is_router = routing_dijkstraboth_is_router;
//   routing->generic_routing.finalize = routing_dijkstra_finalize;
//   routing->size_of_link = size_of_link;
//   routing->loopback = loopback;
//   routing->cached = cached;
// 
//   /* Set it in position */
//   used_routing = (routing_t) routing;
// 
//   /* Setup the parsing callbacks we need */
//   routing->generic_routing.host_id = xbt_dict_new();
//   surfxml_add_callback(STag_surfxml_host_cb_list, &routing_full_parse_Shost);
//   surfxml_add_callback(ETag_surfxml_platform_cb_list, &routing_dijkstra_parse_end);
//   surfxml_add_callback(STag_surfxml_route_cb_list,
//       &routing_full_parse_Sroute_set_endpoints);
//   surfxml_add_callback(ETag_surfxml_route_cb_list, &routing_full_parse_Eroute);
//   surfxml_add_callback(STag_surfxml_cluster_cb_list, &routing_shortest_path_parse_Scluster);
// }
// 
// static void routing_model_dijkstra_create(size_t size_of_link,void *loopback) {
//   routing_model_dijkstraboth_create(size_of_link, loopback, 0);
// }
// 
// static void routing_model_dijkstracache_create(size_t size_of_link,void *loopback) {
//   routing_model_dijkstraboth_create(size_of_link, loopback, 1);
// }

/* ************************************************** */
/* ********** NO ROUTING **************************** */


// static void routing_none_finalize(void) {
//   if (used_routing) {
//     xbt_dict_free(&used_routing->host_id);
//     free(used_routing);
//     used_routing=NULL;
//   }
// }
// 
// static void routing_model_none_create(size_t size_of_link,void *loopback) {
//   routing_t routing = xbt_new0(s_routing_t,1);
//   INFO0("Null routing");
//   routing->name = "none";
//   routing->host_count = 0;
//   routing->host_id = xbt_dict_new();
//   routing->get_onelink_routes = NULL;
//   routing->is_router = NULL;
//   routing->get_route = NULL;
// 
//   routing->finalize = routing_none_finalize;
// 
//   /* Set it in position */
//   used_routing = (routing_t) routing;
// }
