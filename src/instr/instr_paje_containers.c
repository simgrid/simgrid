/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/instr_private.h"
#include "xbt/lib.h"
#include "surf/surf.h"
#include "surf/surf_routing.h"

#ifdef HAVE_TRACING

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_paje_containers, instr, "Paje tracing event system (containers)");

static container_t rootContainer = NULL;    /* the root container */
static xbt_dict_t allContainers = NULL;     /* all created containers indexed by name */
xbt_dict_t trivaNodeTypes = NULL;     /* all host types defined */
xbt_dict_t trivaEdgeTypes = NULL;     /* all link types defined */

long long int instr_new_paje_id (void)
{
  static long long int type_id = 0;
  return type_id++;
}

void PJ_container_alloc (void)
{
  allContainers = xbt_dict_new_homogeneous(NULL);
  trivaNodeTypes = xbt_dict_new_homogeneous(xbt_free);
  trivaEdgeTypes = xbt_dict_new_homogeneous(xbt_free);
}

void PJ_container_release (void)
{
  xbt_dict_free (&allContainers);
  xbt_dict_free (&trivaNodeTypes);
  xbt_dict_free (&trivaEdgeTypes);
}

void PJ_container_set_root (container_t root)
{
  rootContainer = root;
}

container_t PJ_container_new (const char *name, e_container_types kind, container_t father)
{
  if (name == NULL){
    THROWF (tracing_error, 0, "can't create a container with a NULL name");
  }

  static long long int container_id = 0;
  char id_str[INSTR_DEFAULT_STR_SIZE];
  snprintf (id_str, INSTR_DEFAULT_STR_SIZE, "%lld", container_id++);

  container_t new = xbt_new0(s_container_t, 1);
  new->name = xbt_strdup (name); // name of the container
  new->id = xbt_strdup (id_str); // id (or alias) of the container
  new->father = father;

  //Search for network_element_t
  switch (kind){
    case INSTR_HOST:
      new->net_elm = xbt_lib_get_or_null(host_lib,name,ROUTING_HOST_LEVEL);
      if(!new->net_elm) xbt_die("Element '%s' not found",name);
      break;
    case INSTR_ROUTER:
      new->net_elm = xbt_lib_get_or_null(as_router_lib,name,ROUTING_ASR_LEVEL);
      if(!new->net_elm) xbt_die("Element '%s' not found",name);
      break;
    case INSTR_AS:
      new->net_elm = xbt_lib_get_or_null(as_router_lib,name,ROUTING_ASR_LEVEL);
      if(!new->net_elm) xbt_die("Element '%s' not found",name);
      break;
    default:
      new->net_elm = NULL;
      break;
  }

  // level depends on level of father
  if (new->father){
    new->level = new->father->level+1;
    XBT_DEBUG("new container %s, child of %s", name, father->name);
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
      new->type = PJ_type_get_or_null (as_typename, new->father->type);
      if (new->type == NULL){
        new->type = PJ_type_container_new (as_typename, new->father->type);
      }
    }else{
      new->type = PJ_type_container_new ("0", NULL);
    }
  }else{
    //otherwise, the name is its kind
    char typename[INSTR_DEFAULT_STR_SIZE];
    switch (new->kind){
      case INSTR_HOST:        snprintf (typename, INSTR_DEFAULT_STR_SIZE, "HOST");        break;
      case INSTR_LINK:        snprintf (typename, INSTR_DEFAULT_STR_SIZE, "LINK");        break;
      case INSTR_ROUTER:      snprintf (typename, INSTR_DEFAULT_STR_SIZE, "ROUTER");      break;
      case INSTR_SMPI:        snprintf (typename, INSTR_DEFAULT_STR_SIZE, "MPI");         break;
      case INSTR_MSG_PROCESS: snprintf (typename, INSTR_DEFAULT_STR_SIZE, "MSG_PROCESS"); break;
      case INSTR_MSG_VM: snprintf (typename, INSTR_DEFAULT_STR_SIZE, "MSG_VM"); break;
      case INSTR_MSG_TASK:    snprintf (typename, INSTR_DEFAULT_STR_SIZE, "MSG_TASK");    break;
      default: THROWF (tracing_error, 0, "new container kind is unknown."); break;
    }
    type_t type = PJ_type_get_or_null (typename, new->father->type);
    if (type == NULL){
      new->type = PJ_type_container_new (typename, new->father->type);
    }else{
      new->type = type;
    }
  }
  new->children = xbt_dict_new_homogeneous(NULL);
  if (new->father){
    xbt_dict_set(new->father->children, new->name, new, NULL);
    new_pajeCreateContainer (new);
  }

  //register all kinds by name
  if (xbt_dict_get_or_null(allContainers, new->name) != NULL){
    THROWF(tracing_error, 1, "container %s already present in allContainers data structure", new->name);
  }

  xbt_dict_set (allContainers, new->name, new, NULL);
  XBT_DEBUG("Add container name '%s'",new->name);

  //register NODE types for triva configuration
  if (new->kind == INSTR_HOST || new->kind == INSTR_LINK || new->kind == INSTR_ROUTER) {
    xbt_dict_set (trivaNodeTypes, new->type->name, xbt_strdup("1"), NULL);
  }

  return new;
}

