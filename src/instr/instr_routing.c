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

typedef enum {
  INSTR_HOST,
  INSTR_LINK,
  INSTR_ROUTER,
  INSTR_AS,
} e_container_types;

typedef struct s_container *container_t;
typedef struct s_container {
  char *name;     /* Unique id of this container */
  char *type;     /* Type of this container */
  char *typename; /* Type name of this container */
  int level;      /* Level in the hierarchy, root level is 0 */
  e_container_types kind; /* This container is of what kind */
  struct s_container *father;
  xbt_dict_t children;
}s_container_t;

static container_t rootContainer = NULL;    /* the root container */
static xbt_dynar_t currentContainer = NULL; /* push and pop, used only in creation */
static xbt_dict_t allContainers = NULL;     /* all created containers indexed by name */
xbt_dict_t hosts_types = NULL;
xbt_dict_t links_types = NULL;

static void instr_routing_parse_start_AS (void);
static void instr_routing_parse_end_AS (void);
static void instr_routing_parse_start_link (void);
static void instr_routing_parse_end_link (void);
static void instr_routing_parse_start_host (void);
static void instr_routing_parse_end_host (void);
static void instr_routing_parse_start_router (void);
static void instr_routing_parse_end_router (void);
static void instr_routing_parse_end_platform (void);
static char *instr_AS_type (int level);

static char *instr_AS_type (int level)
{
  char *ret = xbt_new (char, INSTR_DEFAULT_STR_SIZE);
  if (level == 0){
    snprintf (ret, INSTR_DEFAULT_STR_SIZE, "0");
  }else{
    snprintf (ret, INSTR_DEFAULT_STR_SIZE, "L%d", level);
  }
  return ret;
}

static void newContainerType (const char *type, const char *parentType, const char *name)
{
  char *defined = xbt_dict_get_or_null (defined_types, type);
  if (!defined){
    pajeDefineContainerType(type, parentType, name);
    xbt_dict_set(defined_types, type, xbt_strdup("1"), xbt_free);
  }
}


static void newVariableType (const char *type, const char *parentType, const char *name, const char *color)
{
  char *defined = xbt_dict_get_or_null (defined_types, type);
  if (!defined){
    if (color){
      pajeDefineVariableTypeWithColor(type, parentType, name, color);
    }else{
      pajeDefineVariableType(type, parentType, name);
    }
    xbt_dict_set(defined_types, type, xbt_strdup("1"), xbt_free);
  }
}

