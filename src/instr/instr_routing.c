/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/instr_private.h"

#ifdef HAVE_TRACING
#include "surf/surf_private.h"
#include "surf/network_private.h"
#include "xbt/graph.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_routing, instr, "Tracing platform hierarchy");

extern xbt_dict_t defined_types; /* from instr_interface.c */

static int platform_created = 0;            /* indicate whether the platform file has been traced */
static xbt_dynar_t currentContainer = NULL; /* push and pop, used only in creation */

static container_t findChild (container_t root, container_t a1)
{
  if (root == a1) return root;

  xbt_dict_cursor_t cursor = NULL;
  container_t child;
  char *child_name;
  xbt_dict_foreach(root->children, cursor, child_name, child) {
    if (findChild (child, a1)) return child;
  }
  return NULL;
}

static container_t findCommonFather (container_t root, container_t a1, container_t a2)
{
  if (a1->father == a2->father) return a1->father;

  xbt_dict_cursor_t cursor = NULL;
  container_t child;
  char *child_name;
  container_t a1_try = NULL;
  container_t a2_try = NULL;
  xbt_dict_foreach(root->children, cursor, child_name, child) {
    a1_try = findChild (child, a1);
    a2_try = findChild (child, a2);
    if (a1_try && a2_try) return child;
  }
  return NULL;
}

static void linkContainers (container_t father, container_t src, container_t dst, xbt_dict_t filter)
{
  //ignore loopback
  if (strcmp (src->name, "__loopback__") == 0 || strcmp (dst->name, "__loopback__") == 0)
    return;

  if (filter != NULL){
    //check if we already register this pair (we only need one direction)
    char aux1[INSTR_DEFAULT_STR_SIZE], aux2[INSTR_DEFAULT_STR_SIZE];
    snprintf (aux1, INSTR_DEFAULT_STR_SIZE, "%s%s", src->name, dst->name);
    snprintf (aux2, INSTR_DEFAULT_STR_SIZE, "%s%s", dst->name, src->name);
    if (xbt_dict_get_or_null (filter, aux1)) return;
    if (xbt_dict_get_or_null (filter, aux2)) return;

    //ok, not found, register it
    xbt_dict_set (filter, aux1, xbt_strdup ("1"), NULL);
    xbt_dict_set (filter, aux2, xbt_strdup ("1"), NULL);
  }

  //declare type
  char link_typename[INSTR_DEFAULT_STR_SIZE];
  snprintf (link_typename, INSTR_DEFAULT_STR_SIZE, "%s-%s", src->type->name, dst->type->name);
  type_t link_type = getLinkType (link_typename, father->type, src->type, dst->type);

  //register EDGE types for triva configuration
  xbt_dict_set (trivaEdgeTypes, link_type->name, xbt_strdup("1"), NULL);

  //create the link
  static long long counter = 0;
  char key[INSTR_DEFAULT_STR_SIZE];
  snprintf (key, INSTR_DEFAULT_STR_SIZE, "%lld", counter++);
  new_pajeStartLink(SIMIX_get_clock(), father, link_type, src, "G", key);
  new_pajeEndLink(SIMIX_get_clock(), father, link_type, dst, "G", key);
}

