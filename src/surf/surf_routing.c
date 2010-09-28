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

routing_t used_routing = NULL;
xbt_dict_t onelink_routes = NULL;

/* Prototypes of each model */
static void routing_model_full_create(size_t size_of_link,void *loopback);
static void routing_model_floyd_create(size_t size_of_link, void*loopback);
static void routing_model_dijkstra_create(size_t size_of_link, void*loopback);
static void routing_model_dijkstracache_create(size_t size_of_link, void*loopback);
static void routing_model_hierarchical_create(size_t size_of_link, void*loopback); // added by DAVID
static void routing_model_none_create(size_t size_of_link, void*loopback);

/* Definition of each model */
struct model_type {
  const char *name;
  const char *desc;
  void (*fun)(size_t,void*);
};
struct model_type models[] =
{ {"Full", "Full routing data (fast, large memory requirements, fully expressive)", routing_model_full_create },
  {"Floyd", "Floyd routing data (slow initialization, fast lookup, lesser memory requirements, shortest path routing only)", routing_model_floyd_create },
  {"Dijkstra", "Dijkstra routing data (fast initialization, slow lookup, small memory requirements, shortest path routing only)", routing_model_dijkstra_create },
  {"DijkstraCache", "Dijkstra routing data (fast initialization, fast lookup, small memory requirements, shortest path routing only)", routing_model_dijkstracache_create },
  {"Hierarchical", "Hierarchical routing", routing_model_hierarchical_create }, // added by DAVID
  {"none", "No routing (usable with Constant network only)", routing_model_none_create },
  {NULL,NULL,NULL}};


void routing_model_create(size_t size_of_links, void* loopback) {
// 
  char * wanted=xbt_cfg_get_string(_surf_cfg_set,"routing");
  int cpt;
  for (cpt=0;models[cpt].name;cpt++) {
    if (!strcmp(wanted,models[cpt].name)) {
      (*(models[cpt].fun))(size_of_links,loopback);
      return;
    }
  }
  fprintf(stderr,"Routing model %s not found. Existing models:\n",wanted);
  for (cpt=0;models[cpt].name;cpt++)
    fprintf(stderr,"   %s: %s\n",models[cpt].name,models[cpt].desc);
  xbt_die("Invalid model.");
}
// 
/* ************************************************************************** */
/* *************************** FULL ROUTING ********************************* */
typedef struct {
  s_routing_t generic_routing;
  xbt_dynar_t *routing_table;
  void *loopback;
  size_t size_of_link;
} s_routing_full_t,*routing_full_t;

#define ROUTE_FULL(i,j) ((routing_full_t)used_routing)->routing_table[(i)+(j)*(used_routing)->host_count]
#define HOST2ROUTER(id) ((id)+(2<<29))
#define ROUTER2HOST(id) ((id)-(2<<29))
#define ISROUTER(id) ((id)>=(2<<29))

/*
 * Free the onelink routes
 */
static void onelink_route_elem_free(void *e) {
  s_onelink_t tmp = (s_onelink_t)e;
  if(tmp) {
    free(tmp);
  }
}


/*
 * Parsing
 */
static void routing_full_parse_Shost(void) {
  int *val = xbt_malloc(sizeof(int));
  DEBUG2("Seen host %s (#%d)",A_surfxml_host_id,used_routing->host_count);
  *val = used_routing->host_count++;
  xbt_dict_set(used_routing->host_id,A_surfxml_host_id,val,xbt_free);
#ifdef HAVE_TRACING
  TRACE_surf_host_define_id (A_surfxml_host_id, *val);
#endif
}

static void routing_full_parse_Srouter(void) {
	int *val = xbt_malloc(sizeof(int));
  DEBUG3("Seen router %s (%d -> #%d)",A_surfxml_router_id,used_routing->router_count,
  		HOST2ROUTER(used_routing->router_count));
  *val = HOST2ROUTER(used_routing->router_count++);
  xbt_dict_set(used_routing->host_id,A_surfxml_router_id,val,xbt_free);
#ifdef HAVE_TRACING
  TRACE_surf_host_define_id (A_surfxml_router_id, *val);
  TRACE_surf_host_declaration (A_surfxml_router_id, 0);
#endif
}

static int src_id = -1;
static int dst_id = -1;
static void routing_full_parse_Sroute_set_endpoints(void)
{
  src_id = *(int*)xbt_dict_get(used_routing->host_id,A_surfxml_route_src);
  dst_id = *(int*)xbt_dict_get(used_routing->host_id,A_surfxml_route_dst);
  DEBUG4("Route %s %d -> %s %d",A_surfxml_route_src,src_id,A_surfxml_route_dst,dst_id);
  route_action = A_surfxml_route_action;
}
static void routing_full_parse_Eroute(void)
{
  char *name;
  if (src_id != -1 && dst_id != -1) {
    name = bprintf("%x#%x", src_id, dst_id);
    manage_route(route_table, name, route_action, 0);
    free(name);
  }
}

/* Cluster tag functions */

static void routing_full_parse_change_cpu_data(const char *hostName,
                                  const char *surfxml_host_power,
                                  const char *surfxml_host_availability,
                                  const char *surfxml_host_availability_file,
                                  const char *surfxml_host_state_file)
{
  int AX_ptr = 0;

  SURFXML_BUFFER_SET(host_id, hostName);
  SURFXML_BUFFER_SET(host_power, surfxml_host_power /*hostPower */ );
  SURFXML_BUFFER_SET(host_availability, surfxml_host_availability);
  SURFXML_BUFFER_SET(host_availability_file, surfxml_host_availability_file);
  SURFXML_BUFFER_SET(host_state_file, surfxml_host_state_file);
}

static void routing_full_parse_change_link_data(const char *linkName,
                                   const char *surfxml_link_bandwidth,
                                   const char *surfxml_link_bandwidth_file,
                                   const char *surfxml_link_latency,
                                   const char *surfxml_link_latency_file,
                                   const char *surfxml_link_state_file)
{
  int AX_ptr = 0;

  SURFXML_BUFFER_SET(link_id, linkName);
  SURFXML_BUFFER_SET(link_bandwidth, surfxml_link_bandwidth);
  SURFXML_BUFFER_SET(link_bandwidth_file, surfxml_link_bandwidth_file);
  SURFXML_BUFFER_SET(link_latency, surfxml_link_latency);
  SURFXML_BUFFER_SET(link_latency_file, surfxml_link_latency_file);
  SURFXML_BUFFER_SET(link_state_file, surfxml_link_state_file);
}

static void routing_full_parse_Scluster(void)
{
  static int AX_ptr = 0;

  char *cluster_id = A_surfxml_cluster_id;
  char *cluster_prefix = A_surfxml_cluster_prefix;
  char *cluster_suffix = A_surfxml_cluster_suffix;
  char *cluster_radical = A_surfxml_cluster_radical;
  char *cluster_power = A_surfxml_cluster_power;
  char *cluster_bw = A_surfxml_cluster_bw;
  char *cluster_lat = A_surfxml_cluster_lat;
  char *cluster_bb_bw = A_surfxml_cluster_bb_bw;
  char *cluster_bb_lat = A_surfxml_cluster_bb_lat;
  char *backbone_name;
  unsigned int it1,it2;
  char *name1,*name2;
  xbt_dynar_t names = NULL;
  surfxml_bufferstack_push(1);

  /* Make set a set to parse the prefix/suffix/radical into a neat list of names */
  DEBUG4("Make <set id='%s' prefix='%s' suffix='%s' radical='%s'>",
      cluster_id,cluster_prefix,cluster_suffix,cluster_radical);
  SURFXML_BUFFER_SET(set_id, cluster_id);
  SURFXML_BUFFER_SET(set_prefix, cluster_prefix);
  SURFXML_BUFFER_SET(set_suffix, cluster_suffix);
  SURFXML_BUFFER_SET(set_radical, cluster_radical);

  SURFXML_START_TAG(set);
  SURFXML_END_TAG(set);

  names = xbt_dict_get(set_list,cluster_id);

  xbt_dynar_foreach(names,it1,name1) {
    /* create the host */
    routing_full_parse_change_cpu_data(name1, cluster_power, "1.0", "", "");
    A_surfxml_host_state = A_surfxml_host_state_ON;

    SURFXML_START_TAG(host);
    SURFXML_END_TAG(host);

    /* Here comes the link */
    routing_full_parse_change_link_data(name1, cluster_bw, "", cluster_lat, "", "");
    A_surfxml_link_state = A_surfxml_link_state_ON;
    A_surfxml_link_sharing_policy = A_surfxml_link_sharing_policy_SHARED;

    SURFXML_START_TAG(link);
    SURFXML_END_TAG(link);
  }

  /* Make backbone link */
  backbone_name = bprintf("%s_bb", cluster_id);
  routing_full_parse_change_link_data(backbone_name, cluster_bb_bw, "", cluster_bb_lat, "",
                         "");
  A_surfxml_link_state = A_surfxml_link_state_ON;
  A_surfxml_link_sharing_policy = A_surfxml_link_sharing_policy_FATPIPE;

  SURFXML_START_TAG(link);
  SURFXML_END_TAG(link);

  /* And now the internal routes */
  xbt_dynar_foreach(names,it1,name1) {
    xbt_dynar_foreach(names,it2,name2) {
      if (strcmp(name1,name2)) {
        A_surfxml_route_action = A_surfxml_route_action_POSTPEND;
        SURFXML_BUFFER_SET(route_src,name1);
        SURFXML_BUFFER_SET(route_dst,name2);
        SURFXML_START_TAG(route); {
          /* FIXME: name1 link is added by error about 20 lines below, so don't add it here
          SURFXML_BUFFER_SET(link_c_ctn_id, name1);
          SURFXML_START_TAG(link_c_ctn);
          SURFXML_END_TAG(link_c_ctn);
           */
          SURFXML_BUFFER_SET(link_c_ctn_id, backbone_name);
          SURFXML_START_TAG(link_c_ctn);
          SURFXML_END_TAG(link_c_ctn);

          SURFXML_BUFFER_SET(link_c_ctn_id, name2);
          SURFXML_START_TAG(link_c_ctn);
          SURFXML_END_TAG(link_c_ctn);

        } SURFXML_END_TAG(route);
      }
    }
  }

  /* Make route multi with the outside world, i.e. cluster->$* */

  /* FIXME
   * This also adds an elements to the routes within the cluster,
   * and I guess it's wrong, but since this element is commented out in the above
   * code creating the internal routes, we're good.
   * To fix it, I'd say that we need a way to understand "$*-${cluster_id}" as "whole world, but the guys in that cluster"
   * But for that, we need to install a real expression parser for src/dst attributes
   *
   * FIXME
   * This also adds a dumb element (the private link) in place of the loopback. Then, since
   * the loopback is added only if no link to self already exist, this fails.
   * That's really dumb.
   *
   * FIXME
   * It seems to me that it does not add the backbone to the path to outside world...
   */
  SURFXML_BUFFER_SET(route_c_multi_src, cluster_id);
  SURFXML_BUFFER_SET(route_c_multi_dst, "$*");
  A_surfxml_route_c_multi_symmetric = A_surfxml_route_c_multi_symmetric_NO;
  A_surfxml_route_c_multi_action = A_surfxml_route_c_multi_action_PREPEND;

  SURFXML_START_TAG(route_c_multi);

  SURFXML_BUFFER_SET(link_c_ctn_id, "$src");

  SURFXML_START_TAG(link_c_ctn);
  SURFXML_END_TAG(link_c_ctn);

  SURFXML_END_TAG(route_c_multi);

  free(backbone_name);

  /* Restore buff */
  surfxml_bufferstack_pop(1);
}


