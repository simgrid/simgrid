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

static const char *instr_node_name (xbt_node_t node)
{
  void *data = xbt_graph_node_get_data(node);
  char *str = (char*)data;
  return str;
}


static container_t lowestCommonAncestor (container_t a1, container_t a2)
{
  //this is only an optimization (since most of a1 and a2 share the same parent)
  if (a1->father == a2->father) return a1->father;

  //create an array with all ancestors of a1
  xbt_dynar_t ancestors_a1 = xbt_dynar_new(sizeof(container_t), NULL);
  container_t p;
  p = a1->father;
  while (p){
    xbt_dynar_push_as (ancestors_a1, container_t, p);
    p = p->father;
  }

  //create an array with all ancestors of a2
  xbt_dynar_t ancestors_a2 = xbt_dynar_new(sizeof(container_t), NULL);
  p = a2->father;
  while (p){
    xbt_dynar_push_as (ancestors_a2, container_t, p);
    p = p->father;
  }

  //find the lowest ancestor
  p = NULL;
  int i = xbt_dynar_length (ancestors_a1) - 1;
  int j = xbt_dynar_length (ancestors_a2) - 1;
  while (i >= 0 && j >= 0){
    container_t a1p = *(container_t*)xbt_dynar_get_ptr (ancestors_a1, i);
    container_t a2p = *(container_t*)xbt_dynar_get_ptr (ancestors_a2, j);
    if (a1p == a2p){
      p = a1p;
    }else{
      break;
    }
    i--;
    j--;
  }
  xbt_dynar_free (&ancestors_a1);
  xbt_dynar_free (&ancestors_a2);
  return p;
}

static void linkContainers (container_t src, container_t dst, xbt_dict_t filter)
{
  //ignore loopback
  if (strcmp (src->name, "__loopback__") == 0 || strcmp (dst->name, "__loopback__") == 0)
    return;

  //find common father
  container_t father = lowestCommonAncestor (src, dst);
  if (!father){
    xbt_die ("common father unknown, this is a tracing problem");
  }

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
  snprintf (link_typename, INSTR_DEFAULT_STR_SIZE, "%s-%s%s-%s%s",
            father->type->name,
            src->type->name, src->type->id,
            dst->type->name, dst->type->id);
  type_t link_type = PJ_type_get_or_null (link_typename, father->type);
  if (link_type == NULL){
    link_type = PJ_type_link_new (link_typename, father->type, src->type, dst->type);
  }

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
    //if child1 is not a link, a smpi node, a msg process or a msg task
    if (child1->kind == INSTR_LINK || child1->kind == INSTR_SMPI || child1->kind == INSTR_MSG_PROCESS || child1->kind == INSTR_MSG_TASK) continue;

    xbt_dict_foreach(container->children, cursor2, child2_name, child2) {
      //if child2 is not a link, a smpi node, a msg process or a msg task
      if (child2->kind == INSTR_LINK || child2->kind == INSTR_SMPI || child2->kind == INSTR_MSG_PROCESS || child2->kind == INSTR_MSG_TASK) continue;

      //if child1 is not child2
      if (strcmp (child1_name, child2_name) == 0) continue;

      //get the route
      sg_platf_route_cbarg_t route = xbt_new0(s_sg_platf_route_cbarg_t,1);
      route->link_list = xbt_dynar_new(sizeof(sg_routing_link_t),NULL);
      rc->get_route_and_latency(rc, child1->net_elm, child2->net_elm,
                                route, NULL);

      //user might want to extract a graph using routes with only one link
      //see --cfg=tracing/onelink_only:1 or --help-tracing for details
      if (TRACE_onelink_only() && xbt_dynar_length (route->link_list) > 1){
        generic_free_route(route);
        continue;
      }

      //traverse the route connecting the containers
      unsigned int cpt;
      void *link;
      container_t current, previous;
      if (route->gw_src){
        previous = PJ_container_get(route->gw_src->name);
      }else{
        previous = child1;
      }

      xbt_dynar_foreach (route->link_list, cpt, link) {
        char *link_name = ((link_CM02_t)link)->lmm_resource.generic_resource.name;
        current = PJ_container_get(link_name);
        linkContainers(previous, current, filter);
        previous = current;
      }
      if (route->gw_dst){
        current = PJ_container_get(route->gw_dst->name);
      }else{
        current = child2;
      }
      linkContainers(previous, current, filter);
      generic_free_route(route);
    }
  }
}

/*
 * Callbacks
 */
