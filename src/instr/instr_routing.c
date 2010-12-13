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
  TYPE_VARIABLE,
  TYPE_LINK,
  TYPE_CONTAINER,
} e_entity_types;

typedef struct s_type *type_t;
typedef struct s_type {
  char *id;
  char *name;
  e_entity_types kind;
  struct s_type *father;
  xbt_dict_t children;
}s_type_t;

typedef enum {
  INSTR_HOST,
  INSTR_LINK,
  INSTR_ROUTER,
  INSTR_AS,
} e_container_types;

typedef struct s_container *container_t;
typedef struct s_container {
  char *name;     /* Unique name of this container */
  char *id;       /* Unique id of this container */
  type_t type;    /* Type of this container */
  int level;      /* Level in the hierarchy, root level is 0 */
  e_container_types kind; /* This container is of what kind */
  struct s_container *father;
  xbt_dict_t children;
}s_container_t;

static int platform_created = 0;            /* indicate whether the platform file has been traced */
static type_t rootType = NULL;              /* the root type */
static container_t rootContainer = NULL;    /* the root container */
static xbt_dynar_t currentContainer = NULL; /* push and pop, used only in creation */
static xbt_dict_t allContainers = NULL;     /* all created containers indexed by name */
xbt_dynar_t allLinkTypes = NULL;     /* all link types defined */
xbt_dynar_t allHostTypes = NULL;     /* all host types defined */

static void instr_routing_parse_start_AS (void);
static void instr_routing_parse_end_AS (void);
static void instr_routing_parse_start_link (void);
static void instr_routing_parse_end_link (void);
static void instr_routing_parse_start_host (void);
static void instr_routing_parse_end_host (void);
static void instr_routing_parse_start_router (void);
static void instr_routing_parse_end_router (void);
static void instr_routing_parse_end_platform (void);

static long long int newTypeId ()
{
  static long long int counter = 0;
  return counter++;
}

static type_t newType (const char *typename, e_entity_types kind, type_t father)
{
  type_t ret = xbt_new0(s_type_t, 1);
  ret->name = xbt_strdup (typename);
  ret->father = father;
  ret->kind = kind;
  ret->children = xbt_dict_new ();

  long long int id = newTypeId();
  char str_id[INSTR_DEFAULT_STR_SIZE];
  snprintf (str_id, INSTR_DEFAULT_STR_SIZE, "%lld", id);
  ret->id = xbt_strdup (str_id);

  if (father != NULL){
    xbt_dict_set (father->children, typename, ret, NULL);
  }
  return ret;
}

static type_t newContainerType (const char *typename, e_entity_types kind, type_t father)
{
  type_t ret = newType (typename, kind, father);
//  if (father) INFO4("ContainerType %s(%s), child of %s(%s)", ret->name, ret->id, father->name, father->id);
  if (father) pajeDefineContainerType(ret->id, ret->father->id, ret->name);
  return ret;
}

static type_t newVariableType (const char *typename, e_entity_types kind, const char *color, type_t father)
{
  type_t ret = newType (typename, kind, father);
//  INFO4("VariableType %s(%s), child of %s(%s)", ret->name, ret->id, father->name, father->id);
  if (color){
    pajeDefineVariableTypeWithColor(ret->id, ret->father->id, ret->name, color);
  }else{
    pajeDefineVariableType(ret->id, ret->father->id, ret->name);
  }
  return ret;
}

static type_t newLinkType (const char *typename, e_entity_types kind, type_t father, type_t source, type_t dest)
{
  type_t ret = newType (typename, kind, father);
//  INFO8("LinkType %s(%s), child of %s(%s)  %s(%s)->%s(%s)", ret->name, ret->id, father->name, father->id, source->name, source->id, dest->name, dest->id);
  pajeDefineLinkType(ret->id, ret->father->id, source->id, dest->id, ret->name);
  return ret;
}

static type_t getContainerType (const char *typename, type_t father)
{
  type_t ret;
  if (father == NULL){
    ret = newContainerType (typename, TYPE_CONTAINER, father);
    rootType = ret;
  }else{
    //check if my father type already has my typename
    ret = (type_t)xbt_dict_get_or_null (father->children, typename);
    if (ret == NULL){
      ret = newContainerType (typename, TYPE_CONTAINER, father);
    }
  }
  return ret;
}

static type_t getVariableType (const char *typename, const char *color, type_t father)
{
  type_t ret = xbt_dict_get_or_null (father->children, typename);
  if (ret == NULL){
    ret = newVariableType (typename, TYPE_VARIABLE, color, father);
  }
  return ret;
}