static void routing_full_parse_end(void) {
  routing_full_t routing = (routing_full_t) used_routing;
  int nb_link = 0;
  unsigned int cpt = 0;
  xbt_dict_cursor_t cursor = NULL;
  char *key, *data, *end;
  const char *sep = "#";
  xbt_dynar_t links, keys;
  char *link_name = NULL;
  int i,j;

  int host_count = routing->generic_routing.host_count;

  /* Create the routing table */
  routing->routing_table = xbt_new0(xbt_dynar_t, host_count * host_count);
  for (i=0;i<host_count;i++)
    for (j=0;j<host_count;j++)
      ROUTE_FULL(i,j) = xbt_dynar_new(routing->size_of_link,NULL);

  /* Put the routes in position */
  xbt_dict_foreach(route_table, cursor, key, data) {
    nb_link = 0;
    links = (xbt_dynar_t) data;
    keys = xbt_str_split_str(key, sep);

    src_id = strtol(xbt_dynar_get_as(keys, 0, char *), &end, 16);
    dst_id = strtol(xbt_dynar_get_as(keys, 1, char *), &end, 16);
    xbt_dynar_free(&keys);

    if(xbt_dynar_length(links) == 1){
      s_onelink_t new_link = (s_onelink_t) xbt_malloc0(sizeof(s_onelink));
      new_link->src_id = src_id;
      new_link->dst_id = dst_id;
      link_name = xbt_dynar_getfirst_as(links, char*);
      new_link->link_ptr = xbt_dict_get_or_null(surf_network_model->resource_set, link_name);
      DEBUG3("Adding onelink route from (#%d) to (#%d), link_name %s",src_id, dst_id, link_name);
      xbt_dict_set(onelink_routes, link_name, (void *)new_link, onelink_route_elem_free);
#ifdef HAVE_TRACING
      TRACE_surf_link_save_endpoints (link_name, src_id, dst_id);
#endif
    }

    if(ISROUTER(src_id) || ISROUTER(dst_id)) {
				DEBUG2("There is route with a router here: (%d ,%d)",src_id,dst_id);
				/* Check there is only one link in the route and store the information */
				continue;
    }

    DEBUG4("Handle %d %d (from %d hosts): %ld links",
        src_id,dst_id,routing->generic_routing.host_count,xbt_dynar_length(links));
    xbt_dynar_foreach(links, cpt, link_name) {
      void* link = xbt_dict_get_or_null(surf_network_model->resource_set, link_name);
      if (link)
        xbt_dynar_push(ROUTE_FULL(src_id,dst_id),&link);
      else
        THROW1(mismatch_error,0,"Link %s not found", link_name);
    }
  }

  /* Add the loopback if needed */
  for (i = 0; i < host_count; i++)
    if (!xbt_dynar_length(ROUTE_FULL(i, i)))
      xbt_dynar_push(ROUTE_FULL(i,i),&routing->loopback);

  /* Shrink the dynar routes (save unused slots) */
  for (i=0;i<host_count;i++)
    for (j=0;j<host_count;j++)
      xbt_dynar_shrink(ROUTE_FULL(i,j),0);
}

/*
 * Business methods
 */
static xbt_dynar_t routing_full_get_route(int src,int dst) {
  xbt_assert0(!(ISROUTER(src) || ISROUTER(dst)), "Ask for route \"from\" or \"to\" a router node");
  return ROUTE_FULL(src,dst);
}

static xbt_dict_t routing_full_get_onelink_routes(void){
  return onelink_routes;
}

static int routing_full_is_router(int id){
	return ISROUTER(id);
}

static void routing_full_finalize(void) {
  routing_full_t routing = (routing_full_t)used_routing;
  int i,j;

  if (routing) {
    for (i = 0; i < used_routing->host_count; i++)
      for (j = 0; j < used_routing->host_count; j++)
        xbt_dynar_free(&ROUTE_FULL(i, j));
    free(routing->routing_table);
    xbt_dict_free(&used_routing->host_id);
    xbt_dict_free(&onelink_routes);
    free(routing);
    routing=NULL;
  }
}

static void routing_model_full_create(size_t size_of_link,void *loopback) {
  /* initialize our structure */
  routing_full_t routing = xbt_new0(s_routing_full_t,1);
  routing->generic_routing.name = "Full";
  routing->generic_routing.host_count = 0;
  routing->generic_routing.get_route = routing_full_get_route;
  routing->generic_routing.get_onelink_routes = routing_full_get_onelink_routes;
  routing->generic_routing.is_router = routing_full_is_router;
  routing->generic_routing.finalize = routing_full_finalize;

  routing->size_of_link = size_of_link;
  routing->loopback = loopback;

  /* Set it in position */
  used_routing = (routing_t) routing;

  /* Set the dict for onehop routes */
  onelink_routes =  xbt_dict_new();

  /* Setup the parsing callbacks we need */
  routing->generic_routing.host_id = xbt_dict_new();
  surfxml_add_callback(STag_surfxml_host_cb_list, &routing_full_parse_Shost);
  surfxml_add_callback(STag_surfxml_router_cb_list, &routing_full_parse_Srouter);
  surfxml_add_callback(ETag_surfxml_platform_cb_list, &routing_full_parse_end);
  surfxml_add_callback(STag_surfxml_route_cb_list,
      &routing_full_parse_Sroute_set_endpoints);
  surfxml_add_callback(ETag_surfxml_route_cb_list, &routing_full_parse_Eroute);
  surfxml_add_callback(STag_surfxml_cluster_cb_list, &routing_full_parse_Scluster);
}

/* ************************************************************************** */

static void routing_shortest_path_parse_Scluster(void)
{
  static int AX_ptr = 0;

  char *cluster_id = A_surfxml_cluster_id;
  char *cluster_prefix = A_surfxml_cluster_prefix;
  char *cluster_suffix = A_surfxml_cluster_suffix;
  char *cluster_radical = A_surfxml_cluster_radical;
  char *cluster_power = A_surfxml_cluster_power;
  char *cluster_bb_bw = A_surfxml_cluster_bb_bw;
  char *cluster_bb_lat = A_surfxml_cluster_bb_lat;
  char *backbone_name;

  surfxml_bufferstack_push(1);

  /* Make set */
  SURFXML_BUFFER_SET(set_id, cluster_id);
  SURFXML_BUFFER_SET(set_prefix, cluster_prefix);
  SURFXML_BUFFER_SET(set_suffix, cluster_suffix);
  SURFXML_BUFFER_SET(set_radical, cluster_radical);

  SURFXML_START_TAG(set);
  SURFXML_END_TAG(set);

  /* Make foreach */
  SURFXML_BUFFER_SET(foreach_set_id, cluster_id);

  SURFXML_START_TAG(foreach);

  /* Make host for the foreach */
  routing_full_parse_change_cpu_data("$1", cluster_power, "1.0", "", "");
  A_surfxml_host_state = A_surfxml_host_state_ON;

  SURFXML_START_TAG(host);
  SURFXML_END_TAG(host);

  SURFXML_END_TAG(foreach);

  /* Make backbone link */
  backbone_name = bprintf("%s_bb", cluster_id);
  routing_full_parse_change_link_data(backbone_name, cluster_bb_bw, "", cluster_bb_lat, "",
                         "");
  A_surfxml_link_state = A_surfxml_link_state_ON;
  A_surfxml_link_sharing_policy = A_surfxml_link_sharing_policy_FATPIPE;

  SURFXML_START_TAG(link);
  SURFXML_END_TAG(link);

  free(backbone_name);

  /* Restore buff */
  surfxml_bufferstack_pop(1);
}

/* ************************************************************************** */
/* *************************** FLOYD ROUTING ********************************* */
typedef struct {
  s_routing_t generic_routing;
  int *predecessor_table;
  void** link_table;
  xbt_dynar_t last_route;
  void *loopback;
  size_t size_of_link;
} s_routing_floyd_t,*routing_floyd_t;

#define FLOYD_COST(i,j) cost_table[(i)+(j)*(used_routing)->host_count]
#define FLOYD_PRED(i,j) ((routing_floyd_t)used_routing)->predecessor_table[(i)+(j)*(used_routing)->host_count]
#define FLOYD_LINK(i,j) ((routing_floyd_t)used_routing)->link_table[(i)+(j)*(used_routing)->host_count]

