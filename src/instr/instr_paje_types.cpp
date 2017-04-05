/* Copyright (c) 2012, 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_paje_types, instr, "Paje tracing event system (types)");

static type_t rootType = nullptr;        /* the root type */

void PJ_type_alloc ()
{
}

void PJ_type_release ()
{
  rootType = nullptr;
}

type_t PJ_type_get_root ()
{
  return rootType;
}

static type_t newType (const char *typeNameBuff, const char *key, const char *color, e_entity_types kind, type_t father)
{
  if (typeNameBuff == nullptr || key == nullptr){
    THROWF(tracing_error, 0, "can't create a new type with name or key equal nullptr");
  }

  type_t ret = xbt_new0(s_type_t, 1);
  ret->name = xbt_strdup (typeNameBuff);
  ret->father = father;
  ret->kind = kind;
  ret->children = xbt_dict_new_homogeneous(nullptr);
  ret->values = xbt_dict_new_homogeneous(nullptr);
  ret->color = xbt_strdup (color);

  char str_id[INSTR_DEFAULT_STR_SIZE];
  snprintf (str_id, INSTR_DEFAULT_STR_SIZE, "%lld", instr_new_paje_id());
  ret->id = xbt_strdup (str_id);

  if (father != nullptr){
    xbt_dict_set (father->children, key, ret, nullptr);
    XBT_DEBUG("new type %s, child of %s", typeNameBuff, father->name);
  }
  return ret;
}

void PJ_type_free (type_t type)
{
  val_t value;
  char *value_name;
  xbt_dict_cursor_t cursor = nullptr;
  xbt_dict_foreach(type->values, cursor, value_name, value) {
    PJ_value_free (value);
  }
  xbt_dict_free (&type->values);
  xbt_free (type->name);
  xbt_free (type->id);
  xbt_free (type->color);
  xbt_dict_free (&type->children);
  xbt_free (type);
  type = nullptr;
}

static void recursiveDestroyType (type_t type)
{
  XBT_DEBUG("recursiveDestroyType %s", type->name);
  xbt_dict_cursor_t cursor = nullptr;
  type_t child;
  char *child_name;
  xbt_dict_foreach(type->children, cursor, child_name, child) {
    recursiveDestroyType (child);
  }
  PJ_type_free(type);
}

void PJ_type_free_all ()
{
  recursiveDestroyType (PJ_type_get_root());
  rootType = nullptr;
}

type_t PJ_type_get (const char *name, type_t father)
{
  type_t ret = PJ_type_get_or_null (name, father);
  if (ret == nullptr){
    THROWF (tracing_error, 2, "type with name (%s) not found in father type (%s)", name, father->name);
  }
  return ret;
}

type_t PJ_type_get_or_null (const char *name, type_t father)
{
  if (name == nullptr || father == nullptr){
    THROWF (tracing_error, 0, "can't get type with a nullptr name or from a nullptr father");
  }

  type_t ret = nullptr;
  type_t child;
  char *child_name;
  xbt_dict_cursor_t cursor = nullptr;
  xbt_dict_foreach(father->children, cursor, child_name, child) {
    if (strcmp (child->name, name) == 0){
      if (ret != nullptr){
        THROWF (tracing_error, 0, "there are two children types with the same name?");
      }else{
        ret = child;
      }
    }
  }
  return ret;
}

type_t PJ_type_container_new (const char *name, type_t father)
{ s_defineContainerType EltContainer;
  if (name == nullptr){
    THROWF (tracing_error, 0, "can't create a container type with a nullptr name");
  }

  type_t ret = nullptr;

  ret = newType (name, name, nullptr, TYPE_CONTAINER, father);
  if (father == nullptr){
    rootType = ret;
  }

  if(father){
    XBT_DEBUG("ContainerType %s(%s), child of %s(%s)", ret->name, ret->id, father->name, father->id);
    EltContainer.new_pajeDefineContainerType (ret);
  }
  return ret;
}

type_t PJ_type_event_new (const char *name, type_t father)
{
  if (name == nullptr){
    THROWF (tracing_error, 0, "can't create an event type with a nullptr name");
  }

  type_t ret = newType (name, name, nullptr, TYPE_EVENT, father);
  XBT_DEBUG("EventType %s(%s), child of %s(%s)", ret->name, ret->id, father->name, father->id);
  new_pajeDefineEventType(ret);
  return ret;
}

type_t PJ_type_variable_new (const char *name, const char *color, type_t father)
{
  if (name == nullptr){
    THROWF (tracing_error, 0, "can't create a variable type with a nullptr name");
  }

  type_t ret = nullptr;

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
  if (name == nullptr){
    THROWF (tracing_error, 0, "can't create a link type with a nullptr name");
  }

  type_t ret = nullptr;

  char key[INSTR_DEFAULT_STR_SIZE];
  snprintf (key, INSTR_DEFAULT_STR_SIZE, "%s-%s-%s", name, source->id, dest->id);
  ret = newType (name, key, nullptr, TYPE_LINK, father);
  XBT_DEBUG("LinkType %s(%s), child of %s(%s)  %s(%s)->%s(%s)", ret->name, ret->id, father->name, father->id,
            source->name, source->id, dest->name, dest->id);
  new_pajeDefineLinkType(ret, source, dest);
  return ret;
}

type_t PJ_type_state_new (const char *name, type_t father)
{
  if (name == nullptr){
    THROWF (tracing_error, 0, "can't create a state type with a nullptr name");
  }

  type_t ret = nullptr;

  ret = newType (name, name, nullptr, TYPE_STATE, father);
  XBT_DEBUG("StateType %s(%s), child of %s(%s)", ret->name, ret->id, father->name, father->id);
  new_pajeDefineStateType(ret);
  return ret;
}