static void recursiveGraphExtraction (AS_t rc, container_t container, xbt_dict_t filter)
{
  if (!xbt_dict_is_empty(rc->routing_sons)){
    xbt_dict_cursor_t cursor = NULL;
    AS_t rc_son;
    char *child_name;
    //bottom-up recursion
    xbt_dict_foreach(rc->routing_sons, cursor, child_name, rc_son) {
      container_t child_container = xbt_dict_get (container->children, rc_son->name);
      recursiveGraphExtraction (rc_son, child_container, filter);
    }
  }

  //let's get routes
  xbt_dict_cursor_t cursor1 = NULL, cursor2 = NULL;
  container_t child1, child2;
  const char *child1_name, *child2_name;
  xbt_dict_foreach(container->children, cursor1, child1_name, child1) {
    if (child1->kind == INSTR_LINK) continue;
    xbt_dict_foreach(container->children, cursor2, child2_name, child2) {
      if (child2->kind == INSTR_LINK) continue;

      if ((child1->kind == INSTR_HOST || child1->kind == INSTR_ROUTER) &&
          (child2->kind == INSTR_HOST  || child2->kind == INSTR_ROUTER) &&
          strcmp (child1_name, child2_name) != 0){

        xbt_dynar_t route = NULL;
        xbt_ex_t e;

        TRY {
          routing_get_route_and_latency(child1_name, child2_name, &route, NULL);
        } CATCH(e) {
          xbt_ex_free(e);
        }
        if (route == NULL) continue;

        if (TRACE_onelink_only()){
          if (xbt_dynar_length (route) > 1) continue;
        }
        container_t previous = child1;
        int i;
        for (i = 0; i < xbt_dynar_length(route); i++){
          link_CM02_t *link = ((link_CM02_t*)xbt_dynar_get_ptr (route, i));
          char *link_name = (*link)->lmm_resource.generic_resource.name;
          container_t current = getContainerByName(link_name);
          linkContainers(container, previous, current, filter);
          previous = current;
        }
        linkContainers(container, previous, child2, filter);

      }else if (child1->kind == INSTR_AS &&
                child2->kind == INSTR_AS &&
                strcmp(child1_name, child2_name) != 0){

        route_t route = xbt_new0(s_route_t,1);
        route->link_list = xbt_dynar_new(global_routing->size_of_link,NULL);
        rc->get_route_and_latency (rc, child1_name, child2_name, route,NULL);
        unsigned int cpt;
        void *link;
        container_t previous = getContainerByName(route->src_gateway);
        xbt_dynar_foreach (route->link_list, cpt, link) {
          char *link_name = ((link_CM02_t)link)->lmm_resource.generic_resource.name;
          container_t current = getContainerByName(link_name);
          linkContainers (container, previous, current, filter);
          previous = current;
        }
        container_t last = getContainerByName(route->dst_gateway);
        linkContainers (container, previous, last, filter);
        generic_free_route(route);
      }
    }
  }
}

/*
 * Callbacks
 */
static void instr_routing_parse_start_AS (const char*id,const char*routing)
{
  if (getRootContainer() == NULL){
    instr_paje_init ();
    container_t root = newContainer (id, INSTR_AS, NULL);
    instr_paje_set_root (root);


    if (TRACE_smpi_is_enabled()) {
      if (!TRACE_smpi_is_grouped()){
        type_t mpi = getContainerType("MPI", root->type);
        getStateType ("MPI_STATE", mpi);
        getLinkType ("MPI_LINK", getRootType(), mpi, mpi);
      }
    }

    if (TRACE_needs_platform()){
      currentContainer = xbt_dynar_new (sizeof(container_t), NULL);
      xbt_dynar_push (currentContainer, &root);
    }
    return;
  }

  if (TRACE_needs_platform()){
    container_t father = *(container_t*)xbt_dynar_get_ptr(currentContainer, xbt_dynar_length(currentContainer)-1);
    container_t new = newContainer (id, INSTR_AS, father);
    xbt_dynar_push (currentContainer, &new);
  }
}

static void instr_routing_parse_end_AS ()
{
  if (TRACE_needs_platform()){
    xbt_dynar_pop_ptr (currentContainer);
  }
}