static void newLinkType (const char *type, const char *parentType, const char *sourceType, const char *destType, const char *name)
{
  char *defined = xbt_dict_get_or_null (defined_types, type);
  if (!defined){
    pajeDefineLinkType(type, parentType, sourceType, destType, name);
    xbt_dict_set(defined_types, type, xbt_strdup("1"), xbt_free);
  }
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


static container_t newContainer (const char *name, const char *type, e_container_types kind)
{
  container_t newContainer = xbt_new0(s_container_t, 1);
  newContainer->name = xbt_strdup (name);
  newContainer->father = xbt_dynar_get_ptr(currentContainer, xbt_dynar_length(currentContainer)-1);
  newContainer->level = newContainer->father->level+1;
  newContainer->type = xbt_strdup (type);
  switch (kind){
    case INSTR_HOST: newContainer->typename = xbt_strdup ("HOST"); break;
    case INSTR_LINK: newContainer->typename = xbt_strdup ("LINK"); break;
    case INSTR_ROUTER: newContainer->typename = xbt_strdup ("ROUTER"); break;
    case INSTR_AS: newContainer->typename = xbt_strdup ("AS"); break;
    default: xbt_die ("Congratulations, you have found a bug on newContainer function of instr_routing.c"); break;
  }
  newContainer->kind = kind;
  newContainer->children = xbt_dict_new();
  xbt_dict_set(newContainer->father->children, newContainer->name, newContainer, NULL);

  newContainerType (newContainer->type, newContainer->father->type, newContainer->typename);
  pajeCreateContainer (0, newContainer->name, newContainer->type, newContainer->father->name, newContainer->name);

  return newContainer;
}

static void recursiveDestroyContainer (container_t container)
{
  xbt_dict_cursor_t cursor = NULL;
  container_t child;
  char *child_name;
  xbt_dict_foreach(container->children, cursor, child_name, child) {
    recursiveDestroyContainer (child);
  }

  pajeDestroyContainer(SIMIX_get_clock(), container->type, container->name);

  xbt_free (container->name);
  xbt_free (container->type);
  xbt_free (container->children);
  xbt_free (container);
  container = NULL;
}

static void linkContainers (container_t container, const char *a1, const char *a2)
{
  //ignore loopback
  if (strcmp (a1, "__loopback__") == 0 || strcmp (a2, "__loopback__") == 0)
    return;

  char *a1_type = ((container_t)xbt_dict_get (allContainers, a1))->type;
  char *a1_typename = ((container_t)xbt_dict_get (allContainers, a1))->typename;

  char *a2_type = ((container_t)xbt_dict_get (allContainers, a2))->type;
  char *a2_typename = ((container_t)xbt_dict_get (allContainers, a2))->typename;

  //declare type
  char new_link_type[INSTR_DEFAULT_STR_SIZE], new_link_typename[INSTR_DEFAULT_STR_SIZE];
  snprintf (new_link_type, INSTR_DEFAULT_STR_SIZE, "%s-%s", a1_type, a2_type);
  snprintf (new_link_typename, INSTR_DEFAULT_STR_SIZE, "%s-%s", a1_typename, a2_typename);
  newLinkType (new_link_type, container->type, a1_type, a2_type, new_link_typename);

  //create the link
  static long long counter = 0;
  char key[INSTR_DEFAULT_STR_SIZE];
  snprintf (key, INSTR_DEFAULT_STR_SIZE, "G%lld", counter++);
  pajeStartLink(SIMIX_get_clock(), new_link_type, container->name, "graph", a1, key);
  pajeEndLink(SIMIX_get_clock(), new_link_type, container->name, "graph", a2, key);
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

    xbt_dict_foreach(container->children, cursor1, child_name1, child1) {
      xbt_dict_foreach(container->children, cursor2, child_name2, child2) {
        if ((child1->kind == INSTR_HOST || child1->kind == INSTR_ROUTER) &&
            (child2->kind == INSTR_HOST  || child2->kind == INSTR_ROUTER)){

          //getting route
          xbt_dynar_t route;
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
            linkContainers (container, previous_entity_name, link_name);
            previous_entity_name = link_name;
          }
          linkContainers (container, previous_entity_name, child_name2);
        }else if (child1->kind == INSTR_AS &&
                  child2->kind == INSTR_AS &&
                  strcmp(child_name1, child_name2) != 0){

          //getting route
          routing_component_t root = global_routing->root;
          route_extended_t route;
          xbt_ex_t exception;
          TRY {
            route = root->get_route (root, child_name1, child_name2);
          }CATCH(exception) {
            //no route between them, that's possible
            continue;
          }
          unsigned int cpt;
          void *link;
          char *previous_entity_name = route->src_gateway;
          xbt_dynar_foreach (route->generic_route.link_list, cpt, link) {
            char *link_name = ((link_CM02_t)link)->lmm_resource.generic_resource.name;
            linkContainers (container, previous_entity_name, link_name);
            previous_entity_name = link_name;
          }
          linkContainers (container, previous_entity_name, route->dst_gateway);
        }
      }
    }
  }
}

/*
 * Callbacks
 */
static void instr_routing_parse_start_AS ()
{
  if (rootContainer == NULL){
    rootContainer = xbt_new0(s_container_t, 1);
    rootContainer->name = xbt_strdup ("0");
    rootContainer->type = xbt_strdup ("0");
    rootContainer->typename = xbt_strdup ("0");
    rootContainer->level = 0;
    rootContainer->father = NULL;
    rootContainer->children = xbt_dict_new();
    rootContainer->kind = INSTR_AS;

    currentContainer = xbt_dynar_new (sizeof(s_container_t), NULL);
    xbt_dynar_push (currentContainer, rootContainer);

    allContainers = xbt_dict_new ();
    hosts_types = xbt_dict_new ();
    links_types = xbt_dict_new ();
  }

  container_t newContainer = xbt_new0(s_container_t, 1);
  newContainer->name = xbt_strdup (A_surfxml_AS_id);
  newContainer->father = xbt_dynar_get_ptr(currentContainer, xbt_dynar_length(currentContainer)-1);
  newContainer->level = newContainer->father->level+1;
  newContainer->type = instr_AS_type (newContainer->level);
  newContainer->typename = instr_AS_type (newContainer->level);
  newContainer->children = xbt_dict_new();
  newContainer->kind = INSTR_AS;
  xbt_dict_set(newContainer->father->children, newContainer->name, newContainer, NULL);

  //trace
  newContainerType (newContainer->type, newContainer->father->type, newContainer->type);
  pajeCreateContainer (0, newContainer->name, newContainer->type, newContainer->father->name, newContainer->name);

  //push
  xbt_dynar_push (currentContainer, newContainer);
}

