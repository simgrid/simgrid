/* Copyright (c) 2012, 2014-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_paje_types, instr, "Paje tracing event system (types)");

static type_t rootType = nullptr;        /* the root type */

void PJ_type_release ()
{
  rootType = nullptr;
}

type_t PJ_type_get_root ()
{
  return rootType;
}

s_type::s_type (const char *typeNameBuff, const char *key, const char *color, e_entity_types kind, type_t father)
{
  if (typeNameBuff == nullptr || key == nullptr){
    THROWF(tracing_error, 0, "can't create a new type with name or key equal nullptr");
  }

  this->name = xbt_strdup (typeNameBuff);
  this->father = father;
  this->kind = kind;
  this->children = xbt_dict_new_homogeneous(nullptr);
  this->values = xbt_dict_new_homogeneous(nullptr);
  this->color = xbt_strdup (color);

  char str_id[INSTR_DEFAULT_STR_SIZE];
  snprintf (str_id, INSTR_DEFAULT_STR_SIZE, "%lld", instr_new_paje_id());
  this->id = xbt_strdup (str_id);

  if (father != nullptr){
    xbt_dict_set (father->children, key, this, nullptr);
    XBT_DEBUG("new type %s, child of %s", typeNameBuff, father->name);
  }
}

void PJ_type_free (type_t type)
{
  value* val;
  char *value_name;
  xbt_dict_cursor_t cursor = nullptr;
  xbt_dict_foreach (type->values, cursor, value_name, val) {
    XBT_DEBUG("free value %s, child of %s", val->name, val->father->name);
    xbt_free(val);
  }
  xbt_dict_free (&type->values);
  xbt_free (type->name);
  xbt_free (type->id);
  xbt_free (type->color);
  xbt_dict_free (&type->children);
  xbt_free (type);
  type = nullptr;
}

void recursiveDestroyType (type_t type)
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

type_t PJ_type_get (const char *name, type_t father)
{
  type_t ret = s_type::getOrNull (name, father);
  if (ret == nullptr){
    THROWF (tracing_error, 2, "type with name (%s) not found in father type (%s)", name, father->name);
  }
  return ret;
}

type_t s_type::getOrNull (const char *name, type_t father)
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

type_t s_type::containerNew (const char *name, type_t father)
{
  if (name == nullptr){
    THROWF (tracing_error, 0, "can't create a container type with a nullptr name");
  }

  type_t ret = new s_type (name, name, nullptr, TYPE_CONTAINER, father);
  if (father == nullptr) {
    rootType = ret;
  } else {
    XBT_DEBUG("ContainerType %s(%s), child of %s(%s)", ret->name, ret->id, father->name, father->id);
    DefineContainerEvent(ret);
  }
  return ret;
}

type_t s_type::eventNew (const char *name, type_t father)
{
  if (name == nullptr){
    THROWF (tracing_error, 0, "can't create an event type with a nullptr name");
  }

  type_t ret = new s_type (name, name, nullptr, TYPE_EVENT, father);
  XBT_DEBUG("EventType %s(%s), child of %s(%s)", ret->name, ret->id, father->name, father->id);
  LogDefineEventType(ret);
  return ret;
}

type_t s_type::variableNew (const char *name, const char *color, type_t father)
{
  if (name == nullptr){
    THROWF (tracing_error, 0, "can't create a variable type with a nullptr name");
  }

  type_t ret = nullptr;

  if (not color) {
    char white[INSTR_DEFAULT_STR_SIZE] = "1 1 1";
    ret = new s_type (name, name, white, TYPE_VARIABLE, father);
  }else{
    ret = new s_type (name, name, color, TYPE_VARIABLE, father);
  }
  XBT_DEBUG("VariableType %s(%s), child of %s(%s)", ret->name, ret->id, father->name, father->id);
  LogVariableTypeDefinition (ret);
  return ret;
}

type_t s_type::linkNew (const char *name, type_t father, type_t source, type_t dest)
{
  if (name == nullptr){
    THROWF (tracing_error, 0, "can't create a link type with a nullptr name");
  }

  type_t ret = nullptr;

  char key[INSTR_DEFAULT_STR_SIZE];
  snprintf (key, INSTR_DEFAULT_STR_SIZE, "%s-%s-%s", name, source->id, dest->id);
  ret = new s_type (name, key, nullptr, TYPE_LINK, father);
  XBT_DEBUG("LinkType %s(%s), child of %s(%s)  %s(%s)->%s(%s)", ret->name, ret->id, father->name, father->id,
            source->name, source->id, dest->name, dest->id);
  LogLinkTypeDefinition(ret, source, dest);
  return ret;
}

type_t s_type::stateNew (const char *name, type_t father)
{
  if (name == nullptr){
    THROWF (tracing_error, 0, "can't create a state type with a nullptr name");
  }

  type_t ret = nullptr;

  ret = new s_type (name, name, nullptr, TYPE_STATE, father);
  XBT_DEBUG("StateType %s(%s), child of %s(%s)", ret->name, ret->id, father->name, father->id);
  LogStateTypeDefinition(ret);
  return ret;
}