static type_t getLinkType (const char *typename, type_t father, type_t source, type_t dest)
{
  type_t ret = xbt_dict_get_or_null (father->children, typename);
  if (ret == NULL){
    ret = newLinkType (typename, TYPE_LINK, father, source, dest);
  }
  return ret;
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

static long long int newContainedId ()
{
  static long long counter = 0;
  return counter++;
}

static container_t newContainer (const char *name, e_container_types kind, container_t father)
{
  long long int counter = newContainedId();
  char id_str[INSTR_DEFAULT_STR_SIZE];
  snprintf (id_str, INSTR_DEFAULT_STR_SIZE, "%lld", counter);

  container_t new = xbt_new0(s_container_t, 1);
  new->name = xbt_strdup (name); // name of the container
  new->id = xbt_strdup (id_str); // id (or alias) of the container
  new->father = father;
  // father of this container
  if (new->father){
    new->father = xbt_dynar_get_ptr(currentContainer, xbt_dynar_length(currentContainer)-1);
  }else{
    new->father = NULL;
  }
  // level depends on level of father
  if (new->father){
    new->level = new->father->level+1;
  }else{
    new->level = 0;
  }
  // type definition (method depends on kind of this new container)
  new->kind = kind;
  if (new->kind == INSTR_AS){
    //if this container is of an AS, its type name depends on its level
    char as_typename[INSTR_DEFAULT_STR_SIZE];
    snprintf (as_typename, INSTR_DEFAULT_STR_SIZE, "L%d", new->level);
    if (new->father){
      new->type = getContainerType (as_typename, new->father->type);
    }else{
      new->type = getContainerType ("0", NULL);
    }
  }else{
    //otherwise, the name is its kind
    switch (new->kind){
      case INSTR_HOST: new->type = getContainerType ("HOST", new->father->type); break;
      case INSTR_LINK: new->type = getContainerType ("LINK", new->father->type); break;
      case INSTR_ROUTER: new->type = getContainerType ("ROUTER", new->father->type); break;
      default: xbt_die ("Congratulations, you have found a bug on newContainer function of instr_routing.c"); break;
    }
  }
  new->children = xbt_dict_new();
  if (new->father){
    xbt_dict_set(new->father->children, new->name, new, NULL);
    pajeCreateContainer (0, new->id, new->type->id, new->father->id, new->name);
  }

  //register hosts, routers, links containers
  if (new->kind == INSTR_HOST || new->kind == INSTR_LINK || new->kind == INSTR_ROUTER) {
    xbt_dict_set (allContainers, new->name, new, NULL);
  }

  //register the host container types
  if (new->kind == INSTR_HOST){
    xbt_dynar_push_as (allHostTypes, type_t, new->type);
  }

  //register the link container types
  if (new->kind == INSTR_LINK){
    xbt_dynar_push_as(allLinkTypes, type_t, new->type);
  }
  return new;
}

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

  container_t a1_container = ((container_t)xbt_dict_get (allContainers, a1));
  type_t a1_type = a1_container->type;

  container_t a2_container = ((container_t)xbt_dict_get (allContainers, a2));
  type_t a2_type = a2_container->type;

  container_t container = findCommonFather (rootContainer, a1_container, a2_container);
  xbt_assert0 (container != NULL, "common father not found");

  //declare type
  char link_typename[INSTR_DEFAULT_STR_SIZE];
  snprintf (link_typename, INSTR_DEFAULT_STR_SIZE, "%s-%s", a1_type->name, a2_type->name);
  type_t link_type = getLinkType (link_typename, container->type, a1_type, a2_type);

  //create the link
  static long long counter = 0;
  char key[INSTR_DEFAULT_STR_SIZE];
  snprintf (key, INSTR_DEFAULT_STR_SIZE, "%lld", counter++);
  pajeStartLink(SIMIX_get_clock(), link_type->id, container->id, "G", a1_container->id, key);
  pajeEndLink(SIMIX_get_clock(), link_type->id, container->id, "G", a2_container->id, key);
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
            linkContainers (previous_entity_name, link_name);
            previous_entity_name = link_name;
          }
          linkContainers (previous_entity_name, child_name2);
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
  if (rootContainer == NULL){
    currentContainer = xbt_dynar_new (sizeof(s_container_t), NULL);
    allContainers = xbt_dict_new ();
    allLinkTypes = xbt_dynar_new (sizeof(s_type_t), NULL);
    allHostTypes = xbt_dynar_new (sizeof(s_type_t), NULL);

    rootContainer = newContainer ("0", INSTR_AS, NULL);
    xbt_dynar_push (currentContainer, rootContainer);

  }
  container_t father = xbt_dynar_get_ptr(currentContainer, xbt_dynar_length(currentContainer)-1);
  container_t new = newContainer (A_surfxml_AS_id, INSTR_AS, father);

  //push
  xbt_dynar_push (currentContainer, new);
}

