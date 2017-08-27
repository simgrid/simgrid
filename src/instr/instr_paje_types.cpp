/* Copyright (c) 2012, 2014-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_paje_types, instr, "Paje tracing event system (types)");

static Type* rootType = nullptr;        /* the root type */

void PJ_type_release ()
{
  rootType = nullptr;
}

type_t PJ_type_get_root ()
{
  return rootType;
}

Type::Type (const char *typeNameBuff, const char *key, const char *color, e_entity_types kind, Type* father)
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
  Value* val;
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
  Type* child;
  char *child_name;
  xbt_dict_foreach(type->children, cursor, child_name, child) {
    recursiveDestroyType (child);
  }
  PJ_type_free(type);
}

type_t PJ_type_get (const char *name, Type* father)
{
  Type* ret = Type::getOrNull (name, father);
  if (ret == nullptr){
    THROWF (tracing_error, 2, "type with name (%s) not found in father type (%s)", name, father->name);
  }
  return ret;
}

type_t Type::getOrNull (const char *name, Type* father)
{
  if (name == nullptr || father == nullptr){
    THROWF (tracing_error, 0, "can't get type with a nullptr name or from a nullptr father");
  }

  Type* ret = nullptr;
  Type* child;
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

type_t Type::containerNew (const char *name, Type* father)
{
  if (name == nullptr){
    THROWF (tracing_error, 0, "can't create a container type with a nullptr name");
  }

  Type* ret = new Type (name, name, nullptr, TYPE_CONTAINER, father);
  if (father == nullptr) {
    rootType = ret;
  } else {
    XBT_DEBUG("ContainerType %s(%s), child of %s(%s)", ret->name, ret->id, father->name, father->id);
    DefineContainerEvent(ret);
  }
  return ret;
}

type_t Type::eventNew (const char *name, Type* father)
{
  if (name == nullptr){
    THROWF (tracing_error, 0, "can't create an event type with a nullptr name");
  }

  Type* ret = new Type (name, name, nullptr, TYPE_EVENT, father);
  XBT_DEBUG("EventType %s(%s), child of %s(%s)", ret->name, ret->id, father->name, father->id);
  LogDefineEventType(ret);
  return ret;
}

type_t Type::variableNew (const char *name, const char *color, Type* father)
{
  if (name == nullptr){
    THROWF (tracing_error, 0, "can't create a variable type with a nullptr name");
  }

  Type* ret = nullptr;

  if (not color) {
    char white[INSTR_DEFAULT_STR_SIZE] = "1 1 1";
    ret = new Type (name, name, white, TYPE_VARIABLE, father);
  }else{
    ret = new Type (name, name, color, TYPE_VARIABLE, father);
  }
  XBT_DEBUG("VariableType %s(%s), child of %s(%s)", ret->name, ret->id, father->name, father->id);
  LogVariableTypeDefinition (ret);
  return ret;
}

type_t Type::linkNew (const char *name, Type* father, Type* source, Type* dest)
{
  if (name == nullptr){
    THROWF (tracing_error, 0, "can't create a link type with a nullptr name");
  }

  Type* ret = nullptr;

  char key[INSTR_DEFAULT_STR_SIZE];
  snprintf (key, INSTR_DEFAULT_STR_SIZE, "%s-%s-%s", name, source->id, dest->id);
  ret = new Type (name, key, nullptr, TYPE_LINK, father);
  XBT_DEBUG("LinkType %s(%s), child of %s(%s)  %s(%s)->%s(%s)", ret->name, ret->id, father->name, father->id,
            source->name, source->id, dest->name, dest->id);
  LogLinkTypeDefinition(ret, source, dest);
  return ret;
}

type_t Type::stateNew (const char *name, Type* father)
{
  if (name == nullptr){
    THROWF (tracing_error, 0, "can't create a state type with a nullptr name");
  }

  Type* ret = nullptr;

  ret = new Type (name, name, nullptr, TYPE_STATE, father);
  XBT_DEBUG("StateType %s(%s), child of %s(%s)", ret->name, ret->id, father->name, father->id);
  LogStateTypeDefinition(ret);
  return ret;
}
