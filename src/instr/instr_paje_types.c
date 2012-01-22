/* Copyright (c) 2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */
#include "instr/instr_private.h"

#ifdef HAVE_TRACING

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_paje_types, instr, "Paje tracing event system (types)");

static type_t rootType = NULL;        /* the root type */
static xbt_dict_t allTypes = NULL;    /* all declared types indexed by name */

void PJ_type_alloc ()
{
  allTypes = xbt_dict_new_homogeneous (NULL);
}

void PJ_type_release ()
{
  xbt_dict_free (&allTypes);
  rootType = NULL;
  allTypes = NULL;
}

type_t PJ_type_get_root ()
{
  return rootType;
}

static type_t newType (const char *typename, const char *key, const char *color, e_entity_types kind, type_t father)
{
  type_t ret = xbt_new0(s_type_t, 1);
  ret->name = xbt_strdup (typename);
  ret->father = father;
  ret->kind = kind;
  ret->children = xbt_dict_new_homogeneous(NULL);
  ret->values = xbt_dict_new_homogeneous(NULL);
  ret->color = xbt_strdup (color);

  char str_id[INSTR_DEFAULT_STR_SIZE];
  snprintf (str_id, INSTR_DEFAULT_STR_SIZE, "%lld", instr_new_paje_id());
  ret->id = xbt_strdup (str_id);

  if (father != NULL){
    xbt_dict_set (father->children, key, ret, NULL);
    XBT_DEBUG("new type %s, child of %s", typename, father->name);
  }

  xbt_dict_set (allTypes, typename, ret, NULL);
  return ret;
}

void PJ_type_free (type_t type)
{
  xbt_dict_remove (allTypes, type->name);

  val_t value;
  char *value_name;
  xbt_dict_cursor_t cursor = NULL;
  xbt_dict_foreach(type->values, cursor, value_name, value) {
    PJ_value_free (value);
  }
  xbt_dict_free (&type->values);
  xbt_free (type->name);
  xbt_free (type->id);
  xbt_free (type->color);
  xbt_dict_free (&type->children);
  xbt_free (type);
  type = NULL;
}

static void recursiveDestroyType (type_t type)
{
  XBT_DEBUG("recursiveDestroyType %s", type->name);
  xbt_dict_cursor_t cursor = NULL;
  type_t child;
  char *child_name;
  xbt_dict_foreach(type->children, cursor, child_name, child) {
    recursiveDestroyType (child);
  }
  PJ_type_free(type);
}

void PJ_type_free_all (void)
{
  recursiveDestroyType (PJ_type_get_root());
  rootType = NULL;

  //checks
  if (xbt_dict_length(allTypes) != 0){
    THROWF(tracing_error, 0, "some types still present even after destroying all of them");
  }
}

static type_t recursiveGetType (const char *name, type_t root)
{
  if (strcmp (root->name, name) == 0) return root;

  xbt_dict_cursor_t cursor = NULL;
  type_t child;
  char *child_name;
  type_t ret = NULL;
  xbt_dict_foreach(root->children, cursor, child_name, child) {
    type_t found = recursiveGetType(name, child);
    if (found){
      if (ret == NULL){
        ret = found;
      }else{
        THROWF(tracing_error, 0, "found two types with the same name");
      }
    }
  }
  return ret;
}

type_t PJ_type_get (const char *name, type_t father)
{
  return recursiveGetType (name, father);
}

type_t PJ_type_container_new (const char *name, type_t father)
{
  type_t ret = (type_t)xbt_dict_get_or_null (allTypes, name);
  if (ret){
    THROWF(tracing_error, 1, "container type with name %s already exists", name);
  }

  ret = newType (name, name, NULL, TYPE_CONTAINER, father);
  if (father == NULL){
    rootType = ret;
  }

  if(father){
    XBT_DEBUG("ContainerType %s(%s), child of %s(%s)", ret->name, ret->id, father->name, father->id);
    new_pajeDefineContainerType (ret);
  }
  return ret;
}

type_t PJ_type_event_new (const char *name, const char *color, type_t father)
{
  type_t ret = (type_t)xbt_dict_get_or_null (allTypes, name);
  if (ret){
    THROWF(tracing_error, 1, "event type with name %s already exists", name);
  }

  char white[INSTR_DEFAULT_STR_SIZE] = "1 1 1";
  if (!color){
    ret = newType (name, name, white, TYPE_EVENT, father);
  }else{
    ret = newType (name, name, color, TYPE_EVENT, father);
  }
  XBT_DEBUG("EventType %s(%s), child of %s(%s)", ret->name, ret->id, father->name, father->id);
  new_pajeDefineEventType(ret);
  return ret;
}

type_t PJ_type_variable_new (const char *name, const char *color, type_t father)
{
  type_t ret = (type_t)xbt_dict_get_or_null (allTypes, name);
  if (ret){
    THROWF(tracing_error, 1, "variable type with name %s already exists", name);
  }

  char white[INSTR_DEFAULT_STR_SIZE] = "1 1 1";
  if (!color){
    ret = newType (name, name, white, TYPE_VARIABLE, father);
  }else{
    ret = newType (name, name, color, TYPE_VARIABLE, father);
  }
  XBT_DEBUG("VariableType %s(%s), child of %s(%s)", ret->name, ret->id, father->name, father->id);
  new_pajeDefineVariableType (ret);
  return ret;
}

type_t PJ_type_link_new (const char *name, type_t father, type_t source, type_t dest)
{
  type_t ret = (type_t)xbt_dict_get_or_null (allTypes, name);
  if (ret){
    THROWF(tracing_error, 1, "link type with name %s already exists", name);
  }

  char key[INSTR_DEFAULT_STR_SIZE];
  snprintf (key, INSTR_DEFAULT_STR_SIZE, "%s-%s-%s", name, source->id, dest->id);
  ret = newType (name, key, NULL, TYPE_LINK, father);
  XBT_DEBUG("LinkType %s(%s), child of %s(%s)  %s(%s)->%s(%s)", ret->name, ret->id, father->name, father->id, source->name, source->id, dest->name, dest->id);
  new_pajeDefineLinkType(ret, source, dest);
  return ret;
}

type_t PJ_type_state_new (const char *name, type_t father)
{
  type_t ret = (type_t)xbt_dict_get_or_null (allTypes, name);
  if (ret){
    THROWF(tracing_error, 1, "state type with name %s already exists", name);
  }

  ret = newType (name, name, NULL, TYPE_STATE, father);
  XBT_DEBUG("StateType %s(%s), child of %s(%s)", ret->name, ret->id, father->name, father->id);
  new_pajeDefineStateType(ret);
  return ret;
}

#endif