static void routing_floyd_parse_end(void) {

  routing_floyd_t routing = (routing_floyd_t) used_routing;
  int nb_link = 0;
  void* link_list = NULL;
  double * cost_table;
  xbt_dict_cursor_t cursor = NULL;
  char *key,*data, *end;
  const char *sep = "#";
  xbt_dynar_t links, keys;

  unsigned int i,j;
  unsigned int a,b,c;
  int host_count = routing->generic_routing.host_count;
  char * link_name = NULL;
  void * link = NULL;

  /* Create Cost, Predecessor and Link tables */
  cost_table = xbt_new0(double, host_count * host_count); //link cost from host to host
  routing->predecessor_table = xbt_new0(int, host_count*host_count); //predecessor host numbers
  routing->link_table = xbt_new0(void*,host_count*host_count); //actual link between src and dst
  routing->last_route = xbt_dynar_new(routing->size_of_link, NULL);

  /* Initialize costs and predecessors*/
  for(i = 0; i<host_count;i++)
    for(j = 0; j<host_count;j++) {
        FLOYD_COST(i,j) = DBL_MAX;
        FLOYD_PRED(i,j) = -1;
    }

   /* Put the routes in position */
  xbt_dict_foreach(route_table, cursor, key, data) {
    nb_link = 0;
    links = (xbt_dynar_t)data;
    keys = xbt_str_split_str(key, sep);

    
    src_id = strtol(xbt_dynar_get_as(keys, 0, char*), &end, 16);
    dst_id = strtol(xbt_dynar_get_as(keys, 1, char*), &end, 16);
    xbt_dynar_free(&keys);
 
    DEBUG4("Handle %d %d (from %d hosts): %ld links",
        src_id,dst_id,routing->generic_routing.host_count,xbt_dynar_length(links));
    xbt_assert3(xbt_dynar_length(links) == 1, "%ld links in route between host %d and %d, should be 1", xbt_dynar_length(links), src_id, dst_id);
    
    link_name = xbt_dynar_getfirst_as(links, char*);
    link = xbt_dict_get_or_null(surf_network_model->resource_set, link_name);

    if (link)
      link_list = link;
    else
      THROW1(mismatch_error,0,"Link %s not found", link_name);
    

    FLOYD_LINK(src_id,dst_id) = link_list;
    FLOYD_PRED(src_id, dst_id) = src_id;

    //link cost
    FLOYD_COST(src_id, dst_id) = 1; // assume 1 for now

  }

    /* Add the loopback if needed */
  for (i = 0; i < host_count; i++)
    if (!FLOYD_PRED(i, i)) {
      FLOYD_PRED(i, i) = i;
      FLOYD_COST(i, i) = 1;
      FLOYD_LINK(i, i) = routing->loopback;
    }


  //Calculate path costs 

  for(c=0;c<host_count;c++) {
    for(a=0;a<host_count;a++) {
      for(b=0;b<host_count;b++) {
        if(FLOYD_COST(a,c) < DBL_MAX && FLOYD_COST(c,b) < DBL_MAX) {
          if(FLOYD_COST(a,b) == DBL_MAX || (FLOYD_COST(a,c)+FLOYD_COST(c,b) < FLOYD_COST(a,b))) {
            FLOYD_COST(a,b) = FLOYD_COST(a,c)+FLOYD_COST(c,b);
            FLOYD_PRED(a,b) = FLOYD_PRED(c,b);
          }
        }
      }
    }
  }

  //cleanup
  free(cost_table);
}

/*
 * Business methods
 */
static xbt_dynar_t routing_floyd_get_route(int src_id,int dst_id) {

  routing_floyd_t routing = (routing_floyd_t) used_routing;

  int pred = dst_id;
  int prev_pred = 0;

  xbt_dynar_reset(routing->last_route);

  do {
    prev_pred = pred;
    pred = FLOYD_PRED(src_id, pred);

    if(pred == -1) // if no pred in route -> no route to host
        break;

    xbt_dynar_unshift(routing->last_route, &FLOYD_LINK(pred,prev_pred));

  } while(pred != src_id);

  xbt_assert2(pred != -1, "no route from host %d to %d", src_id, dst_id);

  return routing->last_route;
}

static void routing_floyd_finalize(void) {
  routing_floyd_t routing = (routing_floyd_t)used_routing;

  if (routing) {
    free(routing->link_table);
    free(routing->predecessor_table);
    xbt_dynar_free(&routing->last_route);
    xbt_dict_free(&used_routing->host_id);
    free(routing);
    routing=NULL;
  }
}

static xbt_dict_t routing_floyd_get_onelink_routes(void){
  xbt_assert0(0,"The get_onelink_routes feature is not supported in routing model Floyd");
}

static int routing_floyd_is_router(int id){
  xbt_assert0(0,"The get_is_router feature is not supported in routing model Floyd");
}

static void routing_model_floyd_create(size_t size_of_link,void *loopback) {
  /* initialize our structure */
  routing_floyd_t routing = xbt_new0(s_routing_floyd_t,1);
  routing->generic_routing.name = "Floyd";
  routing->generic_routing.host_count = 0;
  routing->generic_routing.host_id = xbt_dict_new();
  routing->generic_routing.get_route = routing_floyd_get_route;
  routing->generic_routing.get_onelink_routes = routing_floyd_get_onelink_routes;
  routing->generic_routing.is_router = routing_floyd_is_router;
  routing->generic_routing.finalize = routing_floyd_finalize;
  routing->size_of_link = size_of_link;
  routing->loopback = loopback;

  /* Set it in position */
  used_routing = (routing_t) routing;
  
  /* Setup the parsing callbacks we need */
  surfxml_add_callback(STag_surfxml_host_cb_list, &routing_full_parse_Shost);
  surfxml_add_callback(ETag_surfxml_platform_cb_list, &routing_floyd_parse_end);
  surfxml_add_callback(STag_surfxml_route_cb_list, 
      &routing_full_parse_Sroute_set_endpoints);
  surfxml_add_callback(ETag_surfxml_route_cb_list, &routing_full_parse_Eroute);
  surfxml_add_callback(STag_surfxml_cluster_cb_list, &routing_shortest_path_parse_Scluster);
  
}

/* ************************************************************************** */
/* ********** Dijkstra & Dijkstra Cached ROUTING **************************** */
typedef struct {
  s_routing_t generic_routing;
  xbt_graph_t route_graph;
  xbt_dict_t graph_node_map;
  xbt_dict_t route_cache;
  xbt_dynar_t last_route;
  int cached;
  void *loopback;
  size_t size_of_link;
} s_routing_dijkstra_t,*routing_dijkstra_t;


typedef struct graph_node_data {
  int id; 
  int graph_id; //used for caching internal graph id's
} s_graph_node_data_t, * graph_node_data_t;

typedef struct graph_node_map_element {
  xbt_node_t node;
} s_graph_node_map_element_t, * graph_node_map_element_t;

typedef struct route_cache_element {
  int * pred_arr;
  int size;
} s_route_cache_element_t, * route_cache_element_t;	

/*
 * Free functions
 */
static void route_cache_elem_free(void *e) {
  route_cache_element_t elm=(route_cache_element_t)e;

  if (elm) {
    free(elm->pred_arr);
    free(elm);
  }
}

static void graph_node_map_elem_free(void *e) {
  graph_node_map_element_t elm = (graph_node_map_element_t)e;

  if(elm) {
    free(elm);
  }
}

/*
 * Utility functions
*/
static xbt_node_t route_graph_new_node(int id, int graph_id) {
  xbt_node_t node = NULL;
  graph_node_data_t data = NULL;
  graph_node_map_element_t elm = NULL;
  routing_dijkstra_t routing = (routing_dijkstra_t) used_routing;

  data = xbt_new0(struct graph_node_data, sizeof(struct graph_node_data));
  data->id = id;
  data->graph_id = graph_id;
  node = xbt_graph_new_node(routing->route_graph, data);

  elm = xbt_new0(struct graph_node_map_element, sizeof(struct graph_node_map_element));
  elm->node = node;
  xbt_dict_set_ext(routing->graph_node_map, (char*)(&id), sizeof(int), (xbt_set_elm_t)elm, &graph_node_map_elem_free);

  return node;
}

static graph_node_map_element_t graph_node_map_search(int id) {
  routing_dijkstra_t routing = (routing_dijkstra_t) used_routing;

  graph_node_map_element_t elm = (graph_node_map_element_t)xbt_dict_get_or_null_ext(routing->graph_node_map, (char*)(&id), sizeof(int));

  return elm;
}

/*
 * Parsing
 */
static void route_new_dijkstra(int src_id, int dst_id, void* link) {
  routing_dijkstra_t routing = (routing_dijkstra_t) used_routing;

  xbt_node_t src = NULL;
  xbt_node_t dst = NULL;
  graph_node_map_element_t src_elm = (graph_node_map_element_t)xbt_dict_get_or_null_ext(routing->graph_node_map, (char*)(&src_id), sizeof(int));
  graph_node_map_element_t dst_elm = (graph_node_map_element_t)xbt_dict_get_or_null_ext(routing->graph_node_map, (char*)(&dst_id), sizeof(int));

  if(src_elm)
    src = src_elm->node;

  if(dst_elm)
    dst = dst_elm->node;

  //add nodes if they don't exist in the graph
  if(src_id == dst_id && src == NULL && dst == NULL) {
    src = route_graph_new_node(src_id, -1);
    dst = src;
  } else {
    if(src == NULL) {
      src = route_graph_new_node(src_id, -1);
    }
	
    if(dst == NULL) {
      dst = route_graph_new_node(dst_id, -1);
    }
  }

  //add link as edge to graph
  xbt_graph_new_edge(routing->route_graph, src, dst, link);
  
}

