/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/instr_private.h"

#ifdef HAVE_TRACING
#include "surf/surf_private.h"
#include "surf/network_private.h"

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
    xbt_dict_set (filter, aux1, xbt_strdup ("1"), xbt_free);
    xbt_dict_set (filter, aux2, xbt_strdup ("1"), xbt_free);
  }

  //declare type
  char link_typename[INSTR_DEFAULT_STR_SIZE];
  snprintf (link_typename, INSTR_DEFAULT_STR_SIZE, "%s-%s", src->type->name, dst->type->name);
  type_t link_type = getLinkType (link_typename, father->type, src->type, dst->type);

  //register EDGE types for triva configuration
  xbt_dict_set (trivaEdgeTypes, link_type->name, xbt_strdup("1"), xbt_free);

  //create the link
  static long long counter = 0;
  char key[INSTR_DEFAULT_STR_SIZE];
  snprintf (key, INSTR_DEFAULT_STR_SIZE, "%lld", counter++);
  new_pajeStartLink(SIMIX_get_clock(), father, link_type, src, "G", key);
  new_pajeEndLink(SIMIX_get_clock(), father, link_type, dst, "G", key);
}

static void recursiveGraphExtraction (routing_component_t rc, container_t container, xbt_dict_t filter)
{
  if (xbt_dict_length (rc->routing_sons)){
    xbt_dict_cursor_t cursor = NULL;
    routing_component_t rc_son;
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

        xbt_dynar_t route = global_routing->get_route (child1_name, child2_name);
        unsigned int cpt;
        void *link;
        container_t previous = child1;
        xbt_dynar_foreach (route, cpt, link) {
          char *link_name = ((link_CM02_t)link)->lmm_resource.generic_resource.name;
          container_t current = getContainerByName(link_name);
          linkContainers(container, previous, current, filter);
          previous = current;
        }
        linkContainers(container, previous, child2, filter);

      }else if (child1->kind == INSTR_AS &&
                child2->kind == INSTR_AS &&
                strcmp(child1_name, child2_name) != 0){

        route_extended_t route = rc->get_route (rc, child1_name, child2_name);
        unsigned int cpt;
        void *link;
        container_t previous = getContainerByName(route->src_gateway);
        xbt_dynar_foreach (route->generic_route.link_list, cpt, link) {
          char *link_name = ((link_CM02_t)link)->lmm_resource.generic_resource.name;
          container_t current = getContainerByName(link_name);
          linkContainers (container, previous, current, filter);
          previous = current;
        }
        container_t last = getContainerByName(route->dst_gateway);
        linkContainers (container, previous, last, filter);
      }
    }
  }
}

/*
 * Callbacks
 */
static void instr_routing_parse_start_AS ()
{
  if (getRootContainer() == NULL){
    container_t root = newContainer (A_surfxml_AS_id, INSTR_AS, NULL);
    instr_paje_init (root);

    currentContainer = xbt_dynar_new (sizeof(container_t), NULL);
    xbt_dynar_push (currentContainer, &root);

    if (TRACE_smpi_is_enabled()) {
      if (!TRACE_smpi_is_grouped()){
        container_t father = *(container_t*)xbt_dynar_get_ptr(currentContainer, xbt_dynar_length(currentContainer)-1);
        type_t mpi = getContainerType("MPI", father->type);
        getStateType ("MPI_STATE", mpi);
        getLinkType ("MPI_LINK", getRootType(), mpi, mpi);
      }
    }

    return;
  }
  container_t father = *(container_t*)xbt_dynar_get_ptr(currentContainer, xbt_dynar_length(currentContainer)-1);
  container_t new = newContainer (A_surfxml_AS_id, INSTR_AS, father);

  //push
  xbt_dynar_push (currentContainer, &new);
}

static void instr_routing_parse_end_AS ()
{
  xbt_dynar_pop_ptr (currentContainer);
}

static void instr_routing_parse_start_link ()
{
  container_t father = *(container_t*)xbt_dynar_get_ptr(currentContainer, xbt_dynar_length(currentContainer)-1);
  const char *link_id = A_surfxml_link_id;

  double bandwidth_value = atof(A_surfxml_link_bandwidth);
  double latency_value = atof(A_surfxml_link_latency);
  xbt_dynar_t links_to_create = xbt_dynar_new (sizeof(char*), &xbt_free_ref);

  if (A_surfxml_link_sharing_policy == A_surfxml_link_sharing_policy_FULLDUPLEX){
    char *up = bprintf("%s_UP", link_id);
    char *down = bprintf("%s_DOWN", link_id);
    xbt_dynar_push_as (links_to_create, char*, xbt_strdup(up));
    xbt_dynar_push_as (links_to_create, char*, xbt_strdup(down));
    free (up);
    free (down);
  }else{
    xbt_dynar_push_as (links_to_create, char*, strdup(link_id));
  }

  char *link_name = NULL;
  unsigned int i;
  xbt_dynar_foreach (links_to_create, i, link_name){

    container_t new = newContainer (link_name, INSTR_LINK, father);

    type_t bandwidth = getVariableType ("bandwidth", NULL, new->type);
    type_t latency = getVariableType ("latency", NULL, new->type);
    new_pajeSetVariable (0, new, bandwidth, bandwidth_value);
    new_pajeSetVariable (0, new, latency, latency_value);
    if (TRACE_uncategorized()){
      getVariableType ("bandwidth_used", "0.5 0.5 0.5", new->type);
    }
  }

  xbt_dynar_free (&links_to_create);
}

