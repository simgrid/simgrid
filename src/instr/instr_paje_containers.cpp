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

s_container::s_container (const char *name, e_container_types kind, container_t father)
{
  if (name == nullptr){
    THROWF (tracing_error, 0, "can't create a container with a nullptr name");
  }

  static long long int container_id = 0;
  char id_str[INSTR_DEFAULT_STR_SIZE];
  snprintf (id_str, INSTR_DEFAULT_STR_SIZE, "%lld", container_id);
  container_id++;

  this->name = xbt_strdup (name); // name of the container
  this->id = xbt_strdup (id_str); // id (or alias) of the container
  this->father = father;
  sg_host_t sg_host = sg_host_by_name(name);

  //Search for network_element_t
  switch (kind){
    case INSTR_HOST:
      this->netpoint = sg_host->pimpl_netpoint;
      xbt_assert(this->netpoint, "Element '%s' not found", name);
      break;
    case INSTR_ROUTER:
      this->netpoint = simgrid::s4u::Engine::getInstance()->getNetpointByNameOrNull(name);
      xbt_assert(this->netpoint, "Element '%s' not found", name);
      break;
    case INSTR_AS:
      this->netpoint = simgrid::s4u::Engine::getInstance()->getNetpointByNameOrNull(name);
      xbt_assert(this->netpoint, "Element '%s' not found", name);
      break;
    default:
      this->netpoint = nullptr;
      break;
  }

  // level depends on level of father
  if (this->father){
    this->level = this->father->level+1;
    XBT_DEBUG("new container %s, child of %s", name, father->name);
  }else{
    this->level = 0;
  }
  // type definition (method depends on kind of this new container)
  this->kind = kind;
  if (this->kind == INSTR_AS){
    //if this container is of an AS, its type name depends on its level
    char as_typename[INSTR_DEFAULT_STR_SIZE];
    snprintf (as_typename, INSTR_DEFAULT_STR_SIZE, "L%d", this->level);
    if (this->father){
      this->type = PJ_type_get_or_null (as_typename, this->father->type);
      if (this->type == nullptr){
        this->type = PJ_type_container_new (as_typename, this->father->type);
      }
    }else{
      this->type = PJ_type_container_new ("0", nullptr);
    }
  }else{
    //otherwise, the name is its kind
    char typeNameBuff[INSTR_DEFAULT_STR_SIZE];
    switch (this->kind){
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
    type_t type = PJ_type_get_or_null (typeNameBuff, this->father->type);
    if (type == nullptr){
      this->type = PJ_type_container_new (typeNameBuff, this->father->type);
    }else{
      this->type = type;
    }
  }
  this->children = xbt_dict_new_homogeneous(nullptr);
  if (this->father){
    xbt_dict_set(this->father->children, this->name, this, nullptr);
    LogContainerCreation(this);
  }

  //register all kinds by name
  if (xbt_dict_get_or_null(allContainers, this->name) != nullptr){
    THROWF(tracing_error, 1, "container %s already present in allContainers data structure", this->name);
  }

  xbt_dict_set (allContainers, this->name, this, nullptr);
  XBT_DEBUG("Add container name '%s'",this->name);

  //register NODE types for triva configuration
  if (this->kind == INSTR_HOST || this->kind == INSTR_LINK || this->kind == INSTR_ROUTER) {
    trivaNodeTypes.insert(this->type->name);
  }
}

container_t s_container::s_container_get (const char *name)
{
  container_t ret = s_container_get_or_null (name);
  if (ret == nullptr){
    THROWF(tracing_error, 1, "container with name %s not found", name);
  }
  return ret;
}

container_t s_container_get_or_null (const char *name)
{
  return static_cast<container_t>(name != nullptr ? xbt_dict_get_or_null(allContainers, name) : nullptr);
}

container_t s_container_get_root ()
{
  return rootContainer;
}

void s_container::s_container_remove_from_parent (container_t child)
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
  if (not TRACE_disable_destroy() && container != s_container_get_root()) {
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
  container_t root = s_container_get_root();
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