static void add_loopback_dijkstra(void) {
  routing_dijkstra_t routing = (routing_dijkstra_t) used_routing;

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
			if(other_node == node) {
				found = 1;
				break;
			}
		}

		if(!found)
			xbt_graph_new_edge(routing->route_graph, node, node, &routing->loopback);
	}
}

static void routing_dijkstra_parse_end(void) {
  routing_dijkstra_t routing = (routing_dijkstra_t) used_routing;
  int nb_link = 0;
  xbt_dict_cursor_t cursor = NULL;
  char *key, *data, *end;
  const char *sep = "#";
  xbt_dynar_t links, keys;
  char* link_name = NULL;
  void* link = NULL;
  xbt_node_t node = NULL;
  unsigned int cursor2;
  xbt_dynar_t nodes = NULL;
  /* Create the topology graph */
  routing->route_graph = xbt_graph_new_graph(1, NULL);
  routing->graph_node_map = xbt_dict_new();
  routing->last_route = xbt_dynar_new(routing->size_of_link, NULL);
  if(routing->cached)
    routing->route_cache = xbt_dict_new();


  /* Put the routes in position */
  xbt_dict_foreach(route_table, cursor, key, data) {
    nb_link = 0;
    links = (xbt_dynar_t) data;
    keys = xbt_str_split_str(key, sep);

    src_id = strtol(xbt_dynar_get_as(keys, 0, char *), &end, 16);
    dst_id = strtol(xbt_dynar_get_as(keys, 1, char *), &end, 16);
    xbt_dynar_free(&keys);

    DEBUG4("Handle %d %d (from %d hosts): %ld links",
        src_id,dst_id,routing->generic_routing.host_count,xbt_dynar_length(links));

    xbt_assert3(xbt_dynar_length(links) == 1, "%ld links in route between host %d and %d, should be 1", xbt_dynar_length(links), src_id, dst_id);

    link_name = xbt_dynar_getfirst_as(links, char*);
    link = xbt_dict_get_or_null(surf_network_model->resource_set, link_name);
    if (link)
      route_new_dijkstra(src_id,dst_id,link);
    else
      THROW1(mismatch_error,0,"Link %s not found", link_name);
    
  }

  /* Add the loopback if needed */
  add_loopback_dijkstra();

  /* initialize graph indexes in nodes after graph has been built */
  nodes = xbt_graph_get_nodes(routing->route_graph);

  xbt_dynar_foreach(nodes, cursor2, node) {
    graph_node_data_t data = xbt_graph_node_get_data(node);
    data->graph_id = cursor2;
  }

}

/*
 * Business methods
 */
static xbt_dynar_t routing_dijkstra_get_route(int src_id,int dst_id) {

  routing_dijkstra_t routing = (routing_dijkstra_t) used_routing;
  int * pred_arr = NULL;
  int src_node_id = 0;
  int dst_node_id = 0;
  int * nodeid = NULL;
  int v;
  int size = 0;
  void * link = NULL;
  route_cache_element_t elm = NULL;
  xbt_dynar_t nodes = xbt_graph_get_nodes(routing->route_graph);

  /*Use the graph_node id mapping set to quickly find the nodes */
  graph_node_map_element_t src_elm = graph_node_map_search(src_id);
  graph_node_map_element_t dst_elm = graph_node_map_search(dst_id);
  xbt_assert2(src_elm != NULL && dst_elm != NULL, "src %d or dst %d does not exist", src_id, dst_id);
  src_node_id = ((graph_node_data_t)xbt_graph_node_get_data(src_elm->node))->graph_id;
  dst_node_id = ((graph_node_data_t)xbt_graph_node_get_data(dst_elm->node))->graph_id;

  if(routing->cached) {
    /*check if there is a cached predecessor list avail */
    elm = (route_cache_element_t)xbt_dict_get_or_null_ext(routing->route_cache, (char*)(&src_id), sizeof(int));
  }

  if(elm) { //cached mode and cache hit
    pred_arr = elm->pred_arr;
  } else { //not cached mode or cache miss
    double * cost_arr = NULL;
    xbt_heap_t pqueue = NULL;
    int i = 0;

    int nr_nodes = xbt_dynar_length(nodes);
    cost_arr = xbt_new0(double, nr_nodes); //link cost from src to other hosts
    pred_arr = xbt_new0(int, nr_nodes); //predecessors in path from src
    pqueue = xbt_heap_new(nr_nodes, free);

    //initialize
    cost_arr[src_node_id] = 0.0;

    for(i = 0; i < nr_nodes; i++) {
      if(i != src_node_id) {
        cost_arr[i] = DBL_MAX;
      }

      pred_arr[i] = 0;

      //initialize priority queue
      nodeid = xbt_new0(int, 1);
      *nodeid = i;
      xbt_heap_push(pqueue, nodeid, cost_arr[i]);

    }

    // apply dijkstra using the indexes from the graph's node array
    while(xbt_heap_size(pqueue) > 0) {
      int * v_id = xbt_heap_pop(pqueue);
      xbt_node_t v_node = xbt_dynar_get_as(nodes, *v_id, xbt_node_t);
      xbt_dynar_t out_edges = xbt_graph_node_get_outedges(v_node); 
      xbt_edge_t edge = NULL;
      unsigned int cursor;

      xbt_dynar_foreach(out_edges, cursor, edge) {
        xbt_node_t u_node = xbt_graph_edge_get_target(edge);
        graph_node_data_t data = xbt_graph_node_get_data(u_node);
        int u_id = data->graph_id;
        int cost_v_u = 1; //fixed link cost for now

        if(cost_v_u + cost_arr[*v_id] < cost_arr[u_id]) {
          pred_arr[u_id] = *v_id;
          cost_arr[u_id] = cost_v_u + cost_arr[*v_id];
          nodeid = xbt_new0(int, 1);
          *nodeid = u_id;
          xbt_heap_push(pqueue, nodeid, cost_arr[u_id]);
        }
      }

      //free item popped from pqueue
      free(v_id);
    }

    free(cost_arr);
    xbt_heap_free(pqueue);

  }

  //compose route path with links
  xbt_dynar_reset(routing->last_route);

  for(v = dst_node_id; v != src_node_id; v = pred_arr[v]) {
    xbt_node_t node_pred_v = xbt_dynar_get_as(nodes, pred_arr[v], xbt_node_t);
    xbt_node_t node_v = xbt_dynar_get_as(nodes, v, xbt_node_t);
    xbt_edge_t edge = xbt_graph_get_edge(routing->route_graph, node_pred_v, node_v);

    xbt_assert2(edge != NULL, "no route between host %d and %d", src_id, dst_id);

    link = xbt_graph_edge_get_data(edge);
    xbt_dynar_unshift(routing->last_route, &link);
    size++;
  }


  if(routing->cached && elm == NULL) {
    //add to predecessor list of the current src-host to cache
    elm = xbt_new0(struct route_cache_element, sizeof(struct route_cache_element));
    elm->pred_arr = pred_arr;
    elm->size = size;
    xbt_dict_set_ext(routing->route_cache, (char*)(&src_id), sizeof(int), (xbt_set_elm_t)elm, &route_cache_elem_free);
  }

  if(!routing->cached)
    free(pred_arr);

  return routing->last_route;
}


static void routing_dijkstra_finalize(void) {
  routing_dijkstra_t routing = (routing_dijkstra_t)used_routing;

  if (routing) {
    xbt_graph_free_graph(routing->route_graph, &free, NULL, &free);
    xbt_dict_free(&routing->graph_node_map);
    if(routing->cached)
      xbt_dict_free(&routing->route_cache);
    xbt_dynar_free(&routing->last_route);
    xbt_dict_free(&used_routing->host_id);
    free(routing);
    routing=NULL;
  }
}

static xbt_dict_t routing_dijkstraboth_get_onelink_routes(void){
  xbt_assert0(0,"The get_onelink_routes feature is not supported in routing model dijkstraboth");
}

static int routing_dijkstraboth_is_router(int id){
  xbt_assert0(0,"The get_is_router feature is not supported in routing model dijkstraboth");
}

/*
 *
 */
static void routing_model_dijkstraboth_create(size_t size_of_link,void *loopback, int cached) {
  /* initialize our structure */
  routing_dijkstra_t routing = xbt_new0(s_routing_dijkstra_t,1);
  routing->generic_routing.name = "Dijkstra";
  routing->generic_routing.host_count = 0;
  routing->generic_routing.get_route = routing_dijkstra_get_route;
  routing->generic_routing.get_onelink_routes = routing_dijkstraboth_get_onelink_routes;
  routing->generic_routing.is_router = routing_dijkstraboth_is_router;
  routing->generic_routing.finalize = routing_dijkstra_finalize;
  routing->size_of_link = size_of_link;
  routing->loopback = loopback;
  routing->cached = cached;

  /* Set it in position */
  used_routing = (routing_t) routing;

  /* Setup the parsing callbacks we need */
  routing->generic_routing.host_id = xbt_dict_new();
  surfxml_add_callback(STag_surfxml_host_cb_list, &routing_full_parse_Shost);
  surfxml_add_callback(ETag_surfxml_platform_cb_list, &routing_dijkstra_parse_end);
  surfxml_add_callback(STag_surfxml_route_cb_list,
      &routing_full_parse_Sroute_set_endpoints);
  surfxml_add_callback(ETag_surfxml_route_cb_list, &routing_full_parse_Eroute);
  surfxml_add_callback(STag_surfxml_cluster_cb_list, &routing_shortest_path_parse_Scluster);
}

static void routing_model_dijkstra_create(size_t size_of_link,void *loopback) {
  routing_model_dijkstraboth_create(size_of_link, loopback, 0);
}

static void routing_model_dijkstracache_create(size_t size_of_link,void *loopback) {
  routing_model_dijkstraboth_create(size_of_link, loopback, 1);
}