static void instr_routing_parse_start_link (sg_platf_link_cbarg_t link)
{
  container_t father = *(container_t*)xbt_dynar_get_ptr(currentContainer, xbt_dynar_length(currentContainer)-1);

  double bandwidth_value = link->bandwidth;
  double latency_value = link->latency;
  xbt_dynar_t links_to_create = xbt_dynar_new (sizeof(char*), &xbt_free_ref);

  if (link->policy == SURF_LINK_FULLDUPLEX){
    char *up = bprintf("%s_UP", link->id);
    char *down = bprintf("%s_DOWN", link->id);
    xbt_dynar_push_as (links_to_create, char*, xbt_strdup(up));
    xbt_dynar_push_as (links_to_create, char*, xbt_strdup(down));
    free (up);
    free (down);
  }else{
    xbt_dynar_push_as (links_to_create, char*, strdup(link->id));
  }

  char *link_name = NULL;
  unsigned int i;
  xbt_dynar_foreach (links_to_create, i, link_name){

    container_t new = newContainer (link_name, INSTR_LINK, father);

    if (TRACE_categorized() || TRACE_uncategorized()){
      type_t bandwidth = getVariableType ("bandwidth", NULL, new->type);
      type_t latency = getVariableType ("latency", NULL, new->type);
      new_pajeSetVariable (0, new, bandwidth, bandwidth_value);
      new_pajeSetVariable (0, new, latency, latency_value);
    }
    if (TRACE_uncategorized()){
      getVariableType ("bandwidth_used", "0.5 0.5 0.5", new->type);
    }
  }

  xbt_dynar_free (&links_to_create);
}

static void instr_routing_parse_start_host (sg_platf_host_cbarg_t host)
{
  container_t father = *(container_t*)xbt_dynar_get_ptr(currentContainer, xbt_dynar_length(currentContainer)-1);
  container_t new = newContainer (host->id, INSTR_HOST, father);

  if (TRACE_categorized() || TRACE_uncategorized()) {
    type_t power = getVariableType ("power", NULL, new->type);
    new_pajeSetVariable (0, new, power, host->power_peak);
  }
  if (TRACE_uncategorized()){
    getVariableType ("power_used", "0.5 0.5 0.5", new->type);
  }

  if (TRACE_smpi_is_enabled() && TRACE_smpi_is_grouped()){
    type_t mpi = getContainerType("MPI", new->type);
    getStateType ("MPI_STATE", mpi);
    getLinkType ("MPI_LINK", getRootType(), mpi, mpi);
  }

  if (TRACE_msg_process_is_enabled()) {
    type_t msg_process = getContainerType("MSG_PROCESS", new->type);
    type_t state = getStateType ("MSG_PROCESS_STATE", msg_process);
    getValue ("executing", "0 1 0", state);
    getValue ("suspend", "1 0 1", state);
    getValue ("sleep", "1 1 0", state);
    getValue ("receive", "1 0 0", state);
    getValue ("send", "0 0 1", state);
    getValue ("task_execute", "0 1 1", state);
    getLinkType ("MSG_PROCESS_LINK", getRootType(), msg_process, msg_process);
    getLinkType ("MSG_PROCESS_TASK_LINK", getRootType(), msg_process, msg_process);
  }
}

static void instr_routing_parse_start_router (sg_platf_router_cbarg_t router)
{
  container_t father = *(container_t*)xbt_dynar_get_ptr(currentContainer, xbt_dynar_length(currentContainer)-1);
  newContainer (router->id, INSTR_ROUTER, father);
}

static void instr_routing_parse_end_platform ()
{
  xbt_dynar_free(&currentContainer);
  currentContainer = NULL;
  xbt_dict_t filter = xbt_dict_new_homogeneous(xbt_free);
  recursiveGraphExtraction (global_routing->root, getRootContainer(), filter);
  xbt_dict_free(&filter);
  platform_created = 1;
  TRACE_paje_dump_buffer(1);
}

void instr_routing_define_callbacks ()
{
  if (!TRACE_is_enabled()) return;
  //always need the call backs to ASes (we need only the root AS),
  //to create the rootContainer and the rootType properly
  sg_platf_AS_begin_add_cb(instr_routing_parse_start_AS);
  sg_platf_AS_end_add_cb(instr_routing_parse_end_AS);
  if (!TRACE_needs_platform()) return;
  sg_platf_link_add_cb(instr_routing_parse_start_link);
  sg_platf_host_add_cb(instr_routing_parse_start_host);
  sg_platf_router_add_cb(instr_routing_parse_start_router);

  sg_platf_postparse_add_cb(instr_routing_parse_end_platform);
}