static void instr_routing_parse_start_AS (const char*id,int routing)
{
  if (PJ_container_get_root() == NULL){
    PJ_container_alloc ();
    PJ_type_alloc();
    container_t root = PJ_container_new (id, INSTR_AS, NULL);
    PJ_container_set_root (root);

    if (TRACE_smpi_is_enabled()) {
      if (!TRACE_smpi_is_grouped()){
        type_t mpi = PJ_type_get_or_null ("MPI", root->type);
        if (mpi == NULL){
          mpi = PJ_type_container_new("MPI", root->type);
          PJ_type_state_new ("MPI_STATE", mpi);
          PJ_type_link_new ("MPI_LINK", PJ_type_get_root(), mpi, mpi);
        }
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
    container_t new = PJ_container_new (id, INSTR_AS, father);
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

    container_t new = PJ_container_new (link_name, INSTR_LINK, father);

    if (TRACE_categorized() || TRACE_uncategorized() || TRACE_platform()){
      type_t bandwidth = PJ_type_get_or_null ("bandwidth", new->type);
      if (bandwidth == NULL){
        bandwidth = PJ_type_variable_new ("bandwidth", NULL, new->type);
      }
      type_t latency = PJ_type_get_or_null ("latency", new->type);
      if (latency == NULL){
        latency = PJ_type_variable_new ("latency", NULL, new->type);
      }
      new_pajeSetVariable (0, new, bandwidth, bandwidth_value);
      new_pajeSetVariable (0, new, latency, latency_value);
    }
    if (TRACE_uncategorized()){
      type_t bandwidth_used = PJ_type_get_or_null ("bandwidth_used", new->type);
      if (bandwidth_used == NULL){
        bandwidth_used = PJ_type_variable_new ("bandwidth_used", "0.5 0.5 0.5", new->type);
      }
    }
  }

  xbt_dynar_free (&links_to_create);
}

static void instr_routing_parse_start_host (sg_platf_host_cbarg_t host)
{
  container_t father = *(container_t*)xbt_dynar_get_ptr(currentContainer, xbt_dynar_length(currentContainer)-1);
  container_t new = PJ_container_new (host->id, INSTR_HOST, father);

  if (TRACE_categorized() || TRACE_uncategorized() || TRACE_platform()) {
    type_t power = PJ_type_get_or_null ("power", new->type);
    if (power == NULL){
      power = PJ_type_variable_new ("power", NULL, new->type);
    }
    new_pajeSetVariable (0, new, power, host->power_peak);
  }
  if (TRACE_uncategorized()){
    type_t power_used = PJ_type_get_or_null ("power_used", new->type);
    if (power_used == NULL){
      power_used = PJ_type_variable_new ("power_used", "0.5 0.5 0.5", new->type);
    }
  }

  if (TRACE_smpi_is_enabled() && TRACE_smpi_is_grouped()){
    type_t mpi = PJ_type_get_or_null ("MPI", new->type);
    if (mpi == NULL){
      mpi = PJ_type_container_new("MPI", new->type);
      PJ_type_state_new ("MPI_STATE", mpi);
      PJ_type_link_new ("MPI_LINK", PJ_type_get_root(), mpi, mpi);
    }
  }

  if (TRACE_msg_process_is_enabled()) {
    type_t msg_process = PJ_type_get_or_null ("MSG_PROCESS", new->type);
    if (msg_process == NULL){
      msg_process = PJ_type_container_new("MSG_PROCESS", new->type);
      type_t state = PJ_type_state_new ("MSG_PROCESS_STATE", msg_process);
      PJ_value_new ("suspend", "1 0 1", state);
      PJ_value_new ("sleep", "1 1 0", state);
      PJ_value_new ("receive", "1 0 0", state);
      PJ_value_new ("send", "0 0 1", state);
      PJ_value_new ("task_execute", "0 1 1", state);
      PJ_type_link_new ("MSG_PROCESS_LINK", PJ_type_get_root(), msg_process, msg_process);
      PJ_type_link_new ("MSG_PROCESS_TASK_LINK", PJ_type_get_root(), msg_process, msg_process);
    }
  }
}

static void instr_routing_parse_start_router (sg_platf_router_cbarg_t router)
{
  container_t father = *(container_t*)xbt_dynar_get_ptr(currentContainer, xbt_dynar_length(currentContainer)-1);
  PJ_container_new (router->id, INSTR_ROUTER, father);
}

static void instr_routing_parse_end_platform ()
{
  xbt_dynar_free(&currentContainer);
  currentContainer = NULL;
  xbt_dict_t filter = xbt_dict_new_homogeneous(xbt_free);
  recursiveGraphExtraction (routing_platf->root, PJ_container_get_root(), filter);
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
    PJ_type_variable_new (tnstr, color, root);
  }
  if (!strcmp (root->name, "LINK")){
    char tnstr[INSTR_DEFAULT_STR_SIZE];
    snprintf (tnstr, INSTR_DEFAULT_STR_SIZE, "b%s", new_typename);
    PJ_type_variable_new (tnstr, color, root);
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
  recursiveNewVariableType (new_typename, color, PJ_type_get_root());
}

static void recursiveNewUserVariableType (const char *father_type, const char *new_typename, const char *color, type_t root)
{
  if (!strcmp (root->name, father_type)){
    PJ_type_variable_new (new_typename, color, root);
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
  recursiveNewUserVariableType (father_type, new_typename, color, PJ_type_get_root());
}

static void recursiveNewUserStateType (const char *father_type, const char *new_typename, type_t root)
{
  if (!strcmp (root->name, father_type)){
    PJ_type_state_new (new_typename, root);
  }
  xbt_dict_cursor_t cursor = NULL;
  type_t child_type;
  char *name;
  xbt_dict_foreach(root->children, cursor, name, child_type) {
    recursiveNewUserStateType (father_type, new_typename, child_type);
  }
}

void instr_new_user_state_type (const char *father_type, const char *new_typename)
{
  recursiveNewUserStateType (father_type, new_typename, PJ_type_get_root());
}

static void recursiveNewValueForUserStateType (const char *typename, const char *value, const char *color, type_t root)
{
  if (!strcmp (root->name, typename)){
    PJ_value_new (value, color, root);
  }
  xbt_dict_cursor_t cursor = NULL;
  type_t child_type;
  char *name;
  xbt_dict_foreach(root->children, cursor, name, child_type) {
    recursiveNewValueForUserStateType (typename, value, color, child_type);
  }
}

void instr_new_value_for_user_state_type (const char *typename, const char *value, const char *color)
{
  recursiveNewValueForUserStateType (typename, value, color, PJ_type_get_root());
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

  const char *sn = instr_node_name (s);
  const char *dn = instr_node_name (d);
  int len = strlen(sn)+strlen(dn)+1;
  char *name = (char*)malloc(len * sizeof(char));


  snprintf (name, len, "%s%s", sn, dn);
  ret = xbt_dict_get_or_null (edges, name);
  if (ret == NULL){
    snprintf (name, len, "%s%s", dn, sn);
    ret = xbt_dict_get_or_null (edges, name);
  }

  if (ret == NULL){
    ret = xbt_graph_new_edge(graph, s, d, NULL);
    xbt_dict_set (edges, name, ret, NULL);
  }
  free (name);
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
    //if child1 is not a link, a smpi node, a msg process or a msg task
    if (child1->kind == INSTR_LINK || child1->kind == INSTR_SMPI || child1->kind == INSTR_MSG_PROCESS || child1->kind == INSTR_MSG_TASK) continue;

    xbt_dict_foreach(container->children, cursor2, child2_name, child2) {
      //if child2 is not a link, a smpi node, a msg process or a msg task
      if (child2->kind == INSTR_LINK || child2->kind == INSTR_SMPI || child2->kind == INSTR_MSG_PROCESS || child2->kind == INSTR_MSG_TASK) continue;

      //if child1 is not child2
      if (strcmp (child1_name, child2_name) == 0) continue;

      //get the route
      sg_platf_route_cbarg_t route = xbt_new0(s_sg_platf_route_cbarg_t,1);
      route->link_list = xbt_dynar_new(sizeof(sg_routing_link_t),NULL);
      rc->get_route_and_latency(rc, child1->net_elm, child2->net_elm,
                                route, NULL);

      //user might want to extract a graph using routes with only one link
      //see --cfg=tracing/onelink_only:1 or --help-tracing for details
      if (TRACE_onelink_only() && xbt_dynar_length (route->link_list) > 1) continue;

      //traverse the route connecting the containers
      unsigned int cpt;
      void *link;
      xbt_node_t current, previous;
      if (route->gw_src){
        previous = new_xbt_graph_node(graph, route->gw_src->name, nodes);
      }else{
        previous = new_xbt_graph_node(graph, child1_name, nodes);
      }

      xbt_dynar_foreach (route->link_list, cpt, link) {
        char *link_name = ((link_CM02_t)link)->lmm_resource.generic_resource.name;
        current = new_xbt_graph_node(graph, link_name, nodes);
        new_xbt_graph_edge (graph, previous, current, edges);
        //previous -> current
        previous = current;
      }
      if (route->gw_dst){
        current = new_xbt_graph_node(graph, route->gw_dst->name, nodes);
      }else{
        current = new_xbt_graph_node(graph, child2_name, nodes);
      }
      new_xbt_graph_edge (graph, previous, current, edges);
      generic_free_route(route);
    }
  }

}

xbt_graph_t instr_routing_platform_graph (void)
{
  xbt_graph_t ret = xbt_graph_new_graph (0, NULL);
  xbt_dict_t nodes = xbt_dict_new_homogeneous(NULL);
  xbt_dict_t edges = xbt_dict_new_homogeneous(NULL);
  recursiveXBTGraphExtraction (ret, nodes, edges, routing_platf->root, PJ_container_get_root());
  xbt_dict_free (&nodes);
  xbt_dict_free (&edges);
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
    fprintf(file, "  \"%s\";\n", instr_node_name(node));
  }
  xbt_dynar_foreach(g->edges, cursor, edge) {
    const char *src_s = instr_node_name (edge->src);
    const char *dst_s = instr_node_name (edge->dst);
    if (g->directed)
      fprintf(file, "  \"%s\" -> \"%s\";\n", src_s, dst_s);
    else
      fprintf(file, "  \"%s\" -- \"%s\";\n", src_s, dst_s);
  }
  fprintf(file, "}\n");
  fclose(file);

}

#endif /* HAVE_TRACING */