/* ************************************************** */
/* ********** NO ROUTING **************************** */


static void routing_none_finalize(void) {
  if (used_routing) {
    xbt_dict_free(&used_routing->host_id);
    free(used_routing);
    used_routing=NULL;
  }
}

static void routing_model_none_create(size_t size_of_link,void *loopback) {
  routing_t routing = xbt_new0(s_routing_t,1);
  routing->name = "none";
  routing->host_count = 0;
  routing->host_id = xbt_dict_new();
  routing->get_onelink_routes = NULL;
  routing->is_router = NULL;
  routing->get_route = NULL;
  routing->finalize = routing_none_finalize;

  /* Set it in position */
  used_routing = (routing_t) routing;
}
  
/* ************************************************************************** */
/* *********************** HIERARCHICAL ROUTING ***************************** */

#define ROUTE_TABLE_HOST(dest,i,j)     (((*dest)->routing_table)[(i)+(j)*((*dest)->host_count)])
#define ROUTE_H_TABLE_HOST(dest,i,j) (((*dest)->h_routing_table)[(i)+(j)*((*dest)->host_count)])
#define ROUTE_V_TABLE_HOST(dest,i,j) (((*dest)->v_routing_table)[(i)+(j)*((*dest)->router_count)])
#define ROUTE_TABLE_AS(dest,i,j)       (((*dest)->routing_table)[(i)+(j)*((*dest)->routetree_list->used)])
#define ROUTE_H_TABLE_AS(dest,i,j)   (((*dest)->h_routing_table)[(i)+(j)*((*dest)->routetree_list->used)])
#define ROUTE_V_TABLE_AS(dest,i,j)   (((*dest)->v_routing_table)[(i)+(j)*((*dest)->router_count)])

#define H2ID(rt,val)  (val-((*rt)->host_offset))
#define R2ID(rt,val)  (val-((*rt)->router_offset))
#define A2ID(val)     ((*val)->id_father)

#define HVALID(rt,val)  (          ((*rt)->host_count)>H2ID(rt,val) && H2ID(rt,val)=>0) 
#define RVALID(rt,val)  (        ((*rt)->router_count)>R2ID(rt,val) && R2ID(rt,val)=>0) 
#define AVALID(rt,val)  (((*rt)->routetree_list->used)>A2ID(val)    &&    A2ID(val)=>0) 

#define ROUTE_HIERARCHICAL_HH(src,dst,rt)   (((*rt)->routing_table)[H2ID(rt,src)+H2ID(rt,dst)*((*rt)->host_count)]) 
#define ROUTE_HIERARCHICAL_HR(src,dst,rt) (((*rt)->h_routing_table)[H2ID(rt,src)+R2ID(rt,dst)*((*rt)->host_count)]) 
#define ROUTE_HIERARCHICAL_RH(src,dst,rt) (((*rt)->v_routing_table)[R2ID(rt,src)+H2ID(rt,dst)*((*rt)->router_count)]) 
#define ROUTE_HIERARCHICAL_AA(src,dst,rt)   (((*rt)->routing_table)[A2ID(   src)+A2ID(   dst)*((*rt)->routetree_list->used)]) 
#define ROUTE_HIERARCHICAL_AR(src,dst,rt) (((*rt)->h_routing_table)[A2ID(   src)+R2ID(rt,dst)*((*rt)->routetree_list->used)]) 
#define ROUTE_HIERARCHICAL_RA(src,dst,rt) (((*rt)->v_routing_table)[R2ID(rt,src)+A2ID(   dst)*((*rt)->router_count)]) 

#define SRC_DST_HIERARCHICAL_AA(src,dst,rt)   ((routelimits_t)&(((*rt)->src_dst_table)[A2ID(   src)+A2ID(   dst)*((*rt)->routetree_list->used)]))
#define SRC_DST_HIERARCHICAL_AR(src,dst,rt) ((routelimits_t)&(((*rt)->h_src_dst_table)[A2ID(   src)+R2ID(rt,dst)*((*rt)->routetree_list->used)]))
#define SRC_DST_HIERARCHICAL_RA(src,dst,rt) ((routelimits_t)&(((*rt)->v_src_dst_table)[R2ID(rt,src)+A2ID(   dst)*((*rt)->router_count)]))

/* for the calculus of locals ids */
int local_host_count;
int local_router_count;

/*####[ Type definition ]####################################################*/

typedef struct routelimits_ {
    int src_id;         /* source router  */
    int dst_id;         /* destine router */
} s_routelimits_t, *routelimits_t;

typedef struct routetree_ {
    const char* name;              /* name of the set */
    struct routetree_* father;     /* father set */
    int id_father;                 /* id in the father set */
    int host_count;                /* count of host in the set */
    int router_count;              /* count of router in the set */
    int host_offset;               /* offset for the host id */
    int router_offset;             /* offset for the router id */
    xbt_dynar_t routetree_list;    /* other sets in the set */
    xbt_dynar_t *routing_table;    /* host-host or AS-AS */
    xbt_dynar_t *h_routing_table;  /* host-router or AS-router */
    xbt_dynar_t *v_routing_table;  /* router-host or router-AS */
    routelimits_t src_dst_table;   /* src and dst for routing table */
    routelimits_t h_src_dst_table; /* src and dst for h routing table */
    routelimits_t v_src_dst_table; /* src and dst for v routing table */
} s_routetree_t, *routetree_t;

typedef struct routing_hierarchical_t {
  s_routing_t generic_routing;
  routetree_t routetree;
  xbt_dynar_t host_routetree; /* routetree_t* */
  xbt_dynar_t last_route;     /* store the last routing path */
  void *loopback;
  size_t size_of_link;
} s_routing_hierarchical_t,*routing_hierarchical_t;

/*####[ routetree functions ]################################################*/

/* @brief Constructor */
routetree_t routetree_new(void) {
  routetree_t res = xbt_new(s_routetree_t, 1);
  res->name             = NULL;
  res->father           = NULL;
  res->id_father        = 0;
  res->host_count       = 0;
  res->router_count     = 0;
  res->host_offset      = 0;
  res->router_offset    = 0;
  res->routetree_list   = NULL;
  res->routing_table    = NULL;
  res->h_routing_table  = NULL;
  res->v_routing_table  = NULL;
  res->src_dst_table   = NULL;
  res->v_src_dst_table = NULL;
  res->h_src_dst_table = NULL;
  return res;
}

/* @brief Destructor */
void routetree_free(routetree_t* routetree) {
  unsigned int it;
  int i,j,ni,nj;
  int size_host_list = 0, size_router_list = 0, size_routetree_list = 0;
  int size_of_link = ((routing_hierarchical_t)used_routing)->size_of_link;
  routetree_t elem;

  if (*routetree) {
    size_host_list = (*routetree)->host_count;
    size_router_list = (*routetree)->router_count;
    if( (*routetree)->routetree_list ) size_routetree_list = ((*routetree)->routetree_list)->used;
  
    if( size_host_list == 0 && size_routetree_list == 0 ) {
      return;
    }
    else if( size_host_list > 0 && size_routetree_list == 0 ) {
      /* create a host table */
      ni = size_host_list;
      nj = size_host_list;
      if(ni*nj > 1) {
       for (i=0;i<ni;i++)
         for (j=0;j<nj;j++)
             if( &ROUTE_TABLE_HOST(routetree,i,j) )
               xbt_dynar_free( &ROUTE_TABLE_HOST(routetree,i,j) );
      xbt_free( (*routetree)->routing_table );
      }
      /* create a host vs router table */
      ni = size_host_list;
      nj = size_router_list;
      if(ni*nj) {
      for (i=0;i<ni;i++)
        for (j=0;j<nj;j++)
             if( &ROUTE_H_TABLE_HOST(routetree,i,j) )
               xbt_dynar_free( &ROUTE_H_TABLE_HOST(routetree,i,j) );
      xbt_free( (*routetree)->h_routing_table );
      }
      /* create a router vs host table */
      ni = size_router_list;
      nj = size_host_list;
      if(ni*nj) {
      for (i=0;i<ni;i++)
        for (j=0;j<nj;j++)
             if( &ROUTE_V_TABLE_HOST(routetree,i,j) )
               xbt_dynar_free( &ROUTE_V_TABLE_HOST(routetree,i,j) );
      xbt_free( (*routetree)->v_routing_table );
      }
    }
    else if( size_host_list == 0 && size_routetree_list > 0 ) {
      /* create a host table */
      ni = size_routetree_list;
      nj = size_routetree_list;
      if(ni*nj > 1) {
      for (i=0;i<ni;i++)
        for (j=0;j<nj;j++)
             if( &ROUTE_TABLE_AS(routetree,i,j) )
               xbt_dynar_free( &ROUTE_TABLE_AS(routetree,i,j) );
      xbt_free( (*routetree)->src_dst_table );
      xbt_free( (*routetree)->routing_table );
      }
      /* create a host vs router table */
      ni = size_routetree_list;
      nj = size_router_list;
      if(ni*nj) {
      for (i=0;i<ni;i++)
        for (j=0;j<nj;j++)
             if( &ROUTE_H_TABLE_AS(routetree,i,j) )
               xbt_dynar_free( &ROUTE_H_TABLE_AS(routetree,i,j) );
      xbt_free( (*routetree)->h_src_dst_table );
      xbt_free( (*routetree)->h_routing_table );
      }
      /* create a router vs host table */
      ni = size_router_list;
      nj = size_routetree_list;
      if(ni*nj) {
      for (i=0;i<ni;i++)
        for (j=0;j<nj;j++)
             if( &ROUTE_V_TABLE_AS(routetree,i,j) )
               xbt_dynar_free( &ROUTE_V_TABLE_AS(routetree,i,j) );
      xbt_free( (*routetree)->v_src_dst_table );
      xbt_free( (*routetree)->v_routing_table );
      }
    }
    else {
      xbt_assert0(0,"The autonomous system must have hosts or autonomous systems, not the both");
    }
    
    if ((*routetree)->routetree_list) {
      xbt_dynar_foreach((*routetree)->routetree_list,it,elem) {
         routetree_free(&elem);}
      xbt_dynar_free(&((*routetree)->routetree_list));
    }
    free(*routetree);    
    *routetree = NULL;

  }
}