/*
 * user categories support
 */
static void recursiveNewVariableType (const char *new_typename, const char *color, type_t root)
{
  if (!strcmp (root->name, "HOST")){
    char tnstr[INSTR_DEFAULT_STR_SIZE];
    snprintf (tnstr, INSTR_DEFAULT_STR_SIZE, "p%s", new_typename);
    getVariableType(tnstr, color, root);
  }
  if (!strcmp (root->name, "LINK")){
    char tnstr[INSTR_DEFAULT_STR_SIZE];
    snprintf (tnstr, INSTR_DEFAULT_STR_SIZE, "b%s", new_typename);
    getVariableType(tnstr, color, root);
  }
  xbt_dict_cursor_t cursor = NULL;
  type_t child_type;
  char *name;
  xbt_dict_foreach(root->children, cursor, name, child_type) {
    recursiveNewVariableType (new_typename, color, child_type);
  }
}

void instr_new_variable_type (const char *new_typename, const char *color)
{
  recursiveNewVariableType (new_typename, color, getRootType());
}

static void recursiveNewUserVariableType (const char *father_type, const char *new_typename, const char *color, type_t root)
{
  if (!strcmp (root->name, father_type)){
    getVariableType(new_typename, color, root);
  }
  xbt_dict_cursor_t cursor = NULL;
  type_t child_type;
  char *name;
  xbt_dict_foreach(root->children, cursor, name, child_type) {
    recursiveNewUserVariableType (father_type, new_typename, color, child_type);
  }
}

void instr_new_user_variable_type  (const char *father_type, const char *new_typename, const char *color)
{
  recursiveNewUserVariableType (father_type, new_typename, color, getRootType());
}



int instr_platform_traced ()
{
  return platform_created;
}

#define GRAPHICATOR_SUPPORT_FUNCTIONS


static xbt_node_t new_xbt_graph_node (xbt_graph_t graph, const char *name, xbt_dict_t nodes)
{
  xbt_node_t ret = xbt_dict_get_or_null (nodes, name);
  if (ret) return ret;

  ret = xbt_graph_new_node (graph, xbt_strdup(name));
  xbt_dict_set (nodes, name, ret, NULL);
  return ret;
}

static xbt_edge_t new_xbt_graph_edge (xbt_graph_t graph, xbt_node_t s, xbt_node_t d, xbt_dict_t edges)
{
  xbt_edge_t ret;
  char *name;

  const char *sn = TRACE_node_name (s);
  const char *dn = TRACE_node_name (d);

  name = bprintf ("%s%s", sn, dn);
  ret = xbt_dict_get_or_null (edges, name);
  if (ret) return ret;
  free (name);
  name = bprintf ("%s%s", dn, sn);
  ret = xbt_dict_get_or_null (edges, name);
  if (ret) return ret;

  ret = xbt_graph_new_edge(graph, s, d, NULL);
  xbt_dict_set (edges, name, ret, NULL);
  return ret;
}

