/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/instr_private.h"

#ifdef HAVE_TRACING

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_paje, instr, "Paje tracing event system (data structures)");

static type_t rootType = NULL;              /* the root type */
static container_t rootContainer = NULL;    /* the root container */
static xbt_dict_t allContainers = NULL;     /* all created containers indexed by name */
xbt_dynar_t allLinkTypes = NULL;     /* all link types defined */
xbt_dynar_t allHostTypes = NULL;     /* all host types defined */

void instr_paje_init (container_t root)
{
  allContainers = xbt_dict_new ();
  allLinkTypes = xbt_dynar_new (sizeof(s_type_t), NULL);
  allHostTypes = xbt_dynar_new (sizeof(s_type_t), NULL);
  rootContainer = root;
}

static long long int newTypeId ()
{
  static long long int counter = 0;
  return counter++;
}

static type_t newType (const char *typename, const char *key, e_entity_types kind, type_t father)
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
    xbt_dict_set (father->children, key, ret, NULL);
  }
  return ret;
}

type_t getRootType ()
{
  return rootType;
}

type_t getContainerType (const char *typename, type_t father)
{
  type_t ret;
  if (father == NULL){
    ret = newType (typename, typename, TYPE_CONTAINER, father);
    if (father) pajeDefineContainerType(ret->id, ret->father->id, ret->name);
    rootType = ret;
  }else{
    //check if my father type already has my typename
    ret = (type_t)xbt_dict_get_or_null (father->children, typename);
    if (ret == NULL){
      ret = newType (typename, typename, TYPE_CONTAINER, father);
      pajeDefineContainerType(ret->id, ret->father->id, ret->name);
    }
  }
  return ret;
}

type_t getEventType (const char *typename, const char *color, type_t father)
{
  type_t ret = xbt_dict_get_or_null (father->children, typename);
  if (ret == NULL){
    ret = newType (typename, typename, TYPE_EVENT, father);
    //INFO4("EventType %s(%s), child of %s(%s)", ret->name, ret->id, father->name, father->id);
    if (color){
      pajeDefineEventTypeWithColor (ret->id, ret->father->id, ret->name, color);
    }else{
      pajeDefineEventType(ret->id, ret->father->id, ret->name);
    }
  }
  return ret;
}

type_t getVariableType (const char *typename, const char *color, type_t father)
{
  type_t ret = xbt_dict_get_or_null (father->children, typename);
  if (ret == NULL){
    ret = newType (typename, typename, TYPE_VARIABLE, father);
    //INFO4("VariableType %s(%s), child of %s(%s)", ret->name, ret->id, father->name, father->id);
    if (color){
      pajeDefineVariableTypeWithColor(ret->id, ret->father->id, ret->name, color);
    }else{
      pajeDefineVariableType(ret->id, ret->father->id, ret->name);
    }
  }
  return ret;
}

type_t getLinkType (const char *typename, type_t father, type_t source, type_t dest)
{
  //FIXME should check using source and dest here and not by the typename (g5k example)
  char key[INSTR_DEFAULT_STR_SIZE];
  snprintf (key, INSTR_DEFAULT_STR_SIZE, "%s-%s-%s", typename, source->id, dest->id);
  type_t ret = xbt_dict_get_or_null (father->children, key);
  if (ret == NULL){
    ret = newType (typename, key, TYPE_LINK, father);
    //INFO8("LinkType %s(%s), child of %s(%s)  %s(%s)->%s(%s)", ret->name, ret->id, father->name, father->id, source->name, source->id, dest->name, dest->id);
    pajeDefineLinkType(ret->id, ret->father->id, source->id, dest->id, ret->name);
  }
  return ret;
}

type_t getStateType (const char *typename, type_t father)
{
  type_t ret = xbt_dict_get_or_null (father->children, typename);
  if (ret == NULL){
    ret = newType (typename, typename, TYPE_STATE, father);
    //INFO4("StateType %s(%s), child of %s(%s)", ret->name, ret->id, father->name, father->id);
    pajeDefineStateType(ret->id, ret->father->id, ret->name);
  }
  return ret;
}


static long long int newContainedId ()
{
  static long long counter = 0;
  return counter++;
}

container_t newContainer (const char *name, e_container_types kind, container_t father)
{
  long long int counter = newContainedId();
  char id_str[INSTR_DEFAULT_STR_SIZE];
  snprintf (id_str, INSTR_DEFAULT_STR_SIZE, "%lld", counter);

  container_t new = xbt_new0(s_container_t, 1);
  new->name = xbt_strdup (name); // name of the container
  new->id = xbt_strdup (id_str); // id (or alias) of the container
  new->father = father;
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
      case INSTR_SMPI: new->type = getContainerType ("MPI", new->father->type); break;
      case INSTR_MSG_PROCESS: new->type = getContainerType ("MSG_PROCESS", new->father->type); break;
      case INSTR_MSG_TASK: new->type = getContainerType ("MSG_TASK", new->father->type); break;
      default: xbt_die ("Congratulations, you have found a bug on newContainer function of instr_routing.c"); break;
    }
  }
  new->children = xbt_dict_new();
  if (new->father){
    xbt_dict_set(new->father->children, new->name, new, NULL);
    pajeCreateContainer (SIMIX_get_clock(), new->id, new->type->id, new->father->id, new->name);
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

static container_t recursiveGetContainer (const char *name, container_t root)
{
  if (strcmp (root->name, name) == 0) return root;

  xbt_dict_cursor_t cursor = NULL;
  container_t child;
  char *child_name;
  xbt_dict_foreach(root->children, cursor, child_name, child) {
    container_t ret = recursiveGetContainer(name, child);
    if (ret) return ret;
  }
  return NULL;
}

container_t getContainer (const char *name)
{
  return recursiveGetContainer(name, rootContainer);
}

container_t getContainerByName (const char *name)
{
  return (container_t)xbt_dict_get (allContainers, name);
}

container_t getRootContainer ()
{
  return rootContainer;
}

static type_t recursiveGetType (const char *name, type_t root)
{
  if (strcmp (root->name, name) == 0) return root;

  xbt_dict_cursor_t cursor = NULL;
  type_t child;
  char *child_name;
  xbt_dict_foreach(root->children, cursor, child_name, child) {
    type_t ret = recursiveGetType(name, child);
    if (ret) return ret;
  }
  return NULL;
}

type_t getType (const char *name)
{
  return recursiveGetType (name, rootType);
}

void destroyContainer (container_t container)
{
  //remove me from my father
  if (container->father){
    xbt_dict_remove(container->father->children, container->name);
  }

  //trace my destruction
  pajeDestroyContainer(SIMIX_get_clock(), container->type->id, container->id);

  //free
  xbt_free (container->name);
  xbt_free (container->id);
  xbt_free (container->children);
  xbt_free (container);
  container = NULL;
}


#endif /* HAVE_TRACING */