static void instr_routing_parse_end_AS ()
{
  xbt_dynar_pop_ptr (currentContainer);
}

static void instr_routing_parse_start_link ()
{
  container_t father = xbt_dynar_get_ptr(currentContainer, xbt_dynar_length(currentContainer)-1);
  container_t new = newContainer (A_surfxml_link_id, INSTR_LINK, father);

  type_t bandwidth = getVariableType ("bandwidth", NULL, new->type);
  type_t latency = getVariableType ("latency", NULL, new->type);
  pajeSetVariable (0, bandwidth->id, new->id, A_surfxml_link_bandwidth);
  pajeSetVariable (0, latency->id, new->id, A_surfxml_link_latency);
  if (TRACE_uncategorized()){
    getVariableType ("bandwidth_used", "0.5 0.5 0.5", new->type);
  }
}

static void instr_routing_parse_end_link ()
{
}

static void instr_routing_parse_start_host ()
{
  container_t father = xbt_dynar_get_ptr(currentContainer, xbt_dynar_length(currentContainer)-1);
  container_t new = newContainer (A_surfxml_host_id, INSTR_HOST, father);

  type_t power = getVariableType ("power", NULL, new->type);
  pajeSetVariable (0, power->id, new->id, A_surfxml_host_power);
  if (TRACE_uncategorized()){
    getVariableType ("power_used", "0.5 0.5 0.5", new->type);
  }
}

static void instr_routing_parse_end_host ()
{
}

static void instr_routing_parse_start_router ()
{
  container_t father = xbt_dynar_get_ptr(currentContainer, xbt_dynar_length(currentContainer)-1);
  newContainer (A_surfxml_router_id, INSTR_ROUTER, father);
}

static void instr_routing_parse_end_router ()
{
}

static void instr_routing_parse_end_platform ()
{
  currentContainer = NULL;
  recursiveGraphExtraction (rootContainer);
  platform_created = 1;
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

char *instr_variable_type (const char *name, const char *resource)
{
  container_t container = (container_t)xbt_dict_get (allContainers, resource);
  xbt_dict_cursor_t cursor = NULL;
  type_t type;
  char *type_name;
  xbt_dict_foreach(container->type->children, cursor, type_name, type) {
    if (strcmp (name, type->name) == 0) return type->id;
  }
  return NULL;
}

char *instr_resource_type (const char *resource_name)
{
  return ((container_t)xbt_dict_get_or_null (allContainers, resource_name))->id;
}

static void recursiveDestroyContainer (container_t container)
{
  xbt_dict_cursor_t cursor = NULL;
  container_t child;
  char *child_name;
  xbt_dict_foreach(container->children, cursor, child_name, child) {
    recursiveDestroyContainer (child);
  }

  pajeDestroyContainer(SIMIX_get_clock(), container->type->id, container->id);

  xbt_free (container->name);
  xbt_free (container->id);
  xbt_free (container->children);
  xbt_free (container);
  container = NULL;
}

static void recursiveDestroyType (type_t type)
{
  xbt_dict_cursor_t cursor = NULL;
  type_t child;
  char *child_name;
  xbt_dict_foreach(type->children, cursor, child_name, child) {
    recursiveDestroyType (child);
  }
  xbt_free (type->name);
  xbt_free (type->id);
  xbt_free (type->children);
  xbt_free (type);
  type = NULL;
}

void instr_destroy_platform ()
{
  if (rootContainer) recursiveDestroyContainer (rootContainer);
  if (rootType) recursiveDestroyType (rootType);
}

/*
 * user categories support
 */
static void recursiveNewUserVariableType (const char *new_typename, const char *color, type_t root)
{
  if (!strcmp (root->name, "HOST") || !strcmp (root->name, "LINK")){
    newVariableType(new_typename, TYPE_VARIABLE, color, root);
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
  recursiveNewUserVariableType (new_typename, color, rootType);
}

static void recursiveNewUserLinkVariableType (const char *new_typename, const char *color, type_t root)
{
  if (!strcmp (root->name, "LINK")){
    newVariableType(new_typename, TYPE_VARIABLE, color, root);
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
  recursiveNewUserLinkVariableType (new_typename, color, rootType);
}


static void recursiveNewUserHostVariableType (const char *new_typename, const char *color, type_t root)
{
  if (!strcmp (root->name, "HOST")){
    newVariableType(new_typename, TYPE_VARIABLE, color, root);
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
  recursiveNewUserHostVariableType (new_typename, color, rootType);
}

int instr_platform_traced ()
{
  return platform_created;
}

#endif /* HAVE_TRACING */