/* @brief Add a host to routetree */
void routetree_add_host(const char* name, routetree_t* dest) {
  int* val;
  val = xbt_malloc(sizeof(int)); 
  /* count the new host */
  //*val = used_routing->host_count++;
  *val = local_host_count++;
  /* add the host to the dictonary */
  xbt_dict_set(used_routing->host_id,name,val,xbt_free);
  /* add the host to the routetree traduction vector */
  xbt_dynar_push(((routing_hierarchical_t)used_routing)->host_routetree,dest);
  /* add a local count for the routetree */
  (*dest)->host_count++;
}

/* @brief Add a router to routetree */
void routetree_add_router(const char* name, routetree_t* dest) {
  int* val;
  val = xbt_malloc(sizeof(int)); 
  /* count the new host */
  //*val = HOST2ROUTER(used_routing->router_count++);
  *val = HOST2ROUTER(local_router_count++);
  /* add the host to the dictonary */
  xbt_dict_set(used_routing->host_id,name,val,xbt_free);
  /* add a local count for the routetree */
  (*dest)->router_count++;
}

/* @brief Add a child routetree to routetree father */
void routetree_add(routetree_t* dest, routetree_t* elem) {
  xbt_dynar_push((*dest)->routetree_list, elem);
}

void routetree_fill(routetree_t* dest, const char* name, routetree_t father, int id_father) {
  (*dest)->name             = name;
  (*dest)->father           = father;
  (*dest)->id_father        = id_father;
  (*dest)->host_count       = 0;
  (*dest)->router_count     = 0;
  (*dest)->host_offset      = local_host_count;
  (*dest)->router_offset    = local_router_count;
  (*dest)->routetree_list   = xbt_dynar_new(sizeof(routetree_t), NULL);
  (*dest)->routing_table    = NULL;
  (*dest)->v_routing_table  = NULL;
  (*dest)->h_routing_table  = NULL;
  (*dest)->src_dst_table    = NULL;
  (*dest)->v_src_dst_table  = NULL;
  (*dest)->h_src_dst_table  = NULL;
}

/* @brief Make the routing tables for a empty routetree */
void routetree_fill_routing_table(routetree_t* dest) {

  int i,j,ni,nj;
  int size_host_list = 0, size_router_list = 0, size_routetree_list = 0;
  int size_of_link = ((routing_hierarchical_t)used_routing)->size_of_link;
  size_host_list = (*dest)->host_count;
  size_router_list = (*dest)->router_count;
  if( (*dest)->routetree_list ) size_routetree_list = ((*dest)->routetree_list)->used;
  
  if( size_host_list == 0 && size_routetree_list == 0 ) {
    return;
  }
  else if( size_host_list > 0 && size_routetree_list == 0 ) {
    /* create a host table */
    ni = size_host_list;
    nj = size_host_list;
    if(ni*nj) {
    (*dest)->routing_table = xbt_new0(xbt_dynar_t, ni * nj);
    for (i=0;i<ni;i++)
      for (j=0;j<nj;j++)
          ROUTE_TABLE_HOST(dest,i,j) = xbt_dynar_new(size_of_link,NULL);
    }
    /* create a host vs router table */
    ni = size_host_list;
    nj = size_router_list;
    if(ni*nj) {
    (*dest)->h_routing_table = xbt_new0(xbt_dynar_t, ni * nj);
    for (i=0;i<ni;i++)
      for (j=0;j<nj;j++)
          ROUTE_H_TABLE_HOST(dest,i,j) = xbt_dynar_new(size_of_link,NULL);
    }
    /* create a router vs host table */
    ni = size_router_list;
    nj = size_host_list;
    if(ni*nj) {
    (*dest)->v_routing_table = xbt_new0(xbt_dynar_t, ni * nj);
    for (i=0;i<ni;i++)
      for (j=0;j<nj;j++)
          ROUTE_V_TABLE_HOST(dest,i,j) = xbt_dynar_new(size_of_link,NULL);
    }
  }
  else if( size_host_list == 0 && size_routetree_list > 0 ) {
    /* create a host table */
    ni = size_routetree_list;
    nj = size_routetree_list;
    if(ni*nj > 1) {
    (*dest)->src_dst_table = xbt_new0(s_routelimits_t, ni * nj);
    (*dest)->routing_table = xbt_new0(xbt_dynar_t, ni * nj);
    for (i=0;i<ni;i++)
      for (j=0;j<nj;j++)
        ROUTE_TABLE_AS(dest,i,j) = xbt_dynar_new(size_of_link,NULL);
    }
    /* create a host vs router table */
    ni = size_routetree_list;
    nj = size_router_list;
    if(ni*nj) {
    (*dest)->h_src_dst_table = xbt_new0(s_routelimits_t, ni * nj);
    (*dest)->h_routing_table = xbt_new0(xbt_dynar_t, ni * nj);
    for (i=0;i<ni;i++)
      for (j=0;j<nj;j++)
          ROUTE_H_TABLE_AS(dest,i,j) = xbt_dynar_new(size_of_link,NULL);
    }
    /* create a router vs host table */
    ni = size_router_list;
    nj = size_routetree_list;
    if(ni*nj) {
    (*dest)->v_src_dst_table = xbt_new0(s_routelimits_t, ni * nj);
    (*dest)->v_routing_table = xbt_new0(xbt_dynar_t, ni * nj);
    for (i=0;i<ni;i++)
      for (j=0;j<nj;j++)
          ROUTE_V_TABLE_AS(dest,i,j) = xbt_dynar_new(size_of_link,NULL);
    }
  }
  else {
    xbt_assert0(0,"The autonomous system must have hosts or autonomous systems, not the both");
  }
}

////////////////////////////
// PARSER CODE HERE
////////////////////////////


/////////////
// is coming ...	
/////////////


/*
 * Business methods
 */