static void recursiveXBTGraphExtraction (xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges,
    AS_t rc, container_t container)
{
  if (!xbt_dict_is_empty(rc->routing_sons)){
    xbt_dict_cursor_t cursor = NULL;
    AS_t rc_son;
    char *child_name;
    //bottom-up recursion
    xbt_dict_foreach(rc->routing_sons, cursor, child_name, rc_son) {
      container_t child_container = xbt_dict_get (container->children, rc_son->name);
      recursiveXBTGraphExtraction (graph, nodes, edges, rc_son, child_container);
    }
  }

  //let's get routes
  xbt_dict_cursor_t cursor1 = NULL, cursor2 = NULL;
  container_t child1, child2;
  const char *child1_name, *child2_name;
  xbt_dict_foreach(container->children, cursor1, child1_name, child1) {
    if (child1->kind == INSTR_LINK) continue;
    xbt_dict_foreach(container->children, cursor2, child2_name, child2) {
      if (child2->kind == INSTR_LINK) continue;

      if ((child1->kind == INSTR_HOST || child1->kind == INSTR_ROUTER) &&
          (child2->kind == INSTR_HOST  || child2->kind == INSTR_ROUTER) &&
          strcmp (child1_name, child2_name) != 0){

        // FIXME factorize route creation with else branch below (once possible)
        xbt_dynar_t route=NULL;
        routing_get_route_and_latency (child1_name, child2_name,&route,NULL);
        if (TRACE_onelink_only()){
          if (xbt_dynar_length (route) > 1) continue;
        }
        unsigned int cpt;
        void *link;
        xbt_node_t current, previous = new_xbt_graph_node(graph, child1_name, nodes);
        xbt_dynar_foreach (route, cpt, link) {
          char *link_name = ((link_CM02_t)link)->lmm_resource.generic_resource.name;
          current = new_xbt_graph_node(graph, link_name, nodes);
          new_xbt_graph_edge (graph, previous, current, edges);
          //previous -> current
          previous = current;
        }
        current = new_xbt_graph_node(graph, child2_name, nodes);
        new_xbt_graph_edge (graph, previous, current, edges);

      }else if (child1->kind == INSTR_AS &&
                child2->kind == INSTR_AS &&
                strcmp(child1_name, child2_name) != 0){

        route_t route = xbt_new0(s_route_t,1);
        route->link_list = xbt_dynar_new(global_routing->size_of_link,NULL);
        rc->get_route_and_latency (rc, child1_name, child2_name,route, NULL);
        unsigned int cpt;
        void *link;
        xbt_node_t current, previous = new_xbt_graph_node(graph, route->src_gateway, nodes);
        xbt_dynar_foreach (route->link_list, cpt, link) {
          char *link_name = ((link_CM02_t)link)->lmm_resource.generic_resource.name;
          current = new_xbt_graph_node(graph, link_name, nodes);
          //previous -> current
          previous = current;
        }
        current = new_xbt_graph_node(graph, route->dst_gateway, nodes);
        new_xbt_graph_edge (graph, previous, current, edges);
        generic_free_route(route);
      }
    }
  }

}

xbt_graph_t instr_routing_platform_graph (void)
{
  xbt_graph_t ret = xbt_graph_new_graph (0, NULL);
  xbt_dict_t nodes = xbt_dict_new_homogeneous(NULL);
  xbt_dict_t edges = xbt_dict_new_homogeneous(NULL);
  recursiveXBTGraphExtraction (ret, nodes, edges, global_routing->root, getRootContainer());
  return ret;
}

void instr_routing_platform_graph_export_graphviz (xbt_graph_t g, const char *filename)
{
  unsigned int cursor = 0;
  xbt_node_t node = NULL;
  xbt_edge_t edge = NULL;
  FILE *file = NULL;

  file = fopen(filename, "w");
  xbt_assert(file, "Failed to open %s \n", filename);

  if (g->directed)
    fprintf(file, "digraph test {\n");
  else
    fprintf(file, "graph test {\n");

  fprintf(file, "  graph [overlap=scale]\n");

  fprintf(file, "  node [shape=box, style=filled]\n");
  fprintf(file,
          "  node [width=.3, height=.3, style=filled, color=skyblue]\n\n");

  xbt_dynar_foreach(g->nodes, cursor, node) {
    fprintf(file, "  \"%s\";\n", TRACE_node_name(node));
  }
  xbt_dynar_foreach(g->edges, cursor, edge) {
    const char *src_s = TRACE_node_name (edge->src);
    const char *dst_s = TRACE_node_name (edge->dst);
    if (g->directed)
      fprintf(file, "  \"%s\" -> \"%s\";\n", src_s, dst_s);
    else
      fprintf(file, "  \"%s\" -- \"%s\";\n", src_s, dst_s);
  }
  fprintf(file, "}\n");
  fclose(file);

}

#endif /* HAVE_TRACING */

