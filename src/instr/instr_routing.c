/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/instr_private.h"

#ifdef HAVE_TRACING
#include "surf/surfxml_parse_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_routing, instr, "Tracing platform hierarchy");

extern routing_global_t global_routing; /* from surf/surf_routing.c */
extern xbt_dict_t defined_types; /* from instr_interface.c */

typedef struct s_container *container_t;
typedef struct s_container {
  char *name;
  char *type;
  int level;
  struct s_container *father;
  xbt_dict_t children;
}s_container_t;

static container_t rootContainer = NULL;
static xbt_dynar_t currentContainer = NULL;
static xbt_dict_t created_links = NULL;
static xbt_dict_t created_hosts = NULL;
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
}


static container_t newContainer (const char *name, const char *type, const char *typename)
{
  container_t newContainer = xbt_new0(s_container_t, 1);
  newContainer->name = xbt_strdup (name);
  newContainer->father = xbt_dynar_get_ptr(currentContainer, xbt_dynar_length(currentContainer)-1);
  newContainer->level = newContainer->father->level+1;
  newContainer->type = xbt_strdup (type);
  newContainer->children = xbt_dict_new();
  xbt_dict_set(newContainer->father->children, newContainer->name, newContainer, NULL);

  newContainerType (newContainer->type, newContainer->father->type, typename);
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

/*
 * Callbacks
 */
static void instr_routing_parse_start_AS ()
{
  if (rootContainer == NULL){
    rootContainer = xbt_new0(s_container_t, 1);
    rootContainer->name = xbt_strdup ("0");
    rootContainer->type = xbt_strdup ("0");
    rootContainer->level = 0;
    rootContainer->father = NULL;
    rootContainer->children = xbt_dict_new();

    currentContainer = xbt_dynar_new (sizeof(s_container_t), NULL);
    xbt_dynar_push (currentContainer, rootContainer);

    created_links = xbt_dict_new ();
    created_hosts = xbt_dict_new ();
    hosts_types = xbt_dict_new ();
    links_types = xbt_dict_new ();
  }

  container_t newContainer = xbt_new0(s_container_t, 1);
  newContainer->name = xbt_strdup (A_surfxml_AS_id);
  newContainer->father = xbt_dynar_get_ptr(currentContainer, xbt_dynar_length(currentContainer)-1);
  newContainer->level = newContainer->father->level+1;
  newContainer->type = instr_AS_type (newContainer->level);
  newContainer->children = xbt_dict_new();
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
  container_t new = newContainer (A_surfxml_link_id, type, "LINK");

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
  xbt_dict_set (created_links, A_surfxml_link_id, new, NULL);

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
  container_t new = newContainer (A_surfxml_host_id, type, "HOST");

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
  xbt_dict_set (created_hosts, A_surfxml_host_id, new, NULL);

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
  newContainer (A_surfxml_router_id, type, "ROUTER");
}

static void instr_routing_parse_end_router ()
{
}

/*
 * Support functions
 */
int instr_link_is_traced (const char *name)
{
  if (xbt_dict_get_or_null(created_links, name)) {
    return 1;
  } else {
    return 0;
  }
}

char *instr_link_type (const char *name)
{
  container_t created_link = xbt_dict_get_or_null(created_links, name);
  if (created_link){
    return created_link->type;
  }else{
    return NULL;
  }
}


char *instr_host_type (const char *name)
{
  container_t created_host = xbt_dict_get_or_null(created_hosts, name);
  if (created_host){
    return created_host->type;
  }else{
    return NULL;
  }
}

void instr_destroy_platform ()
{
  if (rootContainer) recursiveDestroyContainer (rootContainer);
}

#endif /* HAVE_TRACING */

