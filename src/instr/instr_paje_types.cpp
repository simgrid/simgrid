/* Copyright (c) 2012, 2014-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_paje_types, instr, "Paje tracing event system (types)");

static simgrid::instr::Type* rootType = nullptr; /* the root type */

simgrid::instr::Type* PJ_type_get_root()
{
  return rootType;
}

simgrid::instr::Type::Type(const char* typeNameBuff, const char* key, std::string color, e_entity_types kind,
                           Type* father)
    : color_(color), kind_(kind), father_(father)
{
  if (typeNameBuff == nullptr || key == nullptr){
    THROWF(tracing_error, 0, "can't create a new type with name or key equal nullptr");
  }

  this->name_     = xbt_strdup(typeNameBuff);
  this->id_ = bprintf("%lld", instr_new_paje_id());

  if (father != nullptr){
    father->children_.insert({key, this});
    XBT_DEBUG("new type %s, child of %s", typeNameBuff, father->name_);
  }
}

simgrid::instr::Type::~Type()
{
  for (auto elm : values_) {
    XBT_DEBUG("free value %s, child of %s", elm.second->getCname(), elm.second->father_->name_);
    delete elm.second;
  }
  for (auto elm : children_) {
    delete elm.second;
  }
  xbt_free(name_);
  xbt_free(id_);
}

simgrid::instr::Type* simgrid::instr::Type::getChild(const char* name)
{
  simgrid::instr::Type* ret = this->getChildOrNull(name);
  if (ret == nullptr)
    THROWF(tracing_error, 2, "type with name (%s) not found in father type (%s)", name, this->name_);
  return ret;
}

simgrid::instr::Type* simgrid::instr::Type::getChildOrNull(const char* name)
{
  xbt_assert(name != nullptr, "can't get type with a nullptr name");

  simgrid::instr::Type* ret = nullptr;
  for (auto elm : children_) {
    if (strcmp(elm.second->name_, name) == 0) {
      if (ret != nullptr) {
        THROWF (tracing_error, 0, "there are two children types with the same name?");
      } else {
        ret = elm.second;
      }
    }
  }
  return ret;
}

simgrid::instr::Type* simgrid::instr::Type::containerNew(const char* name, simgrid::instr::Type* father)
{
  if (name == nullptr){
    THROWF (tracing_error, 0, "can't create a container type with a nullptr name");
  }

  simgrid::instr::Type* ret = new simgrid::instr::Type(name, name, "", TYPE_CONTAINER, father);
  if (father == nullptr) {
    rootType = ret;
  } else {
    XBT_DEBUG("ContainerType %s(%s), child of %s(%s)", ret->name_, ret->id_, father->name_, father->id_);
    LogContainerTypeDefinition(ret);
  }
  return ret;
}

simgrid::instr::Type* simgrid::instr::Type::eventNew(const char* name, simgrid::instr::Type* father)
{
  if (name == nullptr){
    THROWF (tracing_error, 0, "can't create an event type with a nullptr name");
  }

  Type* ret = new Type(name, name, "", TYPE_EVENT, father);
  XBT_DEBUG("EventType %s(%s), child of %s(%s)", ret->name_, ret->id_, father->name_, father->id_);
  LogDefineEventType(ret);
  return ret;
}

simgrid::instr::Type* simgrid::instr::Type::variableNew(const char* name, std::string color,
                                                        simgrid::instr::Type* father)
{
  if (name == nullptr){
    THROWF (tracing_error, 0, "can't create a variable type with a nullptr name");
  }

  Type* ret = nullptr;

  if (color.empty()) {
    char white[INSTR_DEFAULT_STR_SIZE] = "1 1 1";
    ret = new Type (name, name, white, TYPE_VARIABLE, father);
  }else{
    ret = new Type (name, name, color, TYPE_VARIABLE, father);
  }
  XBT_DEBUG("VariableType %s(%s), child of %s(%s)", ret->name_, ret->id_, father->name_, father->id_);
  LogVariableTypeDefinition (ret);
  return ret;
}

simgrid::instr::Type* simgrid::instr::Type::linkNew(const char* name, Type* father, Type* source, Type* dest)
{
  if (name == nullptr){
    THROWF (tracing_error, 0, "can't create a link type with a nullptr name");
  }

  char key[INSTR_DEFAULT_STR_SIZE];
  snprintf(key, INSTR_DEFAULT_STR_SIZE, "%s-%s-%s", name, source->id_, dest->id_);
  Type* ret = new Type(name, key, "", TYPE_LINK, father);
  XBT_DEBUG("LinkType %s(%s), child of %s(%s)  %s(%s)->%s(%s)", ret->name_, ret->id_, father->name_, father->id_,
            source->name_, source->id_, dest->name_, dest->id_);
  LogLinkTypeDefinition(ret, source, dest);
  return ret;
}

simgrid::instr::Type* simgrid::instr::Type::stateNew(const char* name, Type* father)
{
  if (name == nullptr){
    THROWF (tracing_error, 0, "can't create a state type with a nullptr name");
  }

  Type* ret = new Type(name, name, "", TYPE_STATE, father);
  XBT_DEBUG("StateType %s(%s), child of %s(%s)", ret->name_, ret->id_, father->name_, father->id_);
  LogStateTypeDefinition(ret);
  return ret;
}
