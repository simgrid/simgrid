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

static void linkContainers (const char *a1, const char *a2)
{
  //ignore loopback
  if (strcmp (a1, "__loopback__") == 0 || strcmp (a2, "__loopback__") == 0)
    return;

  container_t a1_container = getContainerByName (a1);
  type_t a1_type = a1_container->type;

  container_t a2_container = getContainerByName (a2);
  type_t a2_type = a2_container->type;

  container_t container = findCommonFather (getRootContainer(), a1_container, a2_container);
  xbt_assert0 (container != NULL, "common father not found");

  //declare type
  char link_typename[INSTR_DEFAULT_STR_SIZE];
  snprintf (link_typename, INSTR_DEFAULT_STR_SIZE, "%s-%s", a1_type->name, a2_type->name);
  type_t link_type = getLinkType (link_typename, container->type, a1_type, a2_type);

  //register EDGE types for triva configuration
  xbt_dict_set (trivaEdgeTypes, link_type->name, xbt_strdup("1"), xbt_free);

  //create the link
  static long long counter = 0;
  char key[INSTR_DEFAULT_STR_SIZE];
  snprintf (key, INSTR_DEFAULT_STR_SIZE, "%lld", counter++);
  new_pajeStartLink(SIMIX_get_clock(), container, link_type, a1_container, "G", key);
  new_pajeEndLink(SIMIX_get_clock(), container, link_type, a2_container, "G", key);
}

static void recursiveGraphExtraction (container_t container)
{
  if (xbt_dict_length(container->children)){
    xbt_dict_cursor_t cursor = NULL;
    container_t child;
    char *child_name;
    //bottom-up recursion
    xbt_dict_foreach(container->children, cursor, child_name, child) {
      recursiveGraphExtraction (child);
    }

    //let's get routes
    xbt_dict_cursor_t cursor1 = NULL, cursor2 = NULL;
    container_t child1, child2;
    const char *child_name1, *child_name2;

    xbt_dict_t filter = xbt_dict_new ();

    xbt_dict_foreach(container->children, cursor1, child_name1, child1) {
      xbt_dict_foreach(container->children, cursor2, child_name2, child2) {
        //check if we already register this pair (we only need one direction)
        char aux1[INSTR_DEFAULT_STR_SIZE], aux2[INSTR_DEFAULT_STR_SIZE];
        snprintf (aux1, INSTR_DEFAULT_STR_SIZE, "%s%s", child_name1, child_name2);
        snprintf (aux2, INSTR_DEFAULT_STR_SIZE, "%s%s", child_name2, child_name1);
        if (xbt_dict_get_or_null (filter, aux1)) continue;
        if (xbt_dict_get_or_null (filter, aux2)) continue;

        //ok, not found, register it
        xbt_dict_set (filter, aux1, xbt_strdup ("1"), xbt_free);
        xbt_dict_set (filter, aux2, xbt_strdup ("1"), xbt_free);

        if ((child1->kind == INSTR_HOST || child1->kind == INSTR_ROUTER) &&
            (child2->kind == INSTR_HOST  || child2->kind == INSTR_ROUTER)){

          //getting route
          xbt_dynar_t route = NULL;
          xbt_ex_t exception;
          TRY {
            route = global_routing->get_route (child_name1, child_name2);
          }CATCH(exception) {
            //no route between them, that's possible
            continue;
          }

          //link the route members
          unsigned int cpt;
          void *link;
          char *previous_entity_name = (char*)child_name1;
          xbt_dynar_foreach (route, cpt, link) {
            char *link_name = ((link_CM02_t)link)->lmm_resource.generic_resource.name;
            linkContainers (previous_entity_name, link_name);
            previous_entity_name = link_name;
          }
          linkContainers (previous_entity_name, child_name2);
        }else if (child1->kind == INSTR_AS &&
                  child2->kind == INSTR_AS &&
                  strcmp(child_name1, child_name2) != 0){

          //getting route
          routing_component_t root = global_routing->root;
          route_extended_t route = NULL;
          xbt_ex_t exception;
          TRY {
            route = root->get_route (root, child_name1, child_name2);
          }CATCH(exception) {
            //no route between them, that's possible
            continue;
          }
          xbt_assert2(route!=NULL,
              "there is no ASroute between %s and %s", child_name1, child_name2);
          unsigned int cpt;
          void *link;
          char *previous_entity_name = route->src_gateway;
          xbt_dynar_foreach (route->generic_route.link_list, cpt, link) {
            char *link_name = ((link_CM02_t)link)->lmm_resource.generic_resource.name;
            linkContainers (previous_entity_name, link_name);
            previous_entity_name = link_name;
          }
          linkContainers (previous_entity_name, route->dst_gateway);
        }
      }
    }
    xbt_dict_free(&filter);
  }
}

/*
 * Callbacks
 */
static void instr_routing_parse_start_AS ()
{
  if (getRootContainer() == NULL){
    container_t root = newContainer ("0", INSTR_AS, NULL);
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
  container_t new = newContainer (A_surfxml_link_id, INSTR_LINK, father);

  type_t bandwidth = getVariableType ("bandwidth", NULL, new->type);
  type_t latency = getVariableType ("latency", NULL, new->type);
  new_pajeSetVariable (0, new, bandwidth, atof(A_surfxml_link_bandwidth));
  new_pajeSetVariable (0, new, latency, atof(A_surfxml_link_latency));
  if (TRACE_uncategorized()){
    getVariableType ("bandwidth_used", "0.5 0.5 0.5", new->type);
  }
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
    getStateType ("MSG_PROCESS_STATE", msg_process);
    getLinkType ("MSG_PROCESS_LINK", getRootType(), msg_process, msg_process);
  }

  if (TRACE_msg_task_is_enabled()) {
    type_t msg_task = getContainerType ("MSG_TASK", new->type);
    getStateType ("MSG_TASK_STATE", msg_task);
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
  recursiveGraphExtraction (getRootContainer());
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
  if (!strcmp (root->name, "HOST") || !strcmp (root->name, "LINK")){
    getVariableType(new_typename, color, root);
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