static xbt_dynar_t routing_hierarchical_get_route(int src,int dst) {

  routing_hierarchical_t routing = (routing_hierarchical_t)used_routing;
  routetree_t* rt_src = NULL;
  routetree_t* rt_dst = NULL;
  routetree_t* actual = NULL;
  xbt_dynar_t path_src = NULL;
  xbt_dynar_t path_dst = NULL;
  xbt_dynar_t path_first = NULL;
  xbt_dynar_t path_last = NULL;
  int cpt = 0;
  int *val;
  int index_src;
  int index_dst;
  int index_father_src;
  int index_father_dst;
  int actual_router_src;
  int actual_router_dst;
  routetree_t* actual_src;
  routetree_t* actual_dst;
  routetree_t* actual_src_father;
  routetree_t* actual_dst_father;
  routetree_t* father;
  xbt_dynar_t result;
  xbt_dynar_t path_tmp_links;
  xbt_dynar_t path_center_links;
  xbt_dynar_t path_src_links;
  xbt_dynar_t path_dst_links;
  void* link;
  
  xbt_dynar_reset(routing->last_route);
  
  // (0) test for routers ids
  xbt_assert0(!(ISROUTER(src) || ISROUTER(dst)), "Ask for route \"from\" or \"to\" a router node");
  
  // (1) find the as-routetree where the src and dst are located
  rt_src = xbt_dynar_get_ptr(routing->host_routetree,src);
  rt_dst = xbt_dynar_get_ptr(routing->host_routetree,dst);
 
  // (2) find the path to the root routetree
  path_src = xbt_dynar_new(sizeof(routetree_t), NULL);
  actual = rt_src; 
  while( *actual != NULL ) {
    xbt_dynar_push(path_src,actual);
    actual = &((*actual)->father);
  }
  path_dst = xbt_dynar_new(sizeof(routetree_t), NULL);
  actual = rt_dst; 
  while( *actual != NULL ) {
    xbt_dynar_push(path_dst,actual);
    actual = &((*actual)->father);
  }
  
  // (3) find the common father
  index_src  = (path_src->used)-1;
  index_dst  = (path_dst->used)-1;
  actual_src = xbt_dynar_get_ptr(path_src,index_src);
  actual_dst = xbt_dynar_get_ptr(path_dst,index_dst);
  while( index_src >= 0 && index_dst >= 0 && *actual_src == *actual_dst ) {
    actual_src = xbt_dynar_get_ptr(path_src,index_src);
    actual_dst = xbt_dynar_get_ptr(path_dst,index_dst);
    index_src--;
    index_dst--;
  }
  index_src++;
  index_dst++;
  actual_src = xbt_dynar_get_ptr(path_src,index_src);
  actual_dst = xbt_dynar_get_ptr(path_dst,index_dst);
  
  // (4) if they are in the same routetree?
  if( *actual_src == *actual_dst ) {
    
    // (5.1) belong to the same routetree, are in the same routing table
    // even if are the same host, src and dst
    path_tmp_links = ROUTE_HIERARCHICAL_HH(src,dst,actual_src);
    cpt = 0;
    xbt_dynar_foreach(path_tmp_links, cpt, link) {
      xbt_dynar_push(routing->last_route,&link);
    }
    
  }
  else {
    
    // (5.2) they are not in the same routetree, make the path
    index_father_src = index_src+1;
    index_father_dst = index_dst+1;
    father = xbt_dynar_get_ptr(path_src,index_father_src);
    path_center_links = ROUTE_HIERARCHICAL_AA(actual_src,actual_dst,father);
    routelimits_t limits = SRC_DST_HIERARCHICAL_AA(actual_src,actual_dst,father);
 
    // (5.2.1) make the source path
     path_src_links = xbt_dynar_new(routing->size_of_link, NULL);
     actual_router_dst = ROUTER2HOST(limits->src_id); /* first router in the reverse path */
     actual_src_father = xbt_dynar_get_ptr(path_src,index_src);
     index_src--;
     for(index_src;index_src>=0;index_src--) {
       actual_src = xbt_dynar_get_ptr(path_src,index_src);
       path_tmp_links = ROUTE_HIERARCHICAL_AR(actual_src,actual_router_dst,actual_src_father);
       cpt = 0;
       xbt_dynar_foreach(path_tmp_links, cpt, link) {
	 xbt_assert2(link!=NULL,"NULL link in the route from src %d to dst %d (5.2.1)", src, dst);
         xbt_dynar_push(path_src_links,&link);
       }
       actual_router_dst = ROUTER2HOST(SRC_DST_HIERARCHICAL_AR(actual_src,actual_router_dst,actual_src_father)->src_id);
       actual_src_father = actual_src;
     }

    // (5.2.2) make the destination path
     path_dst_links = xbt_dynar_new(routing->size_of_link, NULL);
     actual_router_src = ROUTER2HOST(limits->dst_id); /* first router for the path */
     actual_dst_father = xbt_dynar_get_ptr(path_dst,index_dst);
     index_dst--;
     for(index_dst;index_dst>=0;index_dst--) {
       actual_dst = xbt_dynar_get_ptr(path_dst,index_dst);
       path_tmp_links = ROUTE_HIERARCHICAL_RA(actual_router_src,actual_dst,actual_dst_father);
       cpt = 0;
       xbt_dynar_foreach(path_tmp_links, cpt, link) {
	 xbt_assert2(link!=NULL,"NULL link in the route from src %d to dst %d (5.2.2)", src, dst);
         xbt_dynar_push(path_dst_links,&link);
       }
       actual_router_src = ROUTER2HOST(SRC_DST_HIERARCHICAL_RA(actual_router_src,actual_dst,actual_dst_father)->dst_id);
       actual_dst_father = actual_dst;
     }
    
    // (5.2.3) the first part of the route
    path_first = ROUTE_HIERARCHICAL_HR(src,actual_router_dst,actual_src);
    
    // (5.2.4) the last part of the route
    path_last = ROUTE_HIERARCHICAL_RH(actual_router_src,dst,actual_dst);
    
    // (5.2.5) make all paths together 
    // FIXME: the paths non respect the right order
    
    cpt = 0;
    xbt_dynar_foreach(path_first, cpt, link) {
      xbt_assert2(link!=NULL,"NULL link in the route from src %d to dst %d", src, dst);
      xbt_dynar_push(routing->last_route,&link);
    }
    cpt = 0;
    xbt_dynar_foreach(path_src_links, cpt, link) {
      xbt_assert2(link!=NULL,"NULL link in the route from src %d to dst %d", src, dst);
      xbt_dynar_push(routing->last_route,&link);
    }
    cpt = 0;
    xbt_dynar_foreach(path_center_links, cpt, link) {
      xbt_assert2(link!=NULL,"NULL link in the route from src %d to dst %d", src, dst);
      xbt_dynar_push(routing->last_route,&link);
    }
    cpt = 0;
    xbt_dynar_foreach(path_dst_links, cpt, link) {
      xbt_assert2(link!=NULL,"NULL link in the route from src %d to dst %d", src, dst);
      xbt_dynar_push(routing->last_route,&link);
    }
    cpt = 0;
    xbt_dynar_foreach(path_last, cpt, link) {
      xbt_assert2(link!=NULL,"NULL link in the route from src %d to dst %d", src, dst);
      xbt_dynar_push(routing->last_route,&link);
    }
    
    xbt_dynar_free(&path_src_links);
    xbt_dynar_free(&path_dst_links);
  }    
 
  xbt_dynar_free(&path_src);
  xbt_dynar_free(&path_dst);

//   cpt=0;
//   xbt_dynar_foreach(routing->last_route, cpt, link)
//   { s_surf_resource_t* generic_resource = link; 
//   printf("< LINK > link %s\n",generic_resource->name); }
  
  return(routing->last_route);
}

static xbt_dict_t routing_hierarchical_get_onelink_routes(void){
  xbt_assert0(0,"The get_onelink_routes feature is not supported in routing model Hierarchical");
}

static int routing_hierarchical_is_router(int id){
   return ISROUTER(id);
}

static void routing_hierarchical_finalize(void) {
  routing_hierarchical_t routing = (routing_hierarchical_t)used_routing;
  if(routing) {    
    xbt_dict_free(&used_routing->host_id);
    routetree_free(&(routing->routetree));
    xbt_dynar_free(&(routing->host_routetree));
    xbt_dynar_free(&routing->last_route);
    xbt_free(routing);
    routing=NULL;
  }
}

// void print_tree(routetree_t start)
// {
//   int data;
//   unsigned int it;
//   routetree_t elem;
//   
//   printf("Nombre %s - hosts %i - routers %i - as %i\n",
// 	 start->name,
// 	 (int)start->host_count,
// 	 (int)start->router_count,
// 	 (int)start->routetree_list->used);
// /*
//   printf("hosts:");
//   xbt_dynar_foreach(start->host_list,it,data) {
//     printf(" %i",data);
//   }
//   printf("\n");
// 
//   printf("routers:");
//   xbt_dynar_foreach(start->router_list,it,data) {
//     printf(" %i",ROUTER2HOST(data));    
//   }
//   printf("\n");
//   */
//   xbt_dynar_foreach(start->routetree_list,it,elem) {
//     print_tree(elem);
//   }
// }
  
/* erase me */
// The following function makes a example as if a paser program would do it.
static void example() {
  
  int nb_link = 0;
  unsigned int cpt = 0;
  xbt_dict_cursor_t cursor = NULL;
  char *key, *data, *end;
  const char *sep = "#";
  xbt_dynar_t links, keys;
  char *link_name = NULL;

  routetree_t tmp1,tmp2,tmp3,tmp4,tmp5; // erase me
  int *val;                             // erase me
  int i,j,n;                            // erase me
  void* link;                           // erase me
  
routing_hierarchical_t routing = (routing_hierarchical_t)used_routing;
  
/////////// fixed network /////////////

/////////// make the routetree tree /////////////

	routing->routetree = routetree_new();
	routetree_fill(&(routing->routetree),"as1",NULL,0);
	
		tmp2 = routetree_new();
		routetree_fill(&(tmp2),"as2",(routing->routetree),0);
		//hosts
		//router
		routetree_add_router("R21",&tmp2);
		routetree_add_router("R22",&tmp2);
		// add
		routetree_add(&(routing->routetree),&tmp2);

			tmp3 = routetree_new();
			routetree_fill(&(tmp3),"as3",(tmp2),0);
			//hosts
			routetree_add_host("A",&tmp3);
			routetree_add_host("B",&tmp3);
			//router
			routetree_add_router("R31",&tmp3);
			routetree_add_router("R32",&tmp3);
			// add
			routetree_add(&(tmp2),&tmp3);

		tmp4 = routetree_new();
		routetree_fill(&(tmp4),"as4",(routing->routetree),1);
		//hosts
		routetree_add_host("C",&tmp4);
		routetree_add_host("D",&tmp4);
		routetree_add_host("E",&tmp4);
		//router
		routetree_add_router("R41",&tmp4);
		routetree_add_router("R42",&tmp4);
		// add
		routetree_add(&(routing->routetree),&tmp4);

		tmp5 = routetree_new();
		routetree_fill(&(tmp5),"as5",(routing->routetree),2);
		//hosts
		routetree_add_host("F",&tmp5);
		//router
		routetree_add_router("R51",&tmp5);
		// add
		routetree_add(&(routing->routetree),&tmp5);

/////////// fill the routing tables /////////////

  routetree_fill_routing_table(&(routing->routetree));
  routetree_fill_routing_table(&(tmp2));
  routetree_fill_routing_table(&(tmp3));
  routetree_fill_routing_table(&(tmp4));
  routetree_fill_routing_table(&(tmp5));

/////////// fill the routing tables with the routes /////////////

  // as2 - as4
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "0"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_AA(&tmp2,&tmp4,&(routing->routetree)),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "1"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_AA(&tmp2,&tmp4,&(routing->routetree)),&link);
  SRC_DST_HIERARCHICAL_AA(&tmp2,&tmp4,&(routing->routetree))->src_id = HOST2ROUTER(0); //r21
  SRC_DST_HIERARCHICAL_AA(&tmp2,&tmp4,&(routing->routetree))->dst_id = HOST2ROUTER(4); //r41
  // as4 - as2
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "0"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_AA(&tmp4,&tmp2,&(routing->routetree)),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "1"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_AA(&tmp4,&tmp2,&(routing->routetree)),&link);
  SRC_DST_HIERARCHICAL_AA(&tmp4,&tmp2,&(routing->routetree))->src_id = HOST2ROUTER(4); //r41
  SRC_DST_HIERARCHICAL_AA(&tmp4,&tmp2,&(routing->routetree))->dst_id = HOST2ROUTER(0); //r21

  // as2 - as5  
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "2"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_AA(&tmp2,&tmp5,&(routing->routetree)),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "3"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_AA(&tmp2,&tmp5,&(routing->routetree)),&link);
  SRC_DST_HIERARCHICAL_AA(&tmp2,&tmp5,&(routing->routetree))->src_id = HOST2ROUTER(1); //r22
  SRC_DST_HIERARCHICAL_AA(&tmp2,&tmp5,&(routing->routetree))->dst_id = HOST2ROUTER(6); //r51
  // as5 - as2
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "2"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_AA(&tmp5,&tmp2,&(routing->routetree)),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "3"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_AA(&tmp5,&tmp2,&(routing->routetree)),&link);
  SRC_DST_HIERARCHICAL_AA(&tmp5,&tmp2,&(routing->routetree))->src_id = HOST2ROUTER(6); //r51
  SRC_DST_HIERARCHICAL_AA(&tmp5,&tmp2,&(routing->routetree))->dst_id = HOST2ROUTER(1); //r22
  
  // as4 - as5
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "4"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_AA(&tmp4,&tmp5,&(routing->routetree)),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "5"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_AA(&tmp4,&tmp5,&(routing->routetree)),&link);
  SRC_DST_HIERARCHICAL_AA(&tmp4,&tmp5,&(routing->routetree))->src_id = HOST2ROUTER(5); //r42
  SRC_DST_HIERARCHICAL_AA(&tmp4,&tmp5,&(routing->routetree))->dst_id = HOST2ROUTER(6); //r51
  // as5 - as4
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "4"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_AA(&tmp5,&tmp4,&(routing->routetree)),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "5"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_AA(&tmp5,&tmp4,&(routing->routetree)),&link);
  SRC_DST_HIERARCHICAL_AA(&tmp5,&tmp4,&(routing->routetree))->src_id = HOST2ROUTER(6); //r51
  SRC_DST_HIERARCHICAL_AA(&tmp5,&tmp4,&(routing->routetree))->dst_id = HOST2ROUTER(5); //r42