static void instr_routing_parse_end_link ()
{
}

static void instr_routing_parse_start_host ()
{
  container_t father = *(container_t*)xbt_dynar_get_ptr(currentContainer, xbt_dynar_length(currentContainer)-1);
  container_t new = newContainer (A_surfxml_host_id, INSTR_HOST, father);

  type_t power = getVariableType ("power", NULL, new->type);
  new_pajeSetVariable (0, new, power, atof(A_surfxml_host_power));
  if (TRACE_uncategorized()){
    getVariableType ("power_used", "0.5 0.5 0.5", new->type);
  }

  if (TRACE_smpi_is_enabled()) {
    if (TRACE_smpi_is_grouped()){
      type_t mpi = getContainerType("MPI", new->type);
      getStateType ("MPI_STATE", mpi);
      getLinkType ("MPI_LINK", getRootType(), mpi, mpi);
    }
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

  if (TRACE_msg_task_is_enabled()) {
    type_t msg_task = getContainerType ("MSG_TASK", new->type);
    type_t state = getStateType ("MSG_TASK_STATE", msg_task);
    getValue ("MSG_task_execute", "0 1 0", state);
    getValue ("created", "1 1 0", state);
    getLinkType ("MSG_TASK_LINK", getRootType(), msg_task, msg_task);
  }
}

static void instr_routing_parse_end_host ()
{
}

static void instr_routing_parse_start_router ()
{
  container_t father = *(container_t*)xbt_dynar_get_ptr(currentContainer, xbt_dynar_length(currentContainer)-1);
  newContainer (A_surfxml_router_id, INSTR_ROUTER, father);
}

static void instr_routing_parse_end_router ()
{
}

static void instr_routing_parse_end_platform ()
{
  xbt_dynar_free(&currentContainer);
  currentContainer = NULL;
  xbt_dict_t filter = xbt_dict_new ();
  recursiveGraphExtraction (global_routing->root, getRootContainer(), filter);
  xbt_dict_free(&filter);
  platform_created = 1;
  TRACE_paje_dump_buffer(1);
}

void instr_routing_define_callbacks ()
{
  if (!TRACE_is_active())
    return;
  surfxml_add_callback(STag_surfxml_AS_cb_list, &instr_routing_parse_start_AS);
  surfxml_add_callback(ETag_surfxml_AS_cb_list, &instr_routing_parse_end_AS);
  surfxml_add_callback(STag_surfxml_link_cb_list, &instr_routing_parse_start_link);
  surfxml_add_callback(ETag_surfxml_link_cb_list, &instr_routing_parse_end_link);
  surfxml_add_callback(STag_surfxml_host_cb_list, &instr_routing_parse_start_host);
  surfxml_add_callback(ETag_surfxml_host_cb_list, &instr_routing_parse_end_host);
  surfxml_add_callback(STag_surfxml_router_cb_list, &instr_routing_parse_start_router);
  surfxml_add_callback(ETag_surfxml_router_cb_list, &instr_routing_parse_end_router);
  surfxml_add_callback(ETag_surfxml_platform_cb_list, &instr_routing_parse_end_platform);
}

/*
 * user categories support
 */
static void recursiveNewUserVariableType (const char *new_typename, const char *color, type_t root)
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
    recursiveNewUserVariableType (new_typename, color, child_type);
  }
}

void instr_new_user_variable_type (const char *new_typename, const char *color)
{
  recursiveNewUserVariableType (new_typename, color, getRootType());
}

static void recursiveNewUserLinkVariableType (const char *new_typename, const char *color, type_t root)
{
  if (!strcmp (root->name, "LINK")){
    getVariableType(new_typename, color, root);
  }
  xbt_dict_cursor_t cursor = NULL;
  type_t child_type;
  char *name;
  xbt_dict_foreach(root->children, cursor, name, child_type) {
    recursiveNewUserLinkVariableType (new_typename, color, child_type);
  }
}

void instr_new_user_link_variable_type  (const char *new_typename, const char *color)
{
  recursiveNewUserLinkVariableType (new_typename, color, getRootType());
}


static void recursiveNewUserHostVariableType (const char *new_typename, const char *color, type_t root)
{
  if (!strcmp (root->name, "HOST")){
    getVariableType(new_typename, color, root);
  }
  xbt_dict_cursor_t cursor = NULL;
  type_t child_type;
  char *name;
  xbt_dict_foreach(root->children, cursor, name, child_type) {
    recursiveNewUserHostVariableType (new_typename, color, child_type);
  }
}

void instr_new_user_host_variable_type  (const char *new_typename, const char *color)
{
  recursiveNewUserHostVariableType (new_typename, color, getRootType());
}

int instr_platform_traced ()
{
  return platform_created;
}

#endif /* HAVE_TRACING */