container_t PJ_container_get (const char *name)
{
  container_t ret = PJ_container_get_or_null (name);
  if (ret == NULL){
    THROWF(tracing_error, 1, "container with name %s not found", name);
  }
  return ret;
}

container_t PJ_container_get_or_null (const char *name)
{
  return name ? xbt_dict_get_or_null(allContainers, name) : NULL;
}

container_t PJ_container_get_root ()
{
  return rootContainer;
}

void PJ_container_remove_from_parent (container_t child)
{
  if (child == NULL){
    THROWF (tracing_error, 0, "can't remove from parent with a NULL child");
  }

  container_t parent = child->father;
  if (parent){
    XBT_DEBUG("removeChildContainer (%s) FromContainer (%s) ",
        child->name,
        parent->name);
    xbt_dict_remove (parent->children, child->name);
  }
}

void PJ_container_free (container_t container)
{
  if (container == NULL){
    THROWF (tracing_error, 0, "trying to free a NULL container");
  }
  XBT_DEBUG("destroy container %s", container->name);

  //obligation to dump previous events because they might
  //reference the container that is about to be destroyed
  TRACE_last_timestamp_to_dump = surf_get_clock();
  TRACE_paje_dump_buffer(1);

  //trace my destruction
  if (!TRACE_disable_destroy() && container != PJ_container_get_root()){
    //do not trace the container destruction if user requests
    //or if the container is root
    new_pajeDestroyContainer(container);
  }

  //remove it from allContainers data structure
  xbt_dict_remove (allContainers, container->name);

  //free
  xbt_free (container->name);
  xbt_free (container->id);
  xbt_dict_free (&container->children);
  xbt_free (container);
  container = NULL;
}

static void recursiveDestroyContainer (container_t container)
{
  if (container == NULL){
    THROWF (tracing_error, 0, "trying to recursively destroy a NULL container");
  }
  XBT_DEBUG("recursiveDestroyContainer %s", container->name);
  xbt_dict_cursor_t cursor = NULL;
  container_t child;
  char *child_name;
  xbt_dict_foreach(container->children, cursor, child_name, child) {
    recursiveDestroyContainer (child);
  }
  PJ_container_free (container);
}

void PJ_container_free_all ()
{
  container_t root = PJ_container_get_root();
  if (root == NULL){
    THROWF (tracing_error, 0, "trying to free all containers, but root is NULL");
  }
  recursiveDestroyContainer (root);
  rootContainer = NULL;

  //checks
  if (!xbt_dict_is_empty(allContainers)){
    THROWF(tracing_error, 0, "some containers still present even after destroying all of them");
  }
}

#endif /* HAVE_TRACING */