// tmp2

  // as3 - r21
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "5"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_AR(&tmp3,0,&tmp2),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "2"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_AR(&tmp3,0,&tmp2),&link);
  SRC_DST_HIERARCHICAL_AR(&tmp3,0,&tmp2)->src_id = HOST2ROUTER(2); //r31
  SRC_DST_HIERARCHICAL_AR(&tmp3,0,&tmp2)->dst_id = HOST2ROUTER(0); //r21
  // r21 - as3
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "5"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_RA(0,&tmp3,&tmp2),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "2"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_RA(0,&tmp3,&tmp2),&link);
  SRC_DST_HIERARCHICAL_RA(0,&tmp3,&tmp2)->src_id = HOST2ROUTER(0); //r21
  SRC_DST_HIERARCHICAL_RA(0,&tmp3,&tmp2)->dst_id = HOST2ROUTER(2); //r31
  
  // as3 - r22
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "3"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_AR(&tmp3,1,&tmp2),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "1"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_AR(&tmp3,1,&tmp2),&link);
  SRC_DST_HIERARCHICAL_AR(&tmp3,1,&tmp2)->src_id = HOST2ROUTER(3); //r32
  SRC_DST_HIERARCHICAL_AR(&tmp3,1,&tmp2)->dst_id = HOST2ROUTER(1); //r22
  // r22 - as3
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "3"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_RA(1,&tmp3,&tmp2),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "1"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_RA(1,&tmp3,&tmp2),&link);
  SRC_DST_HIERARCHICAL_RA(1,&tmp3,&tmp2)->src_id = HOST2ROUTER(1); //r22
  SRC_DST_HIERARCHICAL_RA(1,&tmp3,&tmp2)->dst_id = HOST2ROUTER(3); //r32
  
// tmp3

  // a - b
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "7"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HH(0,1,&tmp3),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "2"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HH(0,1,&tmp3),&link);
  // b - a
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "7"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HH(1,0,&tmp3),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "2"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HH(1,0,&tmp3),&link);
  
  // a - r31
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "5"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HR(0,2,&tmp3),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "1"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HR(0,2,&tmp3),&link);
  // a - r32
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "8"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HR(0,3,&tmp3),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "7"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HR(0,3,&tmp3),&link);
  // b - r31
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "3"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HR(1,2,&tmp3),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "2"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HR(1,2,&tmp3),&link);
  // b - r32
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "9"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HR(1,3,&tmp3),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "3"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HR(1,3,&tmp3),&link);  
  
   // r31 - a
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "5"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_RH(2,0,&tmp3),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "1"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_RH(2,0,&tmp3),&link);
  // r32 - a
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "8"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_RH(3,1,&tmp3),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "7"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_RH(3,1,&tmp3),&link);
  // r31 - b
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "3"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_RH(2,0,&tmp3),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "2"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_RH(2,0,&tmp3),&link);
  // r32 - b
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "9"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_RH(3,1,&tmp3),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "3"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_RH(3,1,&tmp3),&link);  
  
// tmp4

  // c - d
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "2"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HH(2,3,&tmp4),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "7"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HH(2,3,&tmp4),&link);
  // d - c
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "2"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HH(3,2,&tmp4),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "7"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HH(3,2,&tmp4),&link);
  // c - e
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "6"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HH(2,4,&tmp4),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "5"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HH(2,4,&tmp4),&link);
  // e - c
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "6"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HH(4,2,&tmp4),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "5"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HH(4,2,&tmp4),&link);  
  // d - e
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "5"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HH(3,4,&tmp4),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "3"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HH(3,4,&tmp4),&link);
  // e - d
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "5"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HH(4,3,&tmp4),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "3"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HH(4,3,&tmp4),&link);
  
  // c - r41
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "7"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HR(2,4,&tmp4),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "9"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HR(2,4,&tmp4),&link);
  // d - r41
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "5"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HR(3,4,&tmp4),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "8"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HR(3,4,&tmp4),&link);
  // e - r41
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "8"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HR(4,4,&tmp4),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "0"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HR(4,4,&tmp4),&link);
  // c - r42
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "6"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HR(2,5,&tmp4),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "5"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HR(2,5,&tmp4),&link);
  // d - r42
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "2"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HR(3,5,&tmp4),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "1"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HR(3,5,&tmp4),&link);
  // e - r42
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "9"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HR(4,5,&tmp4),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "4"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HR(4,5,&tmp4),&link);
  
  // r41 - c
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "7"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_RH(4,2,&tmp4),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "9"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_RH(4,2,&tmp4),&link);
  // r41 - d
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "5"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_RH(4,3,&tmp4),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "8"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_RH(4,3,&tmp4),&link);
  // r41 - e
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "8"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_RH(4,4,&tmp4),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "0"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_RH(4,4,&tmp4),&link);
  // r42 - c
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "6"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_RH(5,2,&tmp4),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "5"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_RH(5,2,&tmp4),&link);
  // r42 - d
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "2"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_RH(5,3,&tmp4),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "1"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_RH(5,3,&tmp4),&link);
  // r42 - e
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "9"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_RH(5,4,&tmp4),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "4"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_RH(5,4,&tmp4),&link);
  
// tmp5
  
  // f - r51
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "6"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HR(5,6,&tmp5),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "8"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HR(5,6,&tmp5),&link);

  // r51 - f
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "6"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_RH(6,5,&tmp5),&link);
  link = xbt_dict_get_or_null(surf_network_model->resource_set, "8"); xbt_assert0(link!=NULL,"NULL");
  xbt_dynar_push(ROUTE_HIERARCHICAL_RH(6,5,&tmp5),&link);

// loopback

  link = xbt_dict_get_or_null(surf_network_model->resource_set, "loopback"); xbt_assert0(link!=NULL,"NULL loopback");
  xbt_dynar_push(ROUTE_HIERARCHICAL_HH(0,0,&tmp3),&link);
  xbt_dynar_push(ROUTE_HIERARCHICAL_HH(1,1,&tmp3),&link);
  xbt_dynar_push(ROUTE_HIERARCHICAL_HH(2,2,&tmp4),&link);
  xbt_dynar_push(ROUTE_HIERARCHICAL_HH(3,3,&tmp4),&link);
  xbt_dynar_push(ROUTE_HIERARCHICAL_HH(4,4,&tmp4),&link);
  xbt_dynar_push(ROUTE_HIERARCHICAL_HH(5,5,&tmp5),&link);
    
//   for(i=0;i<=5;i++) {
//     for(j=0;j<=5;j++) {
//       links = routing_hierarchical_get_route(i,j);
//       printf("route from %d to %d",i,j);
//       cpt=0;
//       xbt_dynar_foreach(links, cpt, link) {
// 	s_surf_resource_t* generic_resource = link;
// 	printf(" l%s,",generic_resource->name);
//       }
//       printf("\n");
//     }
//   }
// 
//     exit(0);

}

static void routing_model_hierarchical_create(size_t size_of_link,void *loopback) {
  
  /* initialize the id counters */  
  local_host_count = 0;
  local_router_count = 0;
  
  /* initialize our structure */
  routing_hierarchical_t routing = xbt_new0(s_routing_hierarchical_t,1);
  routing->generic_routing.name = "Hierarchical";
  routing->generic_routing.host_count = 0;
  routing->generic_routing.get_route = routing_hierarchical_get_route;
  routing->generic_routing.get_onelink_routes = routing_hierarchical_get_onelink_routes;
  routing->generic_routing.is_router = routing_hierarchical_is_router;
  routing->generic_routing.finalize = routing_hierarchical_finalize;
  
  routing->size_of_link = size_of_link;
  routing->loopback = loopback;
  
  /* Set it in position */
  used_routing = (routing_t) routing;

  /* Set the dict for host ids */
  routing->generic_routing.host_id = xbt_dict_new();

  /* Set the host */
  routing->host_routetree = xbt_dynar_new(sizeof(routetree_t), NULL);

  /* Create the last route array */
  routing->last_route = xbt_dynar_new(routing->size_of_link, NULL);
  
  /* Setup the parsing callbacks we need */
  
  // DAVID - only for parse the hosts
  surfxml_add_callback(STag_surfxml_host_cb_list, &routing_full_parse_Shost);
  
  // surfxml_add_callback(STag_surfxml_router_cb_list, &routing_full_parse_Srouter);
  // surfxml_add_callback(ETag_surfxml_platform_cb_list, &routing_full_parse_end);
  // surfxml_add_callback(STag_surfxml_route_cb_list, &routing_full_parse_Sroute_set_endpoints);
  // surfxml_add_callback(ETag_surfxml_route_cb_list, &routing_full_parse_Eroute);
  // surfxml_add_callback(STag_surfxml_cluster_cb_list, &routing_full_parse_Scluster);

  // DAVID - make a fix example
  surfxml_add_callback(ETag_surfxml_platform_cb_list, &example);

}