static void instr_routing_parse_end_AS ()
{
  xbt_dynar_pop_ptr (currentContainer);
}

static void instr_routing_parse_start_link ()
{
  container_t father = xbt_dynar_get_ptr(currentContainer, xbt_dynar_length(currentContainer)-1);
  char type[INSTR_DEFAULT_STR_SIZE];
  snprintf (type, INSTR_DEFAULT_STR_SIZE, "LINK-%s", father->type);
  container_t new = newContainer (A_surfxml_link_id, type, INSTR_LINK);

  //bandwidth and latency
  char bandwidth_type[INSTR_DEFAULT_STR_SIZE], latency_type[INSTR_DEFAULT_STR_SIZE];
  snprintf (bandwidth_type, INSTR_DEFAULT_STR_SIZE, "bandwidth-%s", type);
  snprintf (latency_type, INSTR_DEFAULT_STR_SIZE, "latency-%s", type);
  newVariableType (bandwidth_type, type, "bandwidth", NULL);
  newVariableType (latency_type, type, "latency", NULL);
  pajeSetVariable(0, bandwidth_type, new->name, A_surfxml_link_bandwidth);
  pajeSetVariable(0, latency_type, new->name, A_surfxml_link_latency);

  if (TRACE_uncategorized()){
    //bandwidth_used
    char bandwidth_used_type[INSTR_DEFAULT_STR_SIZE];
    snprintf (bandwidth_used_type, INSTR_DEFAULT_STR_SIZE, "bandwidth_used-%s", type);
    newVariableType (bandwidth_used_type, type, "bandwidth_used", "0.5 0.5 0.5");
  }

  //register created link on the dictionary
  xbt_dict_set (allContainers, A_surfxml_link_id, new, NULL);

  //register this link type
  xbt_dict_set (links_types, type, xbt_strdup("1"), xbt_free);
}

static void instr_routing_parse_end_link ()
{
}

static void instr_routing_parse_start_host ()
{
  container_t father = xbt_dynar_get_ptr(currentContainer, xbt_dynar_length(currentContainer)-1);
  char type[INSTR_DEFAULT_STR_SIZE];
  snprintf (type, INSTR_DEFAULT_STR_SIZE, "HOST-%s", father->type);
  container_t new = newContainer (A_surfxml_host_id, type, INSTR_HOST);

  //power
  char power_type[INSTR_DEFAULT_STR_SIZE];
  snprintf (power_type, INSTR_DEFAULT_STR_SIZE, "power-%s", type);
  newVariableType (power_type, type, "power", NULL);
  pajeSetVariable(0, power_type, new->name, A_surfxml_host_power);

  if (TRACE_uncategorized()){
    //power_used
    char power_used_type[INSTR_DEFAULT_STR_SIZE];
    snprintf (power_used_type, INSTR_DEFAULT_STR_SIZE, "power_used-%s", type);
    newVariableType (power_used_type, type, "power_used", "0.5 0.5 0.5");
  }

  //register created host on the dictionary
  xbt_dict_set (allContainers, A_surfxml_host_id, new, NULL);

  //register this link type
  xbt_dict_set (hosts_types, type, xbt_strdup("1"), xbt_free);
}

static void instr_routing_parse_end_host ()
{
}

static void instr_routing_parse_start_router ()
{
  container_t father = xbt_dynar_get_ptr(currentContainer, xbt_dynar_length(currentContainer)-1);
  char type[INSTR_DEFAULT_STR_SIZE];
  snprintf (type, INSTR_DEFAULT_STR_SIZE, "ROUTER-%s", father->type);
  container_t new = newContainer (A_surfxml_router_id, type, INSTR_ROUTER);

  //register created host on the dictionary
  xbt_dict_set (allContainers, A_surfxml_router_id, new, NULL);
}

static void instr_routing_parse_end_router ()
{
}

static void instr_routing_parse_end_platform ()
{
  currentContainer = NULL;
  recursiveGraphExtraction (rootContainer);
}

/*
 * Support functions
 */
int instr_link_is_traced (const char *name)
{
  if (((container_t)xbt_dict_get_or_null (allContainers, name))){
    return 1;
  } else {
    return 0;
  }
}

char *instr_link_type (const char *name)
{
  return ((container_t)xbt_dict_get (allContainers, name))->type;
}

char *instr_host_type (const char *name)
{
  return ((container_t)xbt_dict_get (allContainers, name))->type;
}

void instr_destroy_platform ()
{
  if (rootContainer) recursiveDestroyContainer (rootContainer);
}

#endif /* HAVE_TRACING */

