/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"

#include "surf/surf.h"

#include "src/instr/instr_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_paje_containers, instr, "Paje tracing event system (containers)");

static container_t rootContainer = nullptr;    /* the root container */
static xbt_dict_t allContainers = nullptr;     /* all created containers indexed by name */
std::set<std::string> trivaNodeTypes;           /* all host types defined */
std::set<std::string> trivaEdgeTypes;           /* all link types defined */

long long int instr_new_paje_id ()
{
  static long long int type_id = 0;
  return type_id++;
}

void PJ_container_alloc ()
{
  allContainers = xbt_dict_new_homogeneous(nullptr);
}

void PJ_container_release ()
{
  xbt_dict_free (&allContainers);
}

void PJ_container_set_root (container_t root)
{
  rootContainer = root;
}

container_t PJ_container_new (const char *name, e_container_types kind, container_t father)
{
  if (name == nullptr){
    THROWF (tracing_error, 0, "can't create a container with a nullptr name");
  }

  static long long int container_id = 0;
  char id_str[INSTR_DEFAULT_STR_SIZE];
  snprintf (id_str, INSTR_DEFAULT_STR_SIZE, "%lld", container_id);
  container_id++;

  container_t newContainer = xbt_new0(s_container, 1);
  newContainer->name = xbt_strdup (name); // name of the container
  newContainer->id = xbt_strdup (id_str); // id (or alias) of the container
  newContainer->father = father;
  sg_host_t sg_host = sg_host_by_name(name);

  //Search for network_element_t
  switch (kind){
    case INSTR_HOST:
      newContainer->netpoint = sg_host->pimpl_netpoint;
      xbt_assert(newContainer->netpoint, "Element '%s' not found", name);
      break;
    case INSTR_ROUTER:
      newContainer->netpoint = simgrid::s4u::Engine::getInstance()->getNetpointByNameOrNull(name);
      xbt_assert(newContainer->netpoint, "Element '%s' not found", name);
      break;
    case INSTR_AS:
      newContainer->netpoint = simgrid::s4u::Engine::getInstance()->getNetpointByNameOrNull(name);
      xbt_assert(newContainer->netpoint, "Element '%s' not found", name);
      break;
    default:
      newContainer->netpoint = nullptr;
      break;
  }

  // level depends on level of father
  if (newContainer->father){
    newContainer->level = newContainer->father->level+1;
    XBT_DEBUG("new container %s, child of %s", name, father->name);
  }else{
    newContainer->level = 0;
  }
  // type definition (method depends on kind of this new container)
  newContainer->kind = kind;
  if (newContainer->kind == INSTR_AS){
    //if this container is of an AS, its type name depends on its level
    char as_typename[INSTR_DEFAULT_STR_SIZE];
    snprintf (as_typename, INSTR_DEFAULT_STR_SIZE, "L%d", newContainer->level);
    if (newContainer->father){
      newContainer->type = s_type::PJ_type_get_or_null (as_typename, newContainer->father->type);
      if (newContainer->type == nullptr){
        newContainer->type = PJ_type_container_new (as_typename, newContainer->father->type);
      }
    }else{
      newContainer->type = PJ_type_container_new ("0", nullptr);
    }
  }else{
    //otherwise, the name is its kind
    char typeNameBuff[INSTR_DEFAULT_STR_SIZE];
    switch (newContainer->kind){
      case INSTR_HOST:
        snprintf (typeNameBuff, INSTR_DEFAULT_STR_SIZE, "HOST");
        break;
      case INSTR_LINK:
        snprintf (typeNameBuff, INSTR_DEFAULT_STR_SIZE, "LINK");
        break;
      case INSTR_ROUTER:
        snprintf (typeNameBuff, INSTR_DEFAULT_STR_SIZE, "ROUTER");
        break;
      case INSTR_SMPI:
        snprintf (typeNameBuff, INSTR_DEFAULT_STR_SIZE, "MPI");
        break;
      case INSTR_MSG_PROCESS:
        snprintf (typeNameBuff, INSTR_DEFAULT_STR_SIZE, "MSG_PROCESS");
        break;
      case INSTR_MSG_VM:
        snprintf (typeNameBuff, INSTR_DEFAULT_STR_SIZE, "MSG_VM");
        break;
      case INSTR_MSG_TASK:
        snprintf (typeNameBuff, INSTR_DEFAULT_STR_SIZE, "MSG_TASK");
        break;
      default:
        THROWF (tracing_error, 0, "new container kind is unknown.");
        break;
    }
    type_t type = s_type::PJ_type_get_or_null (typeNameBuff, newContainer->father->type);
    if (type == nullptr){
      newContainer->type = PJ_type_container_new (typeNameBuff, newContainer->father->type);
    }else{
      newContainer->type = type;
    }
  }
  newContainer->children = xbt_dict_new_homogeneous(nullptr);
  if (newContainer->father){
    xbt_dict_set(newContainer->father->children, newContainer->name, newContainer, nullptr);
    LogContainerCreation(newContainer);
  }

  //register all kinds by name
  if (xbt_dict_get_or_null(allContainers, newContainer->name) != nullptr){
    THROWF(tracing_error, 1, "container %s already present in allContainers data structure", newContainer->name);
  }

  xbt_dict_set (allContainers, newContainer->name, newContainer, nullptr);
  XBT_DEBUG("Add container name '%s'",newContainer->name);

  //register NODE types for triva configuration
  if (newContainer->kind == INSTR_HOST || newContainer->kind == INSTR_LINK || newContainer->kind == INSTR_ROUTER) {
    trivaNodeTypes.insert(newContainer->type->name);
  }
  return newContainer;
}

container_t PJ_container_get (const char *name)
{
  container_t ret = PJ_container_get_or_null (name);
  if (ret == nullptr){
    THROWF(tracing_error, 1, "container with name %s not found", name);
  }
  return ret;
}

container_t PJ_container_get_or_null (const char *name)
{
  return static_cast<container_t>(name != nullptr ? xbt_dict_get_or_null(allContainers, name) : nullptr);
}

container_t PJ_container_get_root ()
{
  return rootContainer;
}

void PJ_container_remove_from_parent (container_t child)
{
  if (child == nullptr){
    THROWF (tracing_error, 0, "can't remove from parent with a nullptr child");
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
  if (container == nullptr){
    THROWF (tracing_error, 0, "trying to free a nullptr container");
  }
  XBT_DEBUG("destroy container %s", container->name);

  //obligation to dump previous events because they might
  //reference the container that is about to be destroyed
  TRACE_last_timestamp_to_dump = surf_get_clock();
  TRACE_paje_dump_buffer(1);

  //trace my destruction
  if (not TRACE_disable_destroy() && container != PJ_container_get_root()) {
    //do not trace the container destruction if user requests
    //or if the container is root
    LogContainerDestruction(container);
  }

  //remove it from allContainers data structure
  xbt_dict_remove (allContainers, container->name);

  //free
  xbt_free (container->name);
  xbt_free (container->id);
  xbt_dict_free (&container->children);
  xbt_free (container);
  container = nullptr;
}

static void recursiveDestroyContainer (container_t container)
{
  if (container == nullptr){
    THROWF (tracing_error, 0, "trying to recursively destroy a nullptr container");
  }
  XBT_DEBUG("recursiveDestroyContainer %s", container->name);
  xbt_dict_cursor_t cursor = nullptr;
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
  if (root == nullptr){
    THROWF (tracing_error, 0, "trying to free all containers, but root is nullptr");
  }
  recursiveDestroyContainer (root);
  rootContainer = nullptr;

  //checks
  if (not xbt_dict_is_empty(allContainers)) {
    THROWF(tracing_error, 0, "some containers still present even after destroying all of them");
  }
}
